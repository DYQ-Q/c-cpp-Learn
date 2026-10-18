/* ==========================================================================
 * 08_inventory_game.cpp —— 小型 RPG：把这一套东西全串起来
 *
 * 这一份不教新概念，而是把前面几份拼成一个「像样的小程序」：
 *   - 继承 + 纯虚函数：物品背包里存的是 std::unique_ptr<Item>，实际装的是派生类
 *   - virtual clone()：多态对象的「虚拟拷贝」，容器持所有权时的必备技巧
 *   - dynamic_cast：拿着基类指针按真实类型算属性（也想清楚什么时候该改用 variant）
 *   - 工厂表：std::unordered_map<std::string, std::function<...>>，存档只写类型名
 *   - 随机：std::mt19937 + 加权抽样 + uniform_int_distribution 做战斗与掉落
 *   - 存档：文本文件一行一个物品，读档时重建整棵对象树
 *   - 命令表：把「指令 -> lambda」放进 map，这就是游戏控制台的通用写法
 *
 * 编译：g++ -std=c++20 -Wall -Wextra cpp\08_inventory_game.cpp -o out\cpp08.exe
 * 运行：out\cpp08.exe              跑一遍自动演示（战斗 + 掉落 + 存读档）
 *      ：out\cpp08.exe interactive 自己敲命令：inv / sort / equip / fight / shop / save / load / help / quit
 * ========================================================================== */
#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

static void sec(std::string_view t) { std::cout << "\n===== " << t << " =====\n"; }

/* ---------------- 稀有度：enum class 不会跟普通整数混用 ---------------- */
enum class Rarity : int { Common = 0, Uncommon, Rare, Epic, Legendary };

static const char *rarity_name(Rarity r)
{
    switch (r) {
        case Rarity::Common:    return "普通";
        case Rarity::Uncommon:  return "优秀";
        case Rarity::Rare:      return "稀有";
        case Rarity::Epic:      return "史诗";
        case Rarity::Legendary: return "传说";
    }
    return "?";
}
/* 掉落权重：普通 60%、优秀 25%、稀有 10%、史诗 4%、传说 1% */
static const std::pair<Rarity, int> DROP_WEIGHTS[] = {
    {Rarity::Common, 6000}, {Rarity::Uncommon, 2500}, {Rarity::Rare, 1000},
    {Rarity::Epic, 400},    {Rarity::Legendary, 100},
};

/* ---------------- Item：多态基类 ---------------- */
class Item {
public:
    Item(std::string name, Rarity rarity, int value)
        : name_(std::move(name)), rarity_(rarity), value_(value) {}
    virtual ~Item() = default;                       /* 基类析构必须是 virtual，否则 delete 基类指针是 UB */

    const std::string &name() const { return name_; }
    Rarity rarity() const { return rarity_; }
    int value()     const { return value_; }

