/* ==========================================================================
 * 06_http_server.cpp —— 手写一个 HTTP 服务器，浏览器直接访问
 *
 * 这是 C/C++ 常被低估的能力：不用任何框架，几百行就能跑一个真的 Web 服务。
 * 你会看到：
 *   - HTTP 其实就是「纯文本 + 一个空行 + 字节数」，没有什么魔法
 *   - socket 用 RAII 包起来（closesocket 再也不会忘记）
 *   - std::function + unordered_map 做「路由表」，加接口只要加一行
 *   - std::atomic / mutex 保护共享计数，多线程访问同一个服务
 *   - 自己写一个压测客户端，量出 QPS 和 P95 延迟
 *
 * 编译：g++ -std=c++20 -O2 cpp\06_http_server.cpp -o out\cpp06.exe -lws2_32
 * 运行：out\cpp06.exe                然后浏览器打开  http://127.0.0.1:8080/
 *      ：out\cpp06.exe 9000          换端口
 * 停止：Ctrl+C
 * 说明：Windows 防火墙可能弹窗，选「允许」；只监听 127.0.0.1，不对外开放。
 * ========================================================================== */
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>

/* ---------------- RAII：把 socket 当成对象管 ---------------- */
class Wsa {                       /* 全局一次初始化/清理，存活期由 main 保证 */
public:
    Wsa() { WSADATA d; WSAStartup(MAKEWORD(2, 2), &d); }
    ~Wsa() { WSACleanup(); }
    Wsa(const Wsa &) = delete;
    Wsa &operator=(const Wsa &) = delete;
};

class Socket {                    /* move-only：所有权明确，离开作用域必然关闭 */
public:
    Socket() = default;
    explicit Socket(SOCKET s) : s_(s) {}
    ~Socket() { reset(); }
    Socket(const Socket &) = delete;
    Socket &operator=(const Socket &) = delete;
    Socket(Socket &&o) noexcept : s_(o.s_) { o.s_ = INVALID_SOCKET; }
    Socket &operator=(Socket &&o) noexcept
    {
        if (this != &o) { reset(); s_ = o.s_; o.s_ = INVALID_SOCKET; }
        return *this;
    }

    SOCKET get() const { return s_; }
    void reset() { if (s_ != INVALID_SOCKET) { ::closesocket(s_); s_ = INVALID_SOCKET; } }

    bool send_all(std::string_view data) const
    {
        size_t off = 0;
        while (off < data.size()) {
            int n = ::send(s_, data.data() + off, (int)(data.size() - off), 0);
            if (n == SOCKET_ERROR || n == 0) return false;
            off += (size_t)n;
        }
        return true;
    }
    /* 一直读到缓冲区里出现 "\r\n\r\n"（HTTP 头部结束） */
    bool read_headers(std::string &buf) const
    {
        char tmp[4096];
        for (;;) {
            int n = ::recv(s_, tmp, (int)sizeof tmp, 0);
            if (n <= 0) return false;
            buf.append(tmp, (size_t)n);
            if (buf.find("\r\n\r\n") != std::string::npos) return true;
            if (buf.size() > (1u << 20)) return false;            /* 头部超 1MB 视为攻击 */
        }
    }
    bool read_more(std::string &buf, size_t need) const
    {
        char tmp[8192];
        while (buf.size() < need) {
            int n = ::recv(s_, tmp, (int)sizeof tmp, 0);
            if (n <= 0) return false;
            buf.append(tmp, (size_t)n);
        }
        return true;
    }

private:
    SOCKET s_ = INVALID_SOCKET;
};

/* ---------------- HTTP 的两个「数据结构」 ---------------- */
struct Request {
    std::string method, path, version, body;
    std::map<std::string, std::string> headers;   /* key 已转小写 */
    std::map<std::string, std::string> query;

    std::string header(std::string_view name) const
    {
        auto it = headers.find(std::string(name));
        return it == headers.end() ? std::string() : it->second;
    }
    std::string param(std::string_view name) const
    {
        auto it = query.find(std::string(name));
        return it == query.end() ? std::string() : it->second;
    }
};

struct Response {
    int status = 200;
    std::string ctype = "text/plain; charset=utf-8";
    std::string body;
    std::map<std::string, std::string> headers;   /* 额外响应头 */

