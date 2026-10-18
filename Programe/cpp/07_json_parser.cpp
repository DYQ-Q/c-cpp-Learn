/* ==========================================================================
 * 07_json_parser.cpp —— 递归下降 JSON 解析器
 *
 * 平时我们写 json.loads() / nlohmann::json，这一份把它拆开：
 *   - 词法：一个字符一个字符往前走（游标 i_），不建 token 数组，最省内存的写法
 *   - 语法：value = null|true|false|number|string|array|object，递归下降 30 行搞定
 *   - 数据：std::variant 表达「六种 JSON 类型」，std::visit 做分派，不用 void* 也不用 union
 *   - 错误：抛异常并带上 行:列，调用方统一 catch —— 这就是 C++ 异常的正确用法
 *   - 序列化：递归输出 + 字符串转义 + 缩进
 *   - 顺便用 std::filesystem 读写一个配置文件
 *
 * 编译：g++ -std=c++20 -Wall -Wextra cpp\07_json_parser.cpp -o out\cpp07.exe
 * 运行：out\cpp07.exe        （会在当前目录写/读 data\config.json）
 * ========================================================================== */
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

/* ---------------- JSON 的值长什么样 ---------------- */
struct Json;
using JsonArray  = std::vector<Json>;                             /* C++17 起，vector 允许装不完整类型 */
using JsonObject = std::vector<std::pair<std::string, Json>>;     /* 用 vector 不用 map：保序，且标准明确允许 */

struct Json {
    std::variant<std::nullptr_t, bool, double, std::string, JsonArray, JsonObject> v;

    Json() : v(nullptr) {}
    Json(std::nullptr_t) : v(nullptr) {}
    Json(bool b) : v(b) {}
    Json(double d) : v(d) {}
    Json(int i) : v((double)i) {}
    Json(std::string s) : v(std::move(s)) {}
    Json(const char *s) : v(std::string(s)) {}
    Json(JsonArray a) : v(std::move(a)) {}
    Json(JsonObject o) : v(std::move(o)) {}

    bool is_null()   const { return std::holds_alternative<std::nullptr_t>(v); }
    bool is_bool()   const { return std::holds_alternative<bool>(v); }
    bool is_number() const { return std::holds_alternative<double>(v); }
    bool is_string() const { return std::holds_alternative<std::string>(v); }
    bool is_array()  const { return std::holds_alternative<JsonArray>(v); }
    bool is_object() const { return std::holds_alternative<JsonObject>(v); }

    const std::string &as_string() const
    {
        if (auto *p = std::get_if<std::string>(&v)) return *p;
        throw std::runtime_error("类型错误：期望 string");
    }
    double as_number() const
    {
        if (auto *p = std::get_if<double>(&v)) return *p;
        throw std::runtime_error("类型错误：期望 number");
    }
    const JsonArray &as_array() const
    {
        if (auto *p = std::get_if<JsonArray>(&v)) return *p;
        throw std::runtime_error("类型错误：期望 array");
    }
    const JsonObject &as_object() const
    {
        if (auto *p = std::get_if<JsonObject>(&v)) return *p;
        throw std::runtime_error("类型错误：期望 object");
    }
    /* 对象里按名字找；找不到返回 nullptr，比塞默认值更能表达「可能没有」 */
    const Json *find(std::string_view key) const
    {
        for (const auto &kv : as_object())
            if (kv.first == key) return &kv.second;
        return nullptr;
    }
    std::string get_str(std::string_view key, std::string def = {}) const
    {
        const Json *p = find(key);
        return (p && p->is_string()) ? p->as_string() : std::move(def);
    }
    double get_num(std::string_view key, double def = 0) const
    {
        const Json *p = find(key);
        return (p && p->is_number()) ? p->as_number() : def;
    }
};

/* variant 的分派：std::visit + 一组重载，比 switch(type) 更贴合类型系统 */
struct TypeName {
    const char *operator()(std::nullptr_t)        const { return "null"; }
    const char *operator()(bool)                  const { return "bool"; }
    const char *operator()(double)                const { return "number"; }
    const char *operator()(const std::string &)   const { return "string"; }
    const char *operator()(const JsonArray &)     const { return "array"; }
    const char *operator()(const JsonObject &)    const { return "object"; }
};
static std::string json_type(const Json &j) { return std::visit(TypeName{}, j.v); }