    virtual std::string kind() const = 0;             /* 纯虚：派生类必须实现，Item 因此不能实例化 */
    virtual std::unique_ptr<Item> clone() const = 0;  /* 「虚拟拷贝」：只有派生类知道自己是谁 */
    virtual std::string describe() const
    {
        return std::format("{}[{}, {}金]", name_, rarity_name(rarity_), value_);
    }

/* ---------------- 三类派生物品 ---------------- */
class Consumable : public Item {
public:
    Consumable(std::string n, Rarity r, int v, int heal, int uses = 1)
        : Item(std::move(n), r, v), heal_(heal), uses_(uses) {}
    std::string kind() const override { return "consumable"; }
    std::unique_ptr<Item> clone() const override { return std::make_unique<Consumable>(*this); }
    std::string describe() const override { return Item::describe() + std::format(" 回{}血x{}次", heal_, uses_); }
    int heal() const { return heal_; }
    int uses() const { return uses_; }
    int &uses() { return uses_; }                 /* 非 const 重载：消耗品要用掉次数 */
private:
    int heal_, uses_;
};

class Weapon : public Item {
public:
    Weapon(std::string n, Rarity r, int v, int atk, int crit)
        : Item(std::move(n), r, v), atk_(atk), crit_(crit) {}
    std::string kind() const override { return "weapon"; }
    std::unique_ptr<Item> clone() const override { return std::make_unique<Weapon>(*this); }
    std::string describe() const override { return Item::describe() + std::format(" 攻+{} 暴击{}%", atk_, crit_); }
    int atk()  const { return atk_; }
    int crit() const { return crit_; }
private:
    int atk_, crit_;
};

class Armor : public Item {
public:
    Armor(std::string n, Rarity r, int v, int def)
        : Item(std::move(n), r, v), def_(def) {}
    std::string kind() const override { return "armor"; }
    std::unique_ptr<Item> clone() const override { return std::make_unique<Armor>(*this); }
    std::string describe() const override { return Item::describe() + std::format(" 防+{}", def_); }
    int def() const { return def_; }
private:
    int def_;
};

/* ---------------- 工厂表：字符串 -> 怎么造这个对象 ---------------- */
/* 好处：以后加一类物品只要在表里加一行，读档代码一个字都不用改（开闭原则） */
using Factory = std::function<std::unique_ptr<Item>(std::istringstream &)>;

static const std::unordered_map<std::string, Factory> &FACTORY()
{
    /* 存档一行的格式： type|name|rarity|value|参数… */
    static const std::unordered_map<std::string, Factory> table = {
        {"consumable", [](std::istringstream &f) -> std::unique_ptr<Item> {
            std::string n; int r = 0, v = 0, h = 0, u = 1;
            std::getline(f, n, '|'); f >> r >> v >> h >> u;
            return std::make_unique<Consumable>(n, (Rarity)r, v, h, u);
        }},
        {"weapon", [](std::istringstream &f) -> std::unique_ptr<Item> {
            std::string n; int r = 0, v = 0, a = 0, c = 0;
            std::getline(f, n, '|'); f >> r >> v >> a >> c;
            return std::make_unique<Weapon>(n, (Rarity)r, v, a, c);
        }},
        {"armor", [](std::istringstream &f) -> std::unique_ptr<Item> {
            std::string n; int r = 0, v = 0, d = 0;
            std::getline(f, n, '|'); f >> r >> v >> d;
            return std::make_unique<Armor>(n, (Rarity)r, v, d);
        }},
    };
    return table;
}

/* 写存档：基类指针 -> 一行文本。这里必须用 dynamic_cast 判断真实类型 */
static std::string save_line(const Item &it)
{
    std::string out = it.kind() + "|" + it.name() + "|" + std::to_string((int)it.rarity()) +
                      "|" + std::to_string(it.value());
    if (const auto *w = dynamic_cast<const Weapon *>(&it))     out += "|" + std::to_string(w->atk()) + "|" + std::to_string(w->crit());
    else if (const auto *a = dynamic_cast<const Armor *>(&it)) out += "|" + std::to_string(a->def());
    else if (const auto *c = dynamic_cast<const Consumable *>(&it))
        out += "|" + std::to_string(c->heal()) + "|" + std::to_string(c->uses());
    return out;
}

/* ---------------- 掉落表：每种物品在 5 个稀有度上有不同名字，数值随稀有度放大 ---------------- */
struct ItemTemplate {
    const char *type;
    const char *names[5];                  /* 顺序对应 Rarity::Common..Legendary */
};
static const ItemTemplate TEMPLATES[] = {
    {"weapon",     {"生锈短剑", "精钢长剑", "符文战斧", "熔火之刃", "屠龙·断水"}},
    {"weapon",     {"练习木弓", "猎手长弓", "破甲弩",   "风暴之弓", "落日·追星"}},
    {"armor",      {"亚麻布衣", "硬化皮甲", "锁子甲",   "秘银胸铠", "不灭·星辉"}},
    {"consumable", {"小草药",   "大草药",   "精英药剂", "秘传灵药", "不死神药"}},
};

static Rarity roll_rarity(std::mt19937 &rng)
{
    static const int total = [] {          /* 立即调用的 lambda：只算一次的常量 */
        int s = 0;
        for (auto &pw : DROP_WEIGHTS) s += pw.second;
        return s;
    }();
    std::uniform_int_distribution<int> d(1, total);
    int x = d(rng);
    for (auto &[r, w] : DROP_WEIGHTS) {    /* 结构化绑定：一次拿到 pair 的两个成员 */
        if (x <= w) return r;
        x -= w;
    }
    return Rarity::Common;
}

static std::unique_ptr<Item> roll_drop(std::mt19937 &rng)
{
    const Rarity rar = roll_rarity(rng);
    const int  tier = (int)rar;
    const ItemTemplate &tpl = TEMPLATES[std::uniform_int_distribution<size_t>(0, std::size(TEMPLATES) - 1)(rng)];
    const int value = (tier + 1) * (tier + 1) * 12;

    if (std::string_view(tpl.type) == "weapon")
        return std::make_unique<Weapon>(tpl.names[tier], rar, value, 6 + tier * tier * 4, 3 + tier * 5);
    if (std::string_view(tpl.type) == "armor")
        return std::make_unique<Armor>(tpl.names[tier], rar, value, 3 + tier * tier * 2);
    return std::make_unique<Consumable>(tpl.names[tier], rar, value, 15 + tier * 25, 1 + tier);
}
/* ---------------- 玩家：背包就是一个「unique_ptr 装的多态对象」容器 ---------------- */
class Player {
public:
    explicit Player(std::string n) : name_(std::move(n)) { hp_ = max_hp(); }

