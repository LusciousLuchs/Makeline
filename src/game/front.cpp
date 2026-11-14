// Makeline - the front: the lobby, the counter, the phone, and the order screen.
#include "game.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace pz;
using ui::INK;

namespace {
const float COUNTER_Y = 560, SCENE_W = 940;

void bubble(const std::string& s, float x, float y, float size, Color fill = ui::PAPER) {
    float w = std::min(300.f, ui::textW(s, size) + 4), h = ui::wrap(s, 0, 0, w, size, INK, false);
    Rectangle r{std::clamp(x - w / 2 - 12, 8.f, SCENE_W - w - 32), y - h - 24, w + 24, h + 16};
    ui::panel(r, fill, 12, 3, true);
    DrawTriangle({x - 8, r.y + r.height}, {x, r.y + r.height + 14}, {x + 10, r.y + r.height}, INK);
    DrawTriangle({x - 4, r.y + r.height - 2}, {x, r.y + r.height + 8}, {x + 6, r.y + r.height - 2}, fill);
    ui::wrap(s, r.x + 12, r.y + 8, w, size, INK);
}
void patienceBar(float x, float y, float w, float p) {
    DrawRectangleRounded({x - w / 2 - 2, y - 2, w + 4, 12}, 1, 4, INK);
    Color c = p > 0.5f ? ui::GREEN : p > 0.25f ? ui::YELLOW : ui::RED;
    DrawRectangleRounded({x - w / 2, y, std::max(4.f, w * std::clamp(p, 0.f, 1.f)), 8}, 1, 4, c);
}

// where people stand: the line runs back from the register, people waiting for food stand over by the window
struct Spot { float x, base, k; };
Spot lineSpot(float slot) { return {300 + slot * 104, COUNTER_Y + 6 - slot * 14, 1.0f - slot * 0.09f}; }
Spot lobbySpot(size_t i) { return {880 - (float)i * 98, COUNTER_Y - 36 - (i % 2) * 16.f, 0.72f}; }

void scene() {
    Store& S = G.S;
    // wall, window, menu board
    DrawRectangle(0, 64, (int)SCENE_W, 500, {236, 208, 160, 255});
    DrawRectangle(0, 64, (int)SCENE_W, 26, {214, 66, 50, 255});
    for (int i = 0; i < 24; i++) DrawRectangle(i * 40, 90, 20, 14, {250, 240, 220, 255});   // a checkered trim
    float dusk = std::clamp((float)(S.t / SHIFT_LEN), 0.f, 1.f);
    Rectangle win{520, 150, 380, 250};
    DrawRectangleRec({win.x - 8, win.y - 8, win.width + 16, win.height + 16}, INK);
    DrawRectangleGradientV((int)win.x, (int)win.y, (int)win.width, (int)win.height, ui::mix({150, 200, 235, 255}, {40, 40, 90, 255}, dusk), ui::mix({250, 214, 150, 255}, {220, 110, 80, 255}, dusk));
    DrawCircleV({win.x + 290, win.y + 60 + dusk * 150}, 28, ui::mix({255, 240, 170, 255}, {255, 150, 90, 255}, dusk));
    DrawRectangle((int)win.x, (int)(win.y + 200), (int)win.width, 50, {70, 70, 80, 255});   // the street
    DrawRectangle((int)(win.x + 186), (int)win.y, 8, (int)win.height, INK);
    ui::text("NEPO", win.x + 20, win.y + 16, 30, {255, 255, 255, 120});   // "OPEN", from the inside
    ui::panel({40, 124, 420, 196}, {50, 60, 56, 255}, 8, 4, true);
    ui::text("TODAY'S MENU", 250, 134, 26, ui::YELLOW, 1);
    static const char* MENU[] = {"Small 10\"   7.99", "Medium 12\"   9.99", "Large 14\"   11.99", "X-Large 16\"   13.99"};
    for (int i = 0; i < 4; i++) ui::text(MENU[i], 64, 172 + i * 30, 22, {240, 236, 220, 255});
    ui::text("Thin: M, L", 300, 180, 18, {200, 210, 190, 255});
    ui::text("Pan: M only", 300, 206, 18, {200, 210, 190, 255});
    ui::text("Toppings +1.50", 300, 240, 18, {200, 210, 190, 255});

    // people waiting for their food stand at the back
    auto lobby = S.lobby();
    int clickCust = -1;
    for (size_t i = 0; i < lobby.size(); i++) {
        Customer* c = S.cust(lobby[i]);
        Spot sp = lobbySpot(i);
        drawCustomer(c->who, sp.x, sp.base, sp.k, 1 - c->patience, 0);
    }
    // the line at the register
    auto line = S.line();
    Customer* talking = S.talk.active() ? S.cust(S.talk.cust) : nullptr;
    for (size_t i = line.size(); i-- > 0;) {
        Customer* c = S.cust(line[i]);
        float slot = (float)i + (talking && !talking->phone ? 1 : 0);
        Spot sp = lineSpot(slot);
        drawCustomer(c->who, sp.x, sp.base, sp.k, 1 - c->patience, 0);
    }
    if (talking && !talking->phone) drawCustomer(talking->who, 300, COUNTER_Y + 6, 1.0f, 1 - talking->patience, 0);
    // people storming out (or leaving happy) slide off to the door
    for (auto& c : S.custs) if (c.leaving > 0 && c.leaving < 1.6f && !c.phone)
        drawCustomer(c.who, 500 + c.leaving * 420, COUNTER_Y - 20, 0.8f, c.leftStars >= 4 ? 0.1f : 1.0f, 0);

    // the counter
    DrawRectangle(0, (int)COUNTER_Y - 4, (int)SCENE_W, 8, INK);
    DrawRectangle(0, (int)COUNTER_Y, (int)SCENE_W, 26, {222, 214, 200, 255});
    DrawRectangle(0, (int)COUNTER_Y + 26, (int)SCENE_W, 320, ui::WOOD);
    for (int i = 0; i < 8; i++) DrawRectangle(i * 124, (int)COUNTER_Y + 26, 4, 320, ui::DKWOOD);
    DrawRectangle(0, (int)COUNTER_Y + 22, (int)SCENE_W, 5, INK);
    // the register
    ui::panel({70, COUNTER_Y - 78, 130, 80}, {70, 74, 84, 255}, 8, 3, false);
    DrawRectangleRec({84, COUNTER_Y - 66, 102, 40}, {150, 220, 190, 255});
    ui::text("$", 135, COUNTER_Y - 62, 30, INK, 1);
    // the phone
    int ring = S.ringing();
    bool onPhone = talking && talking->phone;
    float px = 840, py = COUNTER_Y - 30, jit = ring >= 0 ? std::sin(ui::time * 60) * 4 : 0;
    Rectangle phone{px - 50, py - 50, 100, 82};
    ui::panel({px - 44 + jit, py - 26, 88, 54}, {60, 60, 70, 255}, 10, 3, false);
    if (!onPhone) {
        DrawRectangleRounded({px - 54 + jit, py - 46, 108, 26}, 1, 6, INK);
        DrawRectangleRounded({px - 51 + jit, py - 43, 102, 20}, 1, 6, ui::RED);
    }
    for (int i = 0; i < 9; i++) DrawCircleV({px - 22 + (i % 3) * 22 + jit, py - 10 + (i / 3) * 12}, 3.5f, {200, 200, 210, 255});
    if (ring >= 0) {
        Customer* c = S.cust(ring);
        float pulse = 0.5f + 0.5f * std::sin(ui::time * 14);
        ui::text("RING!", px - 78 - pulse * 8, py - 100 - pulse * 6, 30, ui::RED, 1);
        ui::text("RING!", px + 40 + pulse * 8, py - 124 + pulse * 6, 24, ui::RED, 1);
        patienceBar(px, py - 64, 90, c->patience);
        if (!S.talk.active() && ui::hover(phone)) {
            DrawRectangleRoundedLinesEx({phone.x - 6, phone.y - 6, phone.width + 12, phone.height + 12}, 0.3f, 6, 3, ui::YELLOW);
            if (ui::pressed && !ui::clickUsed) { ui::clickUsed = true; S.beginTalk(ring); sfx(SFX_CLICK); }
        }
    }
    if (onPhone) {
        ui::text("ON THE PHONE", px, py - 96, 20, INK, 1);
        patienceBar(px, py - 68, 90, talking->patience);
    }

    // tags, bubbles and clicks (drawn over the counter)
    for (size_t i = 0; i < lobby.size(); i++) {
        Customer* c = S.cust(lobby[i]);
        Order* o = S.order(c->order);
        Spot sp = lobbySpot(i);
        float x = sp.x, base = sp.base, head = base - 226 * sp.k - 8;
        char b[32]; snprintf(b, sizeof b, "#%d", c->order);
        bool ready = o && o->ready && !o->done;
        Rectangle hit{x - 48, head - 40, 96, base - head - 30};
        if (ready) {
            float bounce = std::fabs(std::sin(ui::time * 5)) * 8;
            ui::panel({x - 50, head - 44 - bounce, 100, 40}, ui::GREEN, 10, 3, true);
            ui::text(std::string("READY ") + b, x, head - 36 - bounce, 20, ui::PAPER, 1);
            if (!S.talk.active() && ui::hover(hit)) {
                DrawRectangleRoundedLinesEx(hit, 0.2f, 6, 3, ui::YELLOW);
                if (ui::pressed && !ui::clickUsed) { ui::clickUsed = true; clickCust = c->id; }
            }
        } else {
            ui::panel({x - 26, head - 34, 52, 26}, ui::PAPER, 8, 2, false);
            ui::text(b, x, head - 31, 18, INK, 1);
        }
        patienceBar(x, head - 4, 70, c->patience);
        if (c->barkT > 0 && !ready) bubble(c->bark, x, head - 40, 17);
    }
    for (size_t i = 0; i < line.size(); i++) {
        Customer* c = S.cust(line[i]);
        Spot sp = lineSpot((float)i + (talking && !talking->phone ? 1 : 0));
        float x = sp.x, head = sp.base - 226 * sp.k - 20;
        patienceBar(x, head, 80, c->patience);
        if (c->barkT > 0) bubble(c->bark, x, head - 8, 17);
        if (i == 0 && !S.talk.active()) {
            Rectangle hit{x - 70, head - 10, 140, 250};
            float bounce = std::fabs(std::sin(ui::time * 4)) * 6;
            ui::panel({x - 74, head - 62 - bounce, 148, 40}, ui::YELLOW, 10, 3, true);
            ui::text("TAKE ORDER", x, head - 53 - bounce, 20, INK, 1);
            if (ui::hover(hit)) {
                DrawRectangleRoundedLinesEx(hit, 0.2f, 6, 3, ui::YELLOW);
                if (ui::pressed && !ui::clickUsed) { ui::clickUsed = true; S.beginTalk(c->id); sfx(SFX_CLICK); }
            }
        }
    }
    if (talking && !talking->phone) patienceBar(300, COUNTER_Y - 280, 80, talking->patience);
    if (clickCust >= 0) S.handOut(clickCust);
}

void dialogue() {
    Store& S = G.S;
    Rectangle box{20, 612, 900, 270};
    ui::panel(box, ui::PAPER, 12, 4, true);
    if (!S.talk.active()) {
        ui::text("THE FRONT", box.x + 24, box.y + 18, 30, INK);
        float y = box.y + 62;
        const char* tips[] = {"Click the customer at the register to take their order.", "Click the phone when it rings. They won't wait long.",
            "Punch in what they say on the order screen, then send it to the makeline.", "When an order's READY, click that customer to hand it over."};
        for (auto t : tips) { DrawCircleV({box.x + 34, y + 12}, 5, ui::RED); ui::text(t, box.x + 50, y, 22, INK); y += 34; }
        ui::text("Keys: 1 Front   2 Makeline   3 Oven & cut table   Esc Pause", box.x + 24, box.y + 226, 19, ui::GREY);
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
    if (ui::button({in.x + in.width - 280, in.y + in.height - 66, 266, 54}, "SEND TO MAKELINE", ui::RED, true, 24)) { S.finishOrder(); sfx(SFX_DING); G.posPart = WHOLE; G.posLevel = NORMAL; }
}
}  // namespace

void frontStation() {
    scene();
    dialogue();
    pos();
}