/* ---------------- 解析错误：带上行、列 ---------------- */
class JsonError : public std::runtime_error {
public:
    JsonError(std::string msg, size_t line, size_t col)
        : std::runtime_error(std::format("JSON 第 {} 行第 {} 列: {}", line, col, msg)),
          line_(line), col_(col) {}
    size_t line() const { return line_; }
    size_t col()  const { return col_; }
private:
    size_t line_, col_;
};

/* ---------------- 递归下降解析器 ---------------- */
class Parser {
public:
    static constexpr int kMaxDepth = 200;      /* 防「嵌套十亿层的 JSON」把调用栈打爆 */

    Json parse(std::string_view text)
    {
        s_ = text; i_ = 0; line_ = 1; col_ = 1;
        skip_ws();
        Json j = parse_value(0);
        skip_ws();
        if (i_ != s_.size()) fail("文档结束后还有多余内容");
        return j;
    }

private:
    std::string_view s_;
    size_t i_ = 0, line_ = 1, col_ = 1;

    [[noreturn]] void fail(std::string msg) const { throw JsonError(msg, line_, col_); }

    char peek() const { return i_ < s_.size() ? s_[i_] : '\0'; }
    char advance()
    {
        if (i_ >= s_.size()) fail("内容意外结束");
        char c = s_[i_++];
        if (c == '\n') { line_++; col_ = 1; } else col_++;
        return c;
    }
    bool match(char c) { if (peek() == c) { advance(); return true; } return false; }
    void expect(char c) { if (!match(c)) fail(std::format("期望 '{}'，实际看到 '{}'", c, peek())); }
    void skip_ws()
    {
        while (i_ < s_.size()) {
            char c = s_[i_];
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') advance();
            else break;
        }
    }
    bool literal(const char *w)
    {
        if (s_.size() - i_ < std::char_traits<char>::length(w)) return false;
        if (s_.compare(i_, std::char_traits<char>::length(w), w) != 0) return false;
        i_ += std::char_traits<char>::length(w);
        col_ += std::char_traits<char>::length(w);
        return true;
    }

    Json parse_value(int depth)
    {
        if (depth > kMaxDepth) fail("嵌套太深");
        const char c = peek();
        switch (c) {
            case '{': return parse_object(depth);
            case '[': return parse_array(depth);
            case '"': return Json(parse_string());
            case 't': if (literal("true"))  return Json(true);  break;
            case 'f': if (literal("false")) return Json(false); break;
            case 'n': if (literal("null"))  return Json(nullptr); break;
            default: break;
        }
        if (c == '-' || (c >= '0' && c <= '9')) return parse_number();
        fail(std::format("这里的 '{}' 不是合法的 JSON 值开头", c));
    }

    Json parse_number()
    {
        size_t start = i_;
        if (peek() == '-') advance();
        if (!std::isdigit((unsigned char)peek())) fail("数字部分为空");
        while (std::isdigit((unsigned char)peek())) advance();
        if (peek() == '.') {
            advance();
            if (!std::isdigit((unsigned char)peek())) fail("小数点后要有数字");
            while (std::isdigit((unsigned char)peek())) advance();
        }
        if (peek() == 'e' || peek() == 'E') {
            advance();
            if (peek() == '+' || peek() == '-') advance();
            if (!std::isdigit((unsigned char)peek())) fail("指数部分要有数字");
            while (std::isdigit((unsigned char)peek())) advance();
        }
        std::string txt(s_.substr(start, i_ - start));
        try {
            size_t used = 0;
            double d = std::stod(txt, &used);
            if (used != txt.size()) fail("数字 \"" + txt + "\" 不合法");
            if (std::isinf(d)) fail("数字 \"" + txt + "\" 超出范围");
            return Json(d);
        } catch (const std::exception &) {
            fail("数字 \"" + txt + "\" 不合法");
        }
    }

    /* 把码点写成 UTF-8（JSON 的 \uXXXX 到这里就变成真正的字节了） */
    static void emit_utf8(std::string &out, uint32_t cp)
    {
        if (cp < 0x80) out += (char)cp;
        else if (cp < 0x800) {
            out += (char)(0xC0 | (cp >> 6));
            out += (char)(0x80 | (cp & 0x3F));
        } else if (cp < 0x10000) {
            out += (char)(0xE0 | (cp >> 12));
            out += (char)(0x80 | ((cp >> 6) & 0x3F));
            out += (char)(0x80 | (cp & 0x3F));
        } else {
            out += (char)(0xF0 | (cp >> 18));
            out += (char)(0x80 | ((cp >> 12) & 0x3F));
            out += (char)(0x80 | ((cp >> 6) & 0x3F));
            out += (char)(0x80 | (cp & 0x3F));
        }
    }
    int hex4()
    {
        int v = 0;
        for (int k = 0; k < 4; k++) {
            char c = advance();
            v *= 16;
            if (c >= '0' && c <= '9') v += c - '0';
            else if (c >= 'a' && c <= 'f') v += c - 'a' + 10;
            else if (c >= 'A' && c <= 'F') v += c - 'A' + 10;
            else fail("'\\u' 后面必须是 4 位十六进制");
        }
        return v;
    }