    void add(std::unique_ptr<Item> it) { inv_.push_back(std::move(it)); }
    void add_copy(const Item &it)      { inv_.push_back(it.clone()); }  /* 虚拟拷贝：深复制一份 */
    size_t bag_size() const            { return inv_.size(); }
    int  gold() const { return gold_; }
    void earn(int g)  { gold_ += g; }
    bool spend(int g) { if (g > gold_) return false; gold_ -= g; return true; }
    int  hp()   const { return hp_; }
    bool alive() const { return hp_ > 0; }
    void hurt(int d)   { hp_ = std::max(0, hp_ - d); }
    void full_heal()   { hp_ = max_hp(); }
    std::string name() const { return name_; }

    /* 从背包穿到槽位；kind() 决定槽位，换下来的装备自动回背包 —— 全程没有 new/delete */
    bool equip(const std::string &name)
    {
        auto it = std::find_if(inv_.begin(), inv_.end(),
                               [&](const std::unique_ptr<Item> &p) { return p->name() == name; });
        if (it == inv_.end()) return false;
        std::string slot = (*it)->kind();
        if (slot == "consumable") return false;                        /* 消耗品不能穿 */
        std::unique_ptr<Item> moving = std::move(*it);
        inv_.erase(it);
        if (equip_[slot]) inv_.push_back(std::move(equip_[slot]));      /* 旧的回背包 */
        equip_[slot] = std::move(moving);
        hp_ = std::min(hp_, max_hp());
        return true;
    }

    /* 拿着基类指针问「你到底是不是武器」，只能 dynamic_cast —— 这就是多态的代价 */
    int attack() const
    {
        int s = 8;
        for (const auto &kv : equip_)
            if (const auto *w = dynamic_cast<const Weapon *>(kv.second.get())) s += w->atk();
        return s;
    }
    int crit() const
    {
        int s = 3;
        for (const auto &kv : equip_)
            if (const auto *w = dynamic_cast<const Weapon *>(kv.second.get())) s += w->crit();
        return s;
    }
    int defense() const
    {
        int s = 0;
        for (const auto &kv : equip_)
            if (const auto *a = dynamic_cast<const Armor *>(kv.second.get())) s += a->def();
        return s;
    }
    int max_hp() const { return 100 + 10 * (int)equip_.size(); }