    static Response text(std::string s) { Response r; r.body = std::move(s); return r; }
    static Response html(std::string s) { Response r; r.ctype = "text/html; charset=utf-8"; r.body = std::move(s); return r; }
    static Response json(std::string s) { Response r; r.ctype = "application/json; charset=utf-8"; r.body = std::move(s); return r; }
    static Response not_found(std::string s) { Response r; r.status = 404; r.ctype = "text/html; charset=utf-8"; r.body = std::move(s); return r; }
};

static std::string to_lower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    return s;
}

static std::string url_decode(std::string_view in)
{
    auto hx = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return 16;
    };
    std::string out;
    for (size_t i = 0; i < in.size(); i++) {
        if (in[i] == '+') out += ' ';
        else if (in[i] == '%' && i + 2 < in.size()) {
            int hi = hx(in[i + 1]), lo = hx(in[i + 2]);
            if (hi < 16 && lo < 16) { out += (char)(hi * 16 + lo); i += 2; }
            else out += in[i];
        } else out += in[i];
    }
    return out;
}

static const char *status_text(int code)
{
    switch (code) {
        case 200: return "OK";
        case 204: return "No Content";
        case 400: return "Bad Request";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 500: return "Internal Server Error";
        default:  return "Unknown";
    }
}

/* 只处理「请求行 + 头部 + 带 Content-Length 的 body」，但这一套已经是真 HTTP/1.1 */
static std::optional<Request> parse_request(const std::string &raw)
{
    size_t head_end = raw.find("\r\n\r\n");
    size_t first_end = raw.find("\r\n");
    if (head_end == std::string::npos || first_end == std::string::npos) return std::nullopt;

    Request req;
    {   /* 请求行：GET /path?a=1 HTTP/1.1 */
        std::istringstream ls(raw.substr(0, first_end));
        std::string target;
        ls >> req.method >> target >> req.version;
        if (req.method.empty() || target.empty()) return std::nullopt;
        size_t q = target.find('?');
        req.path = q == std::string::npos ? url_decode(target) : url_decode(target.substr(0, q));
        if (q != std::string::npos) {
            std::string qs = target.substr(q + 1);
            size_t p = 0;
            while (p <= qs.size()) {
                size_t amp = qs.find('&', p);
                std::string kv = qs.substr(p, amp == std::string::npos ? std::string::npos : amp - p);
                size_t eq = kv.find('=');
                if (!kv.empty())
                    req.query[eq == std::string::npos ? url_decode(kv) : url_decode(kv.substr(0, eq))] =
                        eq == std::string::npos ? "1" : url_decode(kv.substr(eq + 1));
                if (amp == std::string::npos) break;
                p = amp + 1;
            }
        }
    }
    {   /* 头部 */
        size_t pos = first_end + 2;
        while (pos < head_end) {
            size_t e = raw.find("\r\n", pos);
            if (e == std::string::npos || e > head_end) break;
            std::string line = raw.substr(pos, e - pos);
            size_t colon = line.find(':');
            if (colon != std::string::npos) {
                std::string v = line.substr(colon + 1);
                size_t b = v.find_first_not_of(" \t");
                req.headers[to_lower(line.substr(0, colon))] = b == std::string::npos ? std::string() : v.substr(b);
            }
            pos = e + 2;
        }
    }
    {   /* body */
        std::string cl = req.header("content-length");
        size_t want = cl.empty() ? 0 : (size_t)std::stoul(cl);
        if (want > (16u << 20)) return std::nullopt;               /* 16MB 上限 */
        req.body = raw.substr(head_end + 4);
        if (req.body.size() < want) return std::nullopt;            /* 还没收全，继续读 */
        req.body.resize(want);
    }
    return req;
}

static std::string serialize(const Response &r, bool keep_alive)
{
    std::ostringstream o;
    o << "HTTP/1.1 " << r.status << " " << status_text(r.status) << "\r\n"
      << "Content-Type: " << r.ctype << "\r\n"
      << "Content-Length: " << r.body.size() << "\r\n"
      << "Connection: " << (keep_alive ? "keep-alive" : "close") << "\r\n"
      << "X-Server: cpp06-handmade\r\n";
    for (const auto &kv : r.headers) o << kv.first << ": " << kv.second << "\r\n";
    o << "\r\n" << r.body;
    return o.str();
}
/* ---------------- 路由表：路径 -> 处理函数（std::function 让函数能存进容器） ---------------- */
using Handler = std::function<Response(const Request &)>;

struct Stats {
    std::atomic<long> total{0}, bytes_in{0}, bytes_out{0}, conns{0};
    std::mutex             mtx;
    std::map<std::string, long> per_path;
    std::chrono::steady_clock::time_point started = std::chrono::steady_clock::now();
};
static Stats g_stats;

