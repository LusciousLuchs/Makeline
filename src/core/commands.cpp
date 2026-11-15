// Makeline - player actions as commands (see commands.hpp).
#include "commands.hpp"
#include "json.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>

namespace pz {

std::string specsToJson(const std::vector<Spec>& v) {
    nlohmann::json out = nlohmann::json::array();
    for (auto& s : v) {
        nlohmann::json j;
        j["size"] = s.size; j["crust"] = s.crust; j["sauce"] = s.sauce; j["sauceLevel"] = s.sauceLevel;
        auto it = nlohmann::json::array();
        for (auto& i : s.items) it.push_back({i.ing, i.part, i.level});
        j["items"] = it;
        out.push_back(j);
    }
    return out.dump();
}
std::vector<Spec> specsFromJson(const std::string& text) {
    std::vector<Spec> v;
    nlohmann::json j = nlohmann::json::parse(text, nullptr, false);
    if (!j.is_array()) return v;
    for (auto& e : j) {
        if (!e.is_object() || v.size() >= 6) continue;
        Spec s;
        s.size = std::clamp(e.value("size", (int)LARGE), 0, SIZE_N - 1);
        s.crust = std::clamp(e.value("crust", (int)HAND), 0, CRUST_N - 1);
        s.sauce = std::clamp(e.value("sauce", (int)MARINARA), 0, SAUCE_N - 1);
        s.sauceLevel = std::clamp(e.value("sauceLevel", (int)NORMAL), 0, 3);
        s.items.clear();
        if (e.contains("items") && e["items"].is_array())
            for (auto& i : e["items"]) if (i.is_array() && i.size() == 3 && s.items.size() < 24)
                s.items.push_back({std::clamp(i[0].get<int>(), (int)SAUCE_N, (int)ING_N - 1), std::clamp(i[1].get<int>(), 0, 2), std::clamp(i[2].get<int>(), 0, 3)});
        v.push_back(s);
    }
    return v;
}

std::string commandToJson(const Command& c) {
    nlohmann::json j;
    j["t"] = c.type; j["p"] = c.player;
    if (c.a) j["a"] = c.a;
    if (c.b) j["b"] = c.b;
    if (c.c) j["c"] = c.c;
    if (c.x) j["x"] = c.x;
    if (c.y) j["y"] = c.y;
    if (c.z) j["z"] = c.z;
    if (c.w) j["w"] = c.w;
    if (!c.text.empty()) j["s"] = c.text;
    return j.dump();
}
bool commandFromJson(const std::string& text, Command& out) {
    nlohmann::json j = nlohmann::json::parse(text, nullptr, false);
    if (!j.is_object() || !j.contains("t") || !j["t"].is_number_integer()) return false;
    int t = j["t"].get<int>();
    if (t < 0 || t >= CMD_N) return false;
    out = Command{};
    out.type = t; out.player = j.value("p", 0);
    out.a = j.value("a", 0); out.b = j.value("b", 0); out.c = j.value("c", 0);
    out.x = j.value("x", 0.f); out.y = j.value("y", 0.f); out.z = j.value("z", 0.f); out.w = j.value("w", 0.f);
    out.text = j.value("s", std::string());
    return true;
}

bool apply(Store& s, const Command& c) {
    if (s.dayOver || c.player < 0 || c.player >= MAX_PLAYERS) return false;
    switch (c.type) {
        case CMD_TAKE_ORDER: return s.beginTalk(c.a);
        case CMD_ASK_REPEAT: if (!s.talk.active()) return false; s.askRepeat(); return true;
        case CMD_ASK_READBACK: if (!s.talk.active()) return false; s.askReadBack(); return true;
        case CMD_SET_DRAFT: {
            if (!s.talk.active()) return false;
            auto v = specsFromJson(c.text);
            if (v.empty()) return false;
            s.talk.draft = v;
            s.talk.draftService = c.a == DELIVERY ? DELIVERY : CARRYOUT;
            s.talk.cur = std::clamp(s.talk.cur, 0, (int)v.size() - 1);
            return true;
        }
        case CMD_SEND_ORDER: return s.talk.active() && s.finishOrder();
        case CMD_HAND_OUT: return s.handOut(c.a);
        case CMD_START_PIZZA:
            if (c.b < 0 || c.b >= SIZE_N || c.c < 0 || c.c >= CRUST_N) return false;
            return s.startPizza(c.a, c.b, c.c);
        case CMD_SAUCE: { Pizza* p = s.pizza(c.a); return p && !p->boxed && p->addSauce(c.b, c.x, c.y, std::clamp(c.z, 0.f, 1.f)); }
        case CMD_BIT: { Pizza* p = s.pizza(c.a); return p && !p->boxed && c.b >= SAUCE_N && c.b < ING_N && p->addBit(c.b, c.x, c.y, s.rng); }
        case CMD_REMOVE_NEAR: { Pizza* p = s.pizza(c.a); return p && !p->boxed && p->removeNear(c.x, c.y, std::clamp(c.z, 0.f, 0.5f)) > 0; }
        case CMD_TO_OVEN: return s.benchToOven();
        case CMD_TRASH_BENCH: if (s.bench < 0) return false; s.trashBench(); return true;
        case CMD_TO_CUT: return s.toCutBoard(c.a);
        case CMD_CUT: { Pizza* p = s.pizza(s.cutBoard); if (!p) return false; size_t n = p->cuts.size(); p->addCut(c.x, c.y, c.z, c.w); return p->cuts.size() > n; }
        case CMD_CLEAR_CUTS: { Pizza* p = s.pizza(s.cutBoard); if (!p) return false; p->cuts.clear(); return true; }
        case CMD_BOX: return s.boxCut();
        case CMD_TRASH_CUT: if (s.cutBoard < 0) return false; s.trashCut(); return true;
        case CMD_WRITE_UP: if (c.a < 0 || c.a >= (int)s.staff.size() || s.staff[c.a].wroteUpToday) return false; s.writeUp(c.a); return true;
        case CMD_HIRE: return s.hire(c.a);
        case CMD_FIRE: if (c.a < 0 || c.a >= (int)s.staff.size() || s.staff[c.a].out) return false; s.fireWorker(c.a); return true;
    }
    return false;
}

std::string digest(const Store& s) {
    uint64_t h = 1469598103934665603ull;
    auto mix = [&](uint64_t v) { h ^= v; h *= 1099511628211ull; };
    auto f = [&](float v) { mix((uint64_t)(int64_t)std::llround(v * 100.0)); };
    mix((uint64_t)s.day); f(s.money); mix((uint64_t)s.t * 1000); mix(s.dayOver);
    mix(s.orders.size()); mix(s.tickets.size()); mix(s.pizzas.size()); mix(s.custs.size()); mix(s.reviews.size());
    for (auto& o : s.orders) { mix(o.num); mix(o.done); mix(o.ready); mix(o.service); f(o.price); }
    for (auto& k : s.tickets) { mix(k.id); mix(k.state); mix(k.pizza + 7); }
    for (auto& p : s.pizzas) { mix(p.bits.size()); mix(p.stamps.size()); mix(p.cuts.size()); mix(p.boxed); f(p.baked); f(p.totalSauce()); }
    for (auto& r : s.reviews) { mix(r.stars); for (char ch : r.text) mix((unsigned char)ch); }
    for (auto& c : s.custs) { mix(c.id); mix(c.state); f(c.patience); }
    for (auto& w : s.staff) { mix(w.role); f(w.mood); mix(w.jobs); mix(w.mistakes); }
    char b[24];
    snprintf(b, sizeof b, "%016llx", (unsigned long long)h);
    return b;
}

}  // namespace pz