    void print_bag() const
    {
        for (const auto &p : inv_)   std::cout << "    " << p->describe() << "\n";
        for (const auto &[slot, p] : equip_)
            std::cout << "    [已穿:" << slot << "] " << p->describe() << "\n";
    }
    void sort_bag()                                                    /* 稀有度高的排前面 */
    {
        std::sort(inv_.begin(), inv_.end(),
                  [](const std::unique_ptr<Item> &a, const std::unique_ptr<Item> &b) {
                      return a->rarity() != b->rarity() ? a->rarity() > b->rarity()
                                                        : a->value() > b->value();
                  });
    }
    /* 喝一口药：用完最后一次就把对象从背包里删掉，unique_ptr 自动析构，不需要手动 delete */
    bool drink()
    {
        for (auto it = inv_.begin(); it != inv_.end(); ++it) {
            auto *c = dynamic_cast<Consumable *>(it->get());
            if (!c) continue;
            hp_ = std::min(max_hp(), hp_ + c->heal());
            if (c->uses() > 1) --c->uses();
            else inv_.erase(it);
            return true;
        }
        return false;
    }

    /* 存档 / 读档 */
    bool save(const std::string &path) const
    {
        std::ofstream f(path, std::ios::binary | std::ios::trunc);
        if (!f) return false;
        f << "player|" << name_ << "|" << gold_ << "|" << hp_ << "\n";
        for (const auto &p : inv_)        f << "bag|"  << save_line(*p) << "\n";
        for (const auto &kv : equip_) f << "worn|" << save_line(*kv.second) << "\n"; /* 槽位就是 kind()，不用另存 */
        return true;
    }
    size_t load(const std::string &path)
    {
        std::ifstream f(path, std::ios::binary);
        if (!f) return 0;
        inv_.clear();
        equip_.clear();
        std::string line;
        while (std::getline(f, line)) {
            std::istringstream ss(line);
            std::string tag;
            std::getline(ss, tag, '|');                       /* player / bag / worn */
            if (tag == "player") {
                std::getline(ss, name_, '|');
                ss >> gold_ >> hp_;
                continue;
            }
            std::string type;
            std::getline(ss, type, '|');                      /* 物品类型：weapon / armor / consumable */
            std::string rest;
            std::getline(ss, rest);                           /* name|rarity|value|参数… */
            auto fac = FACTORY().find(type);
            if (fac == FACTORY().end()) { std::cout << "  存档里有未知类型 " << type << "，已跳过\n"; continue; }
            std::unique_ptr<Item> it = fac->second(make_stream(type, rest));
            if (tag == "worn") equip_[it->kind()] = std::move(it);
            else inv_.push_back(std::move(it));
        }
        return inv_.size() + equip_.size();
    }

private:
    /* 工厂约定吃的是「type|name|rarity|value|…」整行，这里把已读掉的 type 拼回去 */
    static std::istringstream make_stream(const std::string &type, const std::string &rest)
    {
        return std::istringstream(type + "|" + rest);
    }