static std::string html_escape(std::string_view s)
{
    std::string out;
    for (char c : s) {
        switch (c) {
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '&': out += "&amp;"; break;
            case '"': out += "&quot;"; break;
            default:  out += c;
        }
    }
    return out;
}

/* 页面本身是静态的，数据靠前端每 秒轮询 /api/stats —— 服务端不用管浏览器 */
static const char *INDEX_HTML = R"HTML(<!doctype html><html lang="zh"><head><meta charset="utf-8">
<title>cpp06 - 手写的 HTTP 服务器</title>
<style>
 body{font-family:system-ui,"Microsoft YaHei",sans-serif;background:#12141c;color:#e8eaf2;margin:0;padding:40px}
 h1{margin:0 0 4px;font-size:22px} .sub{color:#8b93b0;margin-bottom:24px;font-size:13px}
 .grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(170px,1fr));gap:12px;max-width:760px}
 .card{background:#1b1f2b;border:1px solid #2a3042;border-radius:10px;padding:14px}
 .k{color:#8b93b0;font-size:12px} .v{font-size:24px;font-weight:600;margin-top:6px}
 a{color:#6ea8fe;text-decoration:none;font-size:14px} a:hover{text-decoration:underline}
 ul{line-height:1.9} code{background:#0d0f16;padding:2px 6px;border-radius:4px;color:#9fd47a}
 pre{background:#0d0f16;padding:12px;border-radius:8px;overflow:auto}
</style></head><body>
<h1>这台服务器上跑的是你自己写的 HTTP 服务器</h1>
<div class="sub">没有 nginx，没有 cpp-httplib，只有 winsock + std::string + std::function</div>
<div class="grid">
 <div class="card"><div class="k">累计请求</div><div class="v" id="total">-</div></div>
 <div class="card"><div class="k">连接数</div><div class="v" id="conns">-</div></div>
 <div class="card"><div class="k">收 / 发字节</div><div class="v" id="bytes">-</div></div>
 <div class="card"><div class="k">运行时长</div><div class="v" id="up">-</div></div>
</div>
<h3>可以点的东西</h3>
<ul>
 <li><a href="/api/time">/api/time</a> —— 返回 JSON</li>
 <li><a href="/api/slow?ms=1200">/api/slow?ms=1200</a> —— 故意卡一下，看看别的请求会不会被堵住</li>
 <li><a href="/search?q=C%2B%2B%20%E7%BD%91%E7%BB%9C">/search?q=...</a> —— 查询串解析 + URL 解码</li>
 <li><a href="/does-not-exist">/does-not-exist</a> —— 404 页面</li>
 <li>POST：<code>curl -d "hello" http://127.0.0.1:8080/api/echo</code></li>
</ul>
<pre id="log">等待数据…</pre>
<script>
async function tick(){
  try{
    const r = await fetch('/api/stats'); const j = await r.json();
    total.textContent = j.total; conns.textContent = j.connections;
    bytes.textContent = (j.bytes_in/1024).toFixed(1)+'K / '+(j.bytes_out/1024).toFixed(1)+'K';
    up.textContent = j.uptime_sec + 's';
    log.textContent = JSON.stringify(j.by_path, null, 2);
  }catch(e){ log.textContent = 'fetch 失败: ' + e; }
}
tick(); setInterval(tick, 1000);
</script></body></html>)HTML";

static Response h_index(const Request &)
{
    return Response::html(INDEX_HTML);
}
static Response h_stats(const Request &)
{
    std::map<std::string, long> snapshot;
    {
        std::lock_guard<std::mutex> lk(g_stats.mtx);
        snapshot = g_stats.per_path;
    }
    std::ostringstream paths;
    bool first = true;
    for (const auto &kv : snapshot) {
        if (!first) paths << ",";
        first = false;
        paths << "\"" << kv.first << "\":" << kv.second;
    }
    double up = std::chrono::duration<double>(std::chrono::steady_clock::now() - g_stats.started).count();
    return Response::json(std::format("{{\"total\":{},\"connections\":{},\"bytes_in\":{},\"bytes_out\":{},"
                                      "\"uptime_sec\":{:.0f},\"by_path\":{{{}}}}}",
                                      g_stats.total.load(), g_stats.conns.load(), g_stats.bytes_in.load(),
                                      g_stats.bytes_out.load(), up, paths.str()));
}
static Response h_time(const Request &)
{
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    char buf[64];
    std::strftime(buf, sizeof buf, "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    return Response::json(std::format("{{\"now\":\"{}\",\"epoch\":{}}}", buf, (long long)t));
}
static Response h_slow(const Request &r)
{
    int ms = std::min(5000, std::max(0, std::atoi(r.param("ms").c_str())));
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));   /* 阻塞式：正好能体会 thread-per-connection 的价值 */
    return Response::json(std::format("{{\"slept_ms\":{}}}", ms));
}
static Response h_search(const Request &r)
{
    std::string q = html_escape(r.param("q"));                    /* 永远不要原样把用户输入塞进 HTML（XSS） */
    return Response::html(std::format("<pre>你搜的是：{}\n\n原始 query 串解析结果：</pre>", q));
}
static Response h_echo(const Request &r)
{
    if (r.method != "POST") {
        Response res;
        res.status = 405;
        res.body = "use POST\n";
        return res;
    }
    Response res = Response::json(std::format("{{\"length\":{},\"sha_like\":{},\"head\":\"{}\"}}",
                                              r.body.size(), (unsigned)(r.body.size() * 2654435761u),
                                              html_escape(r.body.substr(0, 40))));
    res.status = 201;
    res.headers["X-Echo"] = "yes";
    return res;
}

static const std::unordered_map<std::string, Handler> ROUTES = {
    {"/",          h_index},
    {"/index.html",h_index},
    {"/api/stats", h_stats},
    {"/api/time",  h_time},
    {"/api/slow",  h_slow},
    {"/api/echo",  h_echo},
    {"/search",    h_search},
};
/* ---------------- 一条连接：keep-alive 下连续处理多个请求 ---------------- */
static void serve_connection(Socket client, std::string peer)
{
    ++g_stats.conns;
    std::string buf;
    for (int i = 0; i < 200; i++) {                       /* 设个上限，防「永不关闭的连接」 */
        buf.clear();
        if (!client.read_headers(buf)) break;
        size_t head_end = buf.find("\r\n\r\n");
        if (head_end == std::string::npos) break;

        /* 先自己摸出 Content-Length，把 body 收全，再交给 parse_request */
        std::string head_block = to_lower(buf.substr(0, head_end));
        size_t clp = head_block.find("content-length:");
        if (clp != std::string::npos) {
            size_t need = (size_t)std::stoul(buf.substr(clp + 15));
            if (need > (16u << 20)) break;
            if (buf.size() < head_end + 4 + need && !client.read_more(buf, head_end + 4 + need)) break;
        }
        std::optional<Request> req = parse_request(buf);
        if (!req) {
            Response bad;
            bad.status = 400;
            bad.body = "bad request\n";
            client.send_all(serialize(bad, false));
            break;
        }

        ++g_stats.total;
        g_stats.bytes_in += (long)buf.size();
        {
            std::lock_guard<std::mutex> lk(g_stats.mtx);
            ++g_stats.per_path[req->path];                /* 共享 map：必须锁 */
        }

        Response res;
        auto it = ROUTES.find(req->path);
        if (it == ROUTES.end()) {
            res = Response::not_found(std::format(
                "<h1>404</h1><p>没有这个路径：<code>{}</code></p><p><a href=\"/\">回首页</a></p>",
                html_escape(req->path)));
        } else {
            try {
                res = it->second(*req);                   /* 处理函数抛异常也不会弄死服务器 */
            } catch (const std::exception &e) {
                res.status = 500;
                res.ctype = "application/json; charset=utf-8";
                res.body = std::format("{{\"error\":\"{}\"}}", e.what());
            }
        }
        bool keep_alive = req->version != "HTTP/1.0" && to_lower(req->header("connection")) != "close";
        std::string wire = serialize(res, keep_alive);
        g_stats.bytes_out += (long)wire.size();
        if (!client.send_all(wire)) break;
        std::printf("[srv] %-18s %s %s -> %d (%zu B)\n", peer.c_str(), req->method.c_str(),
                    req->path.c_str(), res.status, wire.size());
        std::fflush(stdout);
        if (!keep_alive) break;
    }
    --g_stats.conns;
}   /* client 在这里析构 -> 自动 closesocket，永远不会漏 */

static int run_server(int port)
{
    Socket srv(::socket(AF_INET, SOCK_STREAM, 0));
    if (srv.get() == INVALID_SOCKET) { std::printf("socket 失败: %d\n", WSAGetLastError()); return 1; }

    BOOL reuse = TRUE;
    setsockopt(srv.get(), SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof reuse);

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_port        = (u_short)port;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");        /* 只监听本机，不会被外网访问 */

    if (::bind(srv.get(), (sockaddr *)&addr, sizeof addr) == SOCKET_ERROR) {
        std::printf("bind 失败: %d（端口被占用？换个端口）\n", WSAGetLastError());
        return 1;
    }
    if (::listen(srv.get(), 64) == SOCKET_ERROR) { std::printf("listen 失败\n"); return 1; }

    std::printf("\n服务器已启动：http://127.0.0.1:%d/    （Ctrl+C 退出）\n\n", port);
    for (;;) {
        sockaddr_in peer{};
        int plen = (int)sizeof peer;
        SOCKET c = ::accept(srv.get(), (sockaddr *)&peer, &plen);
        if (c == INVALID_SOCKET) { std::printf("accept 失败: %d\n", WSAGetLastError()); break; }

        char ip[INET_ADDRSTRLEN] = "?";
        inet_ntop(AF_INET, &peer.sin_addr, ip, sizeof ip);
        std::string desc = std::string(ip) + ":" + std::to_string(ntohs(peer.sin_port));

        /* 一个连接一个线程：/api/slow 卡住时，其他请求照样能被响应。
           真实服务器不会这么开线程（1 万连接 = 1 万线程），
           正确做法是 cpp\04_thread_pool.cpp 的线程池，或 Windows 的 IOCP。 */
        std::thread([s = Socket(c), d = std::move(desc)]() mutable {
            serve_connection(std::move(s), std::move(d));
        }).detach();
    }
    return 0;
}
/* ---------------- 压测客户端：量一量自己写的服务器有多快 ---------------- */
static int run_bench(int port, int n)
{
    Socket cli(::socket(AF_INET, SOCK_STREAM, 0));
    sockaddr_in a{};
    a.sin_family      = AF_INET;
    a.sin_port        = (u_short)port;
    a.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (::connect(cli.get(), (sockaddr *)&a, sizeof a) == SOCKET_ERROR) {
        std::printf("连不上 127.0.0.1:%d —— 先在另一个窗口把服务器跑起来\n", port);
        return 1;
    }

    const std::string req = "GET /api/time HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: keep-alive\r\n\r\n";
    std::vector<double> lat;
    lat.reserve(n);
    auto t_all = std::chrono::steady_clock::now();

    for (int i = 0; i < n; i++) {
        auto t0 = std::chrono::steady_clock::now();
        if (!cli.send_all(req)) { std::printf("第 %d 个请求发送失败\n", i); break; }
        std::string buf;
        if (!cli.read_headers(buf)) break;
        size_t he = buf.find("\r\n\r\n");
        std::string low = to_lower(buf.substr(0, he == std::string::npos ? buf.size() : he));
        size_t clp = low.find("content-length:");
        if (clp != std::string::npos) {
            size_t need = (size_t)std::stoul(buf.substr(clp + 15));
            if (!cli.read_more(buf, he + 4 + need)) break;
        }
        lat.push_back(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count());
    }
    double total_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t_all).count();
    if (lat.empty()) { std::printf("一个请求都没完成\n"); return 1; }

    std::sort(lat.begin(), lat.end());
    auto pct = [&](double p) { return lat[(size_t)std::min<double>((double)(lat.size() - 1), p * lat.size())]; };
    double sum = 0;
    for (double v : lat) sum += v;

    std::printf("\n完成 %zu 次请求（复用同一条 keep-alive 连接）\n", lat.size());
    std::printf("  平均 %.2f ms   P50 %.2f   P90 %.2f   P99 %.2f   最大 %.2f\n",
                sum / (double)lat.size(), pct(.50), pct(.90), pct(.99), lat.back());
    std::printf("  吞吐 %.0f req/s（单线程串行，没并发）\n", lat.size() * 1000.0 / total_ms);
    std::printf("  想看并发：再开几个窗口同时跑 bench，然后回浏览器看首页数字的变化\n");
    return 0;
}

int main(int argc, char **argv)
{
    Wsa wsa;                                              /* Winsock 的初始化/清理绑在这个对象上 */
    const int port = argc > 1 ? std::atoi(argv[1]) : 8080;
    if (argc > 2 && std::string_view(argv[2]) == "bench")
        return run_bench(port, argc > 3 ? std::atoi(argv[3]) : 5000);
    return run_server(port);
}



