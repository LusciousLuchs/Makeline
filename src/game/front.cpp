// Makeline - at the register: what the customer is saying, and the order screen.
#include "game.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace pz;
using ui::INK;

namespace {

void dialogue() {
    Store& S = G.S;
    Rectangle box{20, 612, 900, 270};
    ui::panel(box, ui::PAPER, 12, 4, true);
    if (!S.talk.active()) {
        ui::text("THE REGISTER", box.x + 24, box.y + 18, 30, INK);
        int next = S.firstInLine();
        if (next >= 0) {
            ui::text("Someone's waiting to order.", box.x + 24, box.y + 70, 24, INK);
            if (ui::button({box.x + 24, box.y + 120, 300, 64}, "TAKE THEIR ORDER", ui::RED, true, 26)) { S.beginTalk(next); sfx(SFX_CLICK); }
        } else ui::wrap("Nobody at the counter. Your open orders are on the screen. Step back (Esc) and get them made.", box.x + 24, box.y + 70, 640, 22, INK);
        return;
    }
    Customer* c = S.cust(S.talk.cust);
    std::string who = c ? c->who.name + (c->phone ? "  (on the phone)" : "  (at the counter)") : "";
    ui::text(who, box.x + 22, box.y + 12, 24, ui::DKRED);
    // the last lines that fit, newest at the bottom
    float w = 640, size = 22, bottom = box.y + box.height - 16, top = box.y + 48;
    std::vector<float> hs;
    for (auto& l : S.talk.log) hs.push_back(ui::wrap((l.you ? "You: " : "") + l.text, 0, 0, w, size, INK, false) + 6);
    float total = 0; size_t first = S.talk.log.size();
    while (first > 0 && total + hs[first - 1] <= bottom - top) { total += hs[first - 1]; first--; }
    float y = top;
    for (size_t i = first; i < S.talk.log.size(); i++) {
        auto& l = S.talk.log[i];
        bool last = i + 1 == S.talk.log.size();
        int chars = last && !l.you && S.talk.speaking ? (int)S.talk.shown : -1;
        ui::wrap((l.you ? "You: " : "") + l.text, box.x + 22, y, w, size, l.you ? ui::GREY : INK, true, chars);
        y += hs[i];
    }
    if (S.talk.log.empty()) ui::text("...", box.x + 22, top, size, ui::GREY);
    // what you can say
    float bx = box.x + 690;
    if (ui::button({bx, box.y + 56, 190, 52}, "Repeat that?", ui::YELLOW, true, 22)) { S.askRepeat(); sfx(SFX_CLICK); }
    if (ui::button({bx, box.y + 124, 190, 52}, "Read it back", ui::BLUE, true, 22)) { S.askReadBack(); sfx(SFX_CLICK); }
    ui::wrap("Asking costs their patience.", bx, box.y + 192, 190, 17, ui::GREY);
}

const char* tag(const Item& it) {
    static char b[24];
    snprintf(b, sizeof b, "%s%s", it.part == LEFT ? "L " : it.part == RIGHT ? "R " : "", it.level == LIGHT ? "light" : it.level == EXTRA ? "extra" : it.part == WHOLE ? "on" : "half");
    return b;
}

void pos() {
    Store& S = G.S;
    Rectangle scr{960, 80, 620, 802};
    ui::panel(scr, {58, 66, 78, 255}, 14, 4, true);
    Rectangle in{scr.x + 10, scr.y + 10, scr.width - 20, scr.height - 20};
    DrawRectangleRounded(in, 0.03f, 6, {232, 238, 240, 255});
    float x = in.x + 14, y = in.y + 10, w = in.width - 28;
    if (!S.talk.active()) {
        ui::text("ORDERS", x, y, 30, INK);
        y += 46;
        int shown = 0;
        for (auto& o : S.orders) {
            if (o.done) continue;
            Customer* c = S.cust(o.cust);
            int boxed = 0;
            for (int id : o.tickets) { Ticket* k = S.ticket(id); if (k && k->state == T_BOXED) boxed++; }
            Rectangle row{x, y, w, 50};
            DrawRectangleRounded(row, 0.3f, 6, o.ready ? Color{200, 232, 200, 255} : ui::PAPER);
            char b[96];
            snprintf(b, sizeof b, "#%d  %s", o.num, c ? c->who.name.c_str() : "");
            ui::text(b, row.x + 10, row.y + 4, 22, INK);
            const char* svc = o.service == DELIVERY ? "Delivery" : o.service == CARRYOUT ? "Carryout" : "Counter";
            std::string st = o.out ? "On the road" : o.ready ? (o.service == DELIVERY ? "Waiting for a driver" : (c && c->state == C_WAITING ? "READY - hand it over!" : "Ready, on the heat rack")) :
                std::to_string(boxed) + " of " + std::to_string(o.tickets.size()) + " boxed";
            ui::text(std::string(svc) + "  -  " + st, row.x + 10, row.y + 27, 18, o.ready ? ui::shade(ui::GREEN, 0.6f) : ui::GREY);
            float left = 1 - (float)((S.t - o.placed) / o.budget);
            DrawRectangleRounded({row.x + w - 130, row.y + 18, 120, 12}, 1, 4, INK);
            DrawRectangleRounded({row.x + w - 128, row.y + 20, std::max(4.f, 116 * std::clamp(left, 0.f, 1.f)), 8}, 1, 4, left > 0.5f ? ui::GREEN : left > 0.25f ? ui::YELLOW : ui::RED);
            y += 58;
            if (++shown >= 11) break;
        }
        if (!shown) ui::wrap("No orders up. Enjoy it while it lasts.", x, y, w, 22, ui::GREY);
        return;
    }
    Customer* c = S.cust(S.talk.cust);
    auto& D = S.talk.draft;
    S.talk.cur = std::clamp(S.talk.cur, 0, (int)D.size() - 1);
    Spec& P = D[S.talk.cur];
    Color OFF{214, 220, 224, 255}, ON = ui::YELLOW;
    auto row = [&](const char* label, int n, const char* const* names, int cur, float h = 36) {
        ui::text(label, x, y + 10, 17, ui::GREY);
        int hit = -1;
        float bw = (w - 84 - (n - 1) * 8) / n;
        for (int i = 0; i < n; i++) if (ui::button({x + 84 + i * (bw + 8), y, bw, h}, names[i], cur == i ? ON : OFF, true, 19)) hit = i;
        y += h + 10;
        if (hit >= 0) sfx(SFX_CLICK);
        return hit;
    };
    ui::text("NEW ORDER", x, y, 26, INK);
    // pizzas in this order
    float tx = x + 170;
    for (int i = 0; i < (int)D.size(); i++) {
        if (ui::button({tx, y - 2, 50, 36}, std::to_string(i + 1), S.talk.cur == i ? ON : OFF, true, 20)) { S.talk.cur = i; sfx(SFX_CLICK); }
        tx += 58;
    }
    if (D.size() < 4 && ui::button({tx, y - 2, 116, 36}, "+ Pizza", ui::GREEN, true, 19)) { D.push_back(Spec{}); S.talk.cur = (int)D.size() - 1; sfx(SFX_CLICK); return; }
    if (D.size() > 1 && ui::button({in.x + in.width - 110, y - 2, 96, 36}, "Remove", ui::RED, true, 18)) { D.erase(D.begin() + S.talk.cur); S.talk.cur = 0; sfx(SFX_CLICK); return; }
    y += 46;
    if (c && c->phone) {
        static const char* const SV[] = {"Carryout", "Delivery"};
        int h = row("Service", 2, SV, S.talk.draftService == DELIVERY ? 1 : 0, 32);
        if (h >= 0) S.talk.draftService = h ? DELIVERY : CARRYOUT;
    }
    static const char* const SZ[] = {"Small 10\"", "Medium 12\"", "Large 14\"", "X-Large 16\""};
    static const char* const CR[] = {"Hand Tossed", "Thin", "Pan"};
    static const char* const SA[] = {"Marinara", "Alfredo", "Garlic Parm", "BBQ"};
    static const char* const LV3[] = {"Light", "Normal", "Extra"};
    static const char* const LV4[] = {"None", "Light", "Normal", "Extra"};
    static const char* const PT[] = {"Whole", "Left half", "Right half"};
    int h;
    if ((h = row("Size", 4, SZ, P.size)) >= 0) P.size = h;
    if ((h = row("Crust", 3, CR, P.crust)) >= 0) P.crust = h;
    if ((h = row("Sauce", 4, SA, P.sauce)) >= 0) P.sauce = h;
    if ((h = row("", 3, LV3, P.sauceLevel - 1, 30)) >= 0) P.sauceLevel = h + 1;
    Item* mz = P.find(MOZZ);
    if ((h = row("Cheese", 4, LV4, mz ? mz->level : NONE, 30)) >= 0) {
        P.remove(MOZZ);
        if (h != NONE) P.items.insert(P.items.begin(), {MOZZ, WHOLE, h});
    }
    DrawLineEx({x, y - 4}, {x + w, y - 4}, 2, ui::DKSTEEL);
    y += 4;
    ui::text("Toppings go on as:", x, y - 2, 17, ui::GREY);
    y += 20;
    if ((h = row("Where", 3, PT, G.posPart, 30)) >= 0) G.posPart = h;
    if ((h = row("Amount", 3, LV3, G.posLevel - 1, 30)) >= 0) G.posLevel = h + 1;
    // the toppings: meats, vegetables, cheeses
    static const int GRID[3][6] = {{PEPPERONI, SAUSAGE, HAM, BACON, BEEF, CHICKEN}, {ONION, PEPPER, PINEAPPLE, TOMATO, MUSHROOM, OLIVE}, {MOZZ_BALLS, PROVOLONE, CHEDDAR, PARMESAN, FETA, -1}};
    float bw = (w - 16) / 3, bh = 36;
    for (int col = 0; col < 3; col++) for (int r = 0; r < 6; r++) {
        int ing = GRID[col][r];
        if (ing < 0) continue;
        Item* it = P.find(ing);
        Rectangle b{x + col * (bw + 8), y + r * (bh + 8), bw, bh};
        std::string label = info(ing).name;
        if (it) label += std::string("  [") + tag(*it) + "]";
        Color on = col == 0 ? Color{240, 150, 130, 255} : col == 1 ? Color{160, 214, 150, 255} : Color{250, 222, 140, 255};
        if (ui::button(b, label, it ? on : OFF, true, 18, it != nullptr)) {
            sfx(SFX_CLICK);
            if (it && it->part == G.posPart && it->level == G.posLevel) P.remove(ing);
            else if (it) { it->part = G.posPart; it->level = G.posLevel; }
            else P.items.push_back({ing, G.posPart, G.posLevel});
        }
    }
    float total = 0;
    for (auto& s : D) total += price(s);
    char b[48]; snprintf(b, sizeof b, "Total  $%.2f", total);
    ui::text(b, x, in.y + in.height - 48, 26, INK);
    if (ui::button({in.x + in.width - 280, in.y + in.height - 66, 266, 54}, "SEND TO MAKELINE", ui::RED, true, 24)) { S.finishOrder(); sfx(SFX_DING); G.posPart = WHOLE; G.posLevel = NORMAL; setMode(M_WALK); }
}
}  // namespace

void registerOverlay() {
    dialogue();
    pos();
    if (!G.S.talk.active() && ui::button({20, 80, 190, 46}, "Step back (Esc)", ui::CREAM, true, 19)) setMode(M_WALK);
}