    std::string name_;
    int gold_ = 100, hp_ = 100;
    std::vector<std::unique_ptr<Item>> inv_;
    std::map<std::string, std::unique_ptr<Item>> equip_;               /* 槽位 -> 装备 */
};

/* ---------------- 怪物与战斗：全是标准库随机数引擎，不需要 rand() ---------------- */
struct Monster {
    std::string name;
    int hp = 30, atk = 8, def = 1, gold = 20;
};

static Monster spawn_monster(std::mt19937 &rng, int level)
{
    static const char *NAMES[] = {"哥布林", "野猪", "骷髅兵", "石像鬼", "暗影狼", "洞穴蝠", "双头魔"};
    std::uniform_int_distribution<int> pick(0, (int)std::size(NAMES) - 1);
    std::uniform_int_distribution<int> jitter(0, 9);
    Monster m;
    m.name = NAMES[pick(rng)];
    m.hp   = 26 + 14 * level + jitter(rng);
    m.atk  = 6 + 3 * level + jitter(rng) / 3;
    m.def  = level / 2 + jitter(rng) / 4;
    m.gold = 12 * level + jitter(rng) * 2;
    return m;
}

struct BattleResult {
    bool win = false;
    int  rounds = 0, dealt = 0, taken = 0, crits = 0, potions = 0;
};

/* 战斗全程只是「属性 + 随机数」，把逻辑和数值分开写，方便以后接 UI */
static BattleResult battle(Player &p, Monster m, std::mt19937 &rng, bool verbose)
{
    BattleResult r;
    std::uniform_int_distribution<int> jitter(-3, 3);
    std::uniform_int_distribution<int> percent(0, 99);

    if (verbose) std::cout << std::format("    遭遇 {}（HP{} 攻{} 防{}）\n", m.name, m.hp, m.atk, m.def);
    for (int round = 1; round <= 80 && p.alive() && m.hp > 0; round++) {
        r.rounds = round;

        const bool crit = percent(rng) < p.crit();
        int dmg = std::max(1, p.attack() + jitter(rng) - m.def);
        if (crit) { dmg *= 2; ++r.crits; }
        m.hp -= dmg;
        r.dealt += dmg;
        if (m.hp <= 0) { r.win = true; break; }

        int mdmg = std::max(1, m.atk + jitter(rng) - p.defense());
        p.hurt(mdmg);
        r.taken += mdmg;

        if (!p.alive()) break;
        if (p.hp() < p.max_hp() / 3 && p.drink()) {          /* 血少于一成就喝药：喝药也是操作 */
            ++r.potions;
            if (verbose) std::cout << "      喝了一瓶药\n";
        }
    }
    if (verbose)
        std::cout << std::format("      {}：{} 回合，造成 {} 伤害，承受 {} 伤害，暴击 {} 次，喝药 {} 次\n",
                                 r.win ? "胜利" : "倒地", r.rounds, r.dealt, r.taken, r.crits, r.potions);
    return r;
}

/* ---------------- 直方图：把 map<Rarity,int> 打成好看的表 ---------------- */
static void print_histogram(const std::map<Rarity, int> &hist, int total)
{
    for (const auto &[rar, cnt] : hist) {
        const double pct = total ? 100.0 * cnt / total : 0.0;
        std::cout << std::format("    {:<6} {:>6} 次  {:5.2f}%  {}\n", rarity_name(rar), cnt, pct,
                                 std::string(std::max<size_t>(1, (size_t)(pct * 2)), '#'));
    }
}

/* ---------------- 开局角色 ---------------- */
static Player make_starter_character()
{
    Player p("冒险者");
    p.add(std::make_unique<Weapon>("生锈短剑", Rarity::Common, 10, 6, 3));
    p.add(std::make_unique<Weapon>("练习木弓", Rarity::Common, 12, 5, 2));
    p.add(std::make_unique<Armor>("亚麻布衣", Rarity::Common, 8, 2));
    p.add(std::make_unique<Consumable>("小草药", Rarity::Common, 5, 20, 3));
    p.add(std::make_unique<Consumable>("大草药", Rarity::Uncommon, 14, 45, 2));
    return p;                                         /* Player 只能移动不能拷贝：unique_ptr 的拷贝构造是 delete 的 */
}

static void demo_basics(Player &p)
{
    sec("1. 一个 vector<unique_ptr<Item>> 里装着三种不同的类");
    std::cout << std::format("  {} 的背包（{} 格）：\n", p.name(), p.bag_size());
    p.print_bag();
    std::cout << std::format("  穿之前：攻{} 暴{}% 防{} HP{}/{}\n", p.attack(), p.crit(), p.defense(), p.hp(), p.max_hp());
    p.equip("生锈短剑");
    p.equip("亚麻布衣");
    p.equip("小草药");                                 /* 消耗品：equip 返回 false，不会崩 */
    std::cout << std::format("  穿之后：攻{} 暴{}% 防{} HP{}/{}   （属性由 dynamic_cast 从装备里累加出来）\n",
                             p.attack(), p.crit(), p.defense(), p.hp(), p.max_hp());

    {   /* 虚拟拷贝：往背包里塞一份「克隆」，原对象当场销毁也不影响它 */
        Weapon rare("限量测试剑", Rarity::Epic, 500, 30, 25);
        p.add_copy(rare);                               /* 内部调 rare.clone()，得到独立的一份 */
        std::cout << std::format("  add_copy() -> virtual clone()：{} 进了背包，出了作用域原对象自动析构\n",
                                 rare.name());
    }

    sec("2. 排序：unique_ptr 容器照样能 std::sort");
    p.sort_bag();
    std::cout << "  按稀有度降序：\n";
    p.print_bag();
}

static void demo_battle(Player &p, std::mt19937 &rng)
{
    sec("3. 打 8 场：等级递增，自动喝药，看属性和概率怎么起作用");
    int wins = 0, rounds_total = 0, potions = 0;
    std::map<Rarity, int> drops;
    for (int level = 1; level <= 8; level++) {
        Monster m = spawn_monster(rng, level);
        std::cout << std::format("  第 {} 场：\n", level);
        BattleResult r = battle(p, m, rng, true);
        wins += r.win;
        rounds_total += r.rounds;
        potions += r.potions;
        if (r.win) {
            p.earn(m.gold);
            std::cout << std::format("      收获 {} 金\n", m.gold);
            std::unique_ptr<Item> it = roll_drop(rng);
            drops[it->rarity()]++;
            std::cout << std::format("      掉落：{}\n", it->describe());
            p.add(std::move(it));                       /* move 进背包：所有权转移，没有 new 也没有 delete */
        } else {
            std::cout << "      被打倒了，回城满血复活\n";
            p.full_heal();
        }
    }
    std::cout << std::format("  战绩：{} / 8 胜，平均 {:.1f} 回合，喝了 {} 瓶药，现有金币 {}\n",
                             wins, rounds_total / 8.0, potions, p.gold());
    std::cout << "  这几场的掉落稀有度：\n";
    print_histogram(drops, wins);
    std::cout << "  背包现在有 " << p.bag_size() << " 格没穿出去的东西 —— 全是 unique_ptr 在管，没有一个裸指针。\n";
}

static void demo_law_of_large_numbers(std::mt19937 &rng)
{
    sec("4. 掉落概率是不是真的按权重来？扔 20 万次");
    std::map<Rarity, int> hist;
    const int N = 200000;
    for (int i = 0; i < N; i++) hist[roll_rarity(rng)]++;
    std::cout << std::format("  目标权重：普通60% 优秀25% 稀有10% 史诗4% 传说1%\n  实测：\n");
    print_histogram(hist, N);
    std::cout << "  这就是 std::mt19937 + 加权抽样：想要「十连必出金」只需在这里加一条保底计数。\n";
}

static void demo_save_load(Player &p)
{
    sec("5. 存档 / 读档：多态对象写进文本，再从工厂表里长回来");
    const std::string path = "data/savegame.txt";
    std::filesystem::create_directories("data");
    if (!p.save(path)) { std::cout << "  存盘失败\n"; return; }
    std::cout << std::format("  存到 {}（{} 字节），背包 {} 格\n", path,
                             std::filesystem::file_size(path), p.bag_size());
    {
        std::ifstream f(path);
        std::string line;
        for (int i = 0; i < 4 && std::getline(f, line); i++) std::cout << "    " << line << "\n";
    }
    Player q("读档出来的角色");
    size_t n = q.load(path);
    std::cout << std::format("  读回来 {} 件物品，属性：攻{} 防{} 金币{} —— 和原角色一致就说明工厂表没问题\n",
                             n, q.attack(), q.defense(), q.gold());
    q.print_bag();
    std::cout << "  存档里没有一行代码认识「武器」这个类：type 字符串 -> 工厂表 -> 造对象，加新类型不用改读档逻辑。\n";
}

/* ---------------- 控制台：命令 -> lambda，这就是所有游戏 REPL 的写法 ---------------- */
static void run_repl(Player &p, std::mt19937 &rng)
{
    sec("6. 轮到你了：输入 help 看命令，quit 退出");
    const std::string path = "data/savegame.txt";

    std::map<std::string, std::function<void(const std::string &)>> cmds;
    cmds["help"] = [](const std::string &) {
        std::cout << "  inv | sort | equip <名字> | drink | fight | shop | buy | save | load | stats | quit\n";
    };
    cmds["inv"] = [&](const std::string &) { std::cout << "  背包：\n"; p.print_bag(); };
    cmds["sort"] = [&](const std::string &) { p.sort_bag(); std::cout << "  已按稀有度排序\n"; };
    cmds["stats"] = [&](const std::string &) {
        std::cout << std::format("  {}  HP{}/{}  攻{} 暴{}% 防{}  金币{}  背包{}格\n",
                                 p.name(), p.hp(), p.max_hp(), p.attack(), p.crit(), p.defense(),
                                 p.gold(), p.bag_size());
    };
    cmds["equip"] = [&](const std::string &arg) {
        std::cout << (p.equip(arg) ? "  穿上了\n" : "  穿不上（名字要完全一致，消耗品不能穿）\n");
    };
    cmds["drink"] = [&](const std::string &) {
        std::cout << (p.drink() ? std::format("  喝完了，HP {}/{}\n", p.hp(), p.max_hp()) : "  背包里没有能喝的东西\n");
    };
    cmds["fight"] = [&](const std::string &) {
        Monster m = spawn_monster(rng, 4);
        BattleResult r = battle(p, m, rng, true);
        if (r.win) {
            p.earn(m.gold);
            std::unique_ptr<Item> it = roll_drop(rng);
            std::cout << std::format("      缴获 {} 金，掉落 {}\n", m.gold, it->describe());
            p.add(std::move(it));
        } else {
            std::cout << "      你倒下了，被村民抬回城里（复活满血）\n";
            p.full_heal();
        }
    };
    cmds["shop"] = [&](const std::string &) {
        std::unique_ptr<Item> it = roll_drop(rng);
        const int price = it->value() * 2;
        std::cout << std::format("  店主掏出：{}  要 {} 金（你身上 {} 金）\n", it->describe(), price, p.gold());
        if (p.spend(price)) {
            std::cout << "  成交：所有权 move 进背包\n";
            p.add(std::move(it));
        } else {
            std::cout << "  买不起。这个 unique_ptr 出了作用域就自动释放，一点内存都不会漏。\n";
        }
    };
    cmds["save"] = [&](const std::string &) {
        std::filesystem::create_directories("data");
        std::cout << (p.save(path) ? "  已保存\n" : "  保存失败\n");
    };
    cmds["load"] = [&](const std::string &) {
        std::cout << std::format("  读回 {} 件物品\n", p.load(path));
    };
    cmds["quit"] = [](const std::string &) { std::cout << "  再见\n"; };

    std::string line;
    while (std::cout << "> " && std::getline(std::cin, line)) {
        std::istringstream ss(line);
        std::string cmd, arg;
        ss >> cmd;
        std::getline(ss >> std::ws, arg);                      /* 剩下的整行都是参数 */
        if (cmd.empty()) continue;
        auto it = cmds.find(cmd);
        if (it == cmds.end()) { std::cout << "  没这条命令，输入 help 看看\n"; continue; }
        it->second(arg);
        if (cmd == "quit") break;
    }
}

int main(int argc, char **argv)
{
    std::mt19937 rng((unsigned)std::chrono::steady_clock::now().time_since_epoch().count());

    std::cout << "一份「像个小项目」的 C++：多态容器 + 智能指针 + 随机数 + 文件存档 + 命令表。\n";
    Player p = make_starter_character();     /* Player 只能移动：vector<unique_ptr> 不可拷贝 */
    demo_basics(p);
    demo_battle(p, rng);
    demo_law_of_large_numbers(rng);
    demo_save_load(p);

    if (argc > 1 && std::string_view(argv[1]) == "interactive") {
        run_repl(p, rng);
    } else {
        std::cout << "\n想自己动手：out\\cpp08.exe interactive\n";
        std::cout << "  命令：inv / sort / equip <名字> / drink / fight / shop / save / load / stats / quit\n";
    }
    std::cout << "\n程序结束：背包里每个物品都由 unique_ptr 自动销毁 —— 这份代码里一个 delete 都没写。\n";
    return 0;
}