    std::string parse_string()
    {
        expect('"');
        std::string out;
        for (;;) {
            if (i_ >= s_.size()) fail("字符串没有结束引号");
            char c = advance();
            if (c == '"') break;
            if (c != '\\') {
                if ((unsigned char)c < 0x20) fail("字符串里有裸控制字符，必须转义");
                out += c;
                continue;
            }
            switch (advance()) {
                case '"':  out += '"';  break;
                case '\\': out += '\\'; break;
                case '/':  out += '/';  break;
                case 'b':  out += '\b'; break;
                case 'f':  out += '\f'; break;
                case 'n':  out += '\n'; break;
                case 'r':  out += '\r'; break;
                case 't':  out += '\t'; break;
                case 'u': {
                    int cp = hex4();
                    if (cp >= 0xD800 && cp <= 0xDBFF) {          /* 高位代理：\uD83D\uDE00 = 😀 */
                        if (i_ + 1 >= s_.size() || s_[i_] != '\\' || s_[i_ + 1] != 'u')
                            fail("高位代理后面必须跟 \\u 低位代理");
                        advance(); advance();
                        int lo = hex4();
                        if (lo < 0xDC00 || lo > 0xDFFF) fail("低位代理不合法");
                        emit_utf8(out, 0x10000u + (uint32_t)((cp - 0xD800) << 10) + (uint32_t)(lo - 0xDC00));
                    } else if (cp >= 0xDC00 && cp <= 0xDFFF) {
                        fail("出现了落单的低位代理");
                    } else {
                        emit_utf8(out, (uint32_t)cp);
                    }
                    break;
                }
                default: fail("不认识的转义字符");
            }
        }
        return out;
    }

    Json parse_array(int depth)
    {
        expect('[');
        JsonArray arr;
        skip_ws();
        if (match(']')) return Json(std::move(arr));
        for (;;) {
            skip_ws();
            arr.push_back(parse_value(depth + 1));
            skip_ws();
            if (match(',')) continue;
            expect(']');
            break;
        }
        return Json(std::move(arr));
    }

    Json parse_object(int depth)
    {
        expect('{');
        JsonObject obj;
        skip_ws();
        if (match('}')) return Json(std::move(obj));
        for (;;) {
            skip_ws();
            std::string key = parse_string();
            skip_ws();
            expect(':');
            skip_ws();
            obj.emplace_back(std::move(key), parse_value(depth + 1));
            skip_ws();
            if (match(',')) continue;
            expect('}');
            break;
        }
        return Json(std::move(obj));
    }
};

static Json json_parse(std::string_view text) { return Parser().parse(text); }

/* ---------------- 反向：把 DOM 写回文本 ---------------- */
static void escape_into(std::string &out, std::string_view s)
{
    out += '"';
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = (unsigned char)s[i];
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof buf, "\\u%04x", c);
                    out += buf;                        /* 控制字符必须转义 */
                } else out += (char)c;                 /* UTF-8 多字节原样输出，JSON 允许 */
        }
    }
    out += '"';
}

/* indent < 0 -> 压成一行；>=0 -> 每层缩进这么多空格 */
static void dump_into(std::string &out, const Json &j, int indent, int level)
{
    auto newline = [&](int l) {
        if (indent < 0) return;
        out += '\n';
        out.append((size_t)indent * (size_t)l, ' ');
    };
    if (const auto *p = std::get_if<std::nullptr_t>(&j.v)) { (void)p; out += "null"; }
    else if (const auto *p = std::get_if<bool>(&j.v))    out += *p ? "true" : "false";
    else if (const auto *p = std::get_if<double>(&j.v))  out += std::format("{}", *p);
    else if (const auto *p = std::get_if<std::string>(&j.v)) escape_into(out, *p);
    else if (const auto *p = std::get_if<JsonArray>(&j.v)) {
        out += '[';
        for (size_t i = 0; i < p->size(); i++) {
            if (i) out += ',';
            newline(level + 1);
            dump_into(out, (*p)[i], indent, level + 1);
        }
        if (!p->empty()) newline(level);
        out += ']';
    } else if (const auto *p = std::get_if<JsonObject>(&j.v)) {
        out += '{';
        for (size_t i = 0; i < p->size(); i++) {
            if (i) out += ',';
            newline(level + 1);
            escape_into(out, (*p)[i].first);
            out += indent < 0 ? ":" : ": ";
            dump_into(out, (*p)[i].second, indent, level + 1);
        }
        if (!p->empty()) newline(level);
        out += '}';
    }
}

static std::string json_dump(const Json &j, int indent = -1)
{
    std::string out;
    dump_into(out, j, indent, 0);
    return out;
}

static void sec(std::string_view t) { std::cout << "\n===== " << t << " =====\n"; }

/* ---------------- 1. 解析 + 读值：这就是「库」对外提供的接口 ---------------- */
static void demo_parse_basic()
{
    sec("1. 解析一段真实风格的配置");
    static const char *TEXT = R"JSON({
  "name": "LibraryLearn",
  "version": 2,
  "published": true,
  "license": null,
  "authors": ["张三", "Li Lei"],
  "server": { "host": "127.0.0.1", "port": 8080, "tags": ["a", "b"] },
  "emoji": "\u2764 \uD83D\uDE00",
  "weird": "tab\there\nand \"quotes\""
})JSON";

    Json root = json_parse(TEXT);
    std::cout << std::format("  name      = {} (类型 {})\n", root.get_str("name"), json_type(*root.find("name")));
    std::cout << std::format("  version   = {:.0f}\n", root.get_num("version"));
    std::cout << std::format("  published = {}，license = {}（null 也是一种值）\n",
                             std::get<bool>(root.find("published")->v),
                             root.find("license")->is_null() ? "null" : "?");
    std::cout << std::format("  不存在的键 = \"{}\"（给了默认值，不会崩）\n", root.get_str("nope", "<默认>"));

    const Json &authors = *root.find("authors");
    std::cout << "  authors   = [";
    for (const Json &a : authors.as_array()) std::cout << " " << a.as_string();
    std::cout << " ]  共 " << authors.as_array().size() << " 个\n";

    const Json *server = root.find("server");
    std::cout << std::format("  嵌套对象：server.host = {}，server.port = {:.0f}，tags 有 {} 个\n",
                             server->get_str("host"), server->get_num("port"),
                             server->find("tags")->as_array().size());
    std::cout << std::format("  \\u 转义解出来了：{} <- 应该是「❤  😀」\n", root.get_str("emoji"));
    std::cout << std::format("  字符串里的转义：{} <- 里面真的有 TAB 和引号\n", root.get_str("weird"));
}

/* ---------------- 2. 错误处理：坏 JSON 必须报出「第几行第几列」 ---------------- */
static void demo_errors()
{
    sec("2. 出错时报出位置（这就是 C++ 异常 + std::filesystem 之外最实用的调试信息）");
    const std::vector<std::pair<std::string, std::string>> bad = {
        {"少了一个逗号",        "{\n  \"a\": 1\n  \"b\": 2\n}"},
        {"多余的尾逗号",        "[1, 2, 3,]"},
        {"键没用引号",          "{name: \"x\"}"},
        {"裸 NaN（JS 常见错误）", "[1, NaN, 3]"},
        {"字符串没闭合",        "{\"s\": \"abc"},
        {"数字后面跟了字母",    "{\"n\": 12abc}"},
        {"括号不匹配",          "{\"a\": [1, 2}}"},
    };
    for (const auto &[title, text] : bad) {              /* 结构化绑定，一次拿到 pair 的两个成员 */
        try {
            json_parse(text);
            std::cout << std::format("  {:<22} 竟然解析通过了？！\n", title);
        } catch (const JsonError &e) {
            std::cout << std::format("  {:<22} {}\n", title, e.what());
        }
    }
    std::cout << "  注意：所有错误都用异常传出来，解析函数里没有一句 return code 的检查。\n";

    sec("3. 恶意输入：200 层嵌套会把栈打爆，所以解析器有深度上限");
    std::string deep(600, '[');
    try {
        json_parse(deep);
        std::cout << "  没挡住（不该发生）\n";
    } catch (const JsonError &e) {
        std::cout << std::format("  挡住了：{}（栈只有 1MB，递归必须设限）\n", e.what());
    }
}

/* ---------------- 4. 反向：用代码造 DOM，再 dump 出来，然后读回来验证 ---------------- */
static void demo_roundtrip()
{
    sec("4. 序列化 + 往返一致");
    JsonObject hero;
    hero.emplace_back("name", Json("暗夜猎手"));
    hero.emplace_back("level", Json(37));
    hero.emplace_back("alive", Json(true));
    hero.emplace_back("tags", JsonArray{Json("fire"), Json("crit+15%")});

    JsonObject meta;
    meta.emplace_back("hp", Json(1234.5));
    meta.emplace_back("pos", JsonArray{Json(1.5), Json(-2.25)});
    hero.emplace_back("meta", Json(std::move(meta)));

    Json root(std::move(hero));
    std::string compact = json_dump(root);                       /* 一行，给机器看 */
    std::string pretty  = json_dump(root, 2);                    /* 缩进 2，给人看 */
    std::cout << "  compact: " << compact << "\n";
    std::cout << "  pretty:\n" << pretty << "\n";

    Json again = json_parse(compact);                              /* 读回来 */
    std::cout << std::format("  往返一致？{}（level={:.0f}，tags[1]={}）\n",
                             json_dump(again) == compact,
                             again.get_num("level"),
                             again.find("tags")->as_array()[1].as_string());
    std::cout << "  能把「+」放进名字里还不破坏格式，全靠 escape_into 的转义。\n";
}

/* ---------------- 5. 配合 std::filesystem：读写磁盘上的配置文件 ---------------- */
static void demo_file()
{
    sec("5. std::filesystem + fstream：把 JSON 落盘再读回来");
    std::error_code ec;
    std::filesystem::path dir = "data";
    std::filesystem::create_directories(dir, ec);          /* 不存在的目录就地创建；带 ec 就不会抛 */
    std::filesystem::path file = dir / "config.json";

    std::ofstream ofs(file, std::ios::binary);
    ofs << json_dump(Json(JsonObject{
            {"window", Json(JsonObject{{"w", Json(1280)}, {"h", Json(720)}, {"fullscreen", Json(false)}})},
            {"volume", Json(0.35)},
            {"keys",   JsonArray{Json("W"), Json("A"), Json("S"), Json("D")}},
        }), 2)
        << "\n";
    ofs.close();

    std::ifstream ifs(file, std::ios::binary);
    std::string text((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    Json cfg = json_parse(text);
    const Json *win = cfg.find("window");
    std::cout << std::format("  {} 大小 {} 字节\n", std::filesystem::absolute(file).string(),
                             std::filesystem::file_size(file));
    std::cout << std::format("  读回来：window = {:.0f}x{:.0f}，fullscreen={}，volume={:.2f}，键位 {} 个\n",
                             win->get_num("w"), win->get_num("h"),
                             std::get<bool>(win->find("fullscreen")->v), cfg.get_num("volume"),
                             cfg.find("keys")->as_array().size());
    std::cout << std::format("  目录里现在有什么：");
    for (const auto &e : std::filesystem::directory_iterator(dir))
        std::cout << e.path().filename().string() << " ";
    std::cout << "（是否存在 data 目录 = " << std::boolalpha << std::filesystem::exists(dir) << "）\n";
}

/* ---------------- 6. 顺便量一下这个玩具解析器有多快 ---------------- */
static void demo_bench()
{
    sec("6. 基准：解析 20 万次同一个对象");
    std::string doc;
    doc += "{\"items\":[";
    for (int i = 0; i < 50; i++) doc += std::format("{{\"id\":{},\"n\":\"item{}\",\"ok\":true}},", i, i);
    doc.pop_back();
    doc += "]}";
    std::cout << std::format("  文档长度 {} 字节\n", doc.size());

    auto t0 = std::chrono::steady_clock::now();
    long items = 0;
    const int N = 200000;
    for (int i = 0; i < N; i++) {
        Json j = json_parse(doc);
        items += (long)j.find("items")->as_array().size();
    }
    double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    std::cout << std::format("  {} 次解析共 {:.0f} ms，即每秒 {:.0f} 万个文档（每次 {} 个元素）\n",
                             N, ms, N / ms / 1000.0, items / N);
    std::cout << "  手写的够用；真要处理大文件时，业界做法是不建 DOM，直接流式扫（SAX 风格）。\n";
}

int main()
{
    std::cout << "JSON 只有 6 种值：null / bool(true|false) / number / string / array / object。\n"
                 "C++ 里正好用一个 std::variant 就能表达，不需要 void*，也不需要继承体系。\n";
    demo_parse_basic();
    demo_errors();
    demo_roundtrip();
    demo_file();
    demo_bench();
    std::cout << "\n把这份 Parser 里的 Json 换成你的结构体，就是一个真的配置文件系统了。\n";
    return 0;
}


