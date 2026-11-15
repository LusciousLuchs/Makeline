// Makeline - at the register: what the customer is saying, and the order screen.
#include "game.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace pz;
using ui::INK;

namespace {
Command mk(int type) { Command c; c.type = type; return c; }

void dialogue() {
    Store& S = G.S;
    Rectangle box{20, 612, 900, 270};
    ui::panel(box, ui::PAPER, 12, 4, true);
    if (!S.talk.active()) {
        ui::text("THE REGISTER", box.x + 24, box.y + 18, 30, INK);
        int next = S.firstInLine();
        if (next >= 0) {
            ui::text("Someone's waiting to order.", box.x + 24, box.y + 70, 24, INK);
            if (ui::button({box.x + 24, box.y + 120, 300, 64}, "TAKE THEIR ORDER", ui::RED, true, 26)) { Command t = mk(CMD_TAKE_ORDER); t.a = next; sendCommand(t); sfx(SFX_CLICK); }
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
    if (ui::button({bx, box.y + 56, 190, 52}, "Repeat that?", ui::YELLOW, true, 22)) { sendCommand(mk(CMD_ASK_REPEAT)); sfx(SFX_CLICK); }
    if (ui::button({bx, box.y + 124, 190, 52}, "Read it back", ui::BLUE, true, 22)) { sendCommand(mk(CMD_ASK_READBACK)); sfx(SFX_CLICK); }
    ui::wrap("Asking costs their patience.", bx, box.y + 192, 190, 17, ui::GREY);
}

const float PW = 1024, PH = 768;
const Color NAVYBAR{16, 24, 40, 255}, TANBG{216, 200, 164, 255}, SCREENBG{188, 198, 210, 255}, BTN{34, 92, 148, 255}, POSOR{232, 124, 48, 255},
    GREEN_B{74, 164, 72, 255}, RED_B{200, 44, 40, 255}, SECT{222, 230, 238, 255}, TABOFF{74, 84, 100, 255};

bool pbtn(Rectangle r, const std::string& label, bool selected, bool enabled = true, Color base = BTN, float size = 24) {
    return ui::button(r, label, selected ? POSOR : base, enabled, size, false);
}
bool tab(Rectangle r, const char* l1, const char* l2, Color fill, bool enabled) {
    bool h = enabled && ui::hover(r);
    ui::panel(r, !enabled ? TABOFF : h ? ui::shade(fill, 1.12f) : fill, 10, 3, true);
    float y = r.y + (l2[0] ? r.height / 2 - 28 : r.height / 2 - 14);
    ui::text(l1, r.x + r.width / 2, y, 25, enabled ? ui::PAPER : Color{150, 158, 172, 255}, 1);
    if (l2[0]) ui::text(l2, r.x + r.width / 2, y + 28, 25, enabled ? ui::PAPER : Color{150, 158, 172, 255}, 1);
    if (h && ui::pressed && !ui::clickUsed) { ui::clickUsed = true; return true; }
    return false;
}
std::string money(float v) { char b[24]; snprintf(b, sizeof b, "$%.2f", v); return b; }

void orderLines(const Spec& s, std::vector<std::string>& out) {
    if (s.sauce != MARINARA || s.sauceLevel != NORMAL) out.push_back(std::string(s.sauceLevel != NORMAL ? std::string(levelName(s.sauceLevel)) + " " : "") + info(s.sauce).name + " sauce");
    const Item* mz = s.find(MOZZ);
    if (!mz) out.push_back("No cheese"); else if (mz->level != NORMAL) out.push_back(std::string(levelName(mz->level)) + " cheese");
    for (auto& it : s.items) {
        if (it.ing == MOZZ) continue;
        out.push_back(std::string(it.part == LEFT ? "L: " : it.part == RIGHT ? "R: " : "") + (it.level != NORMAL ? std::string(levelName(it.level)) + " " : "") + info(it.ing).name);
    }
}

// the middle of the screen when nobody's being served: the open orders
void ordersList(float x0, float w) {
    Store& S = G.S;
    ui::text("Orders", x0, 50, 38, INK);
    int next = S.ringing() >= 0 ? S.ringing() : S.firstInLine();
    if (next >= 0 && pbtn({x0 + w - 280, 52, 280, 46}, S.ringing() >= 0 ? "Answer the phone" : "Take the next order", false, true, GREEN_B, 21)) {
        Command t = mk(CMD_TAKE_ORDER); t.a = next; sendCommand(t); sfx(SFX_CLICK);
    }
    float y = 112;
    int shown = 0;
    for (auto& o : S.orders) {
        if (o.done) continue;
        Customer* c = S.cust(o.cust);
        int boxed = 0;
        for (int id : o.tickets) { Ticket* k = S.ticket(id); if (k && k->state == T_BOXED) boxed++; }
        Rectangle row{x0, y, w, 58};
        DrawRectangleRounded(row, 0.2f, 6, o.ready ? Color{196, 230, 196, 255} : ui::PAPER);
        ui::text("#" + std::to_string(o.num) + "  " + (c ? c->who.name : ""), row.x + 10, row.y + 4, 22, INK);
        const char* svc = o.service == DELIVERY ? "Delivery" : o.service == CARRYOUT ? "Carryout" : "Counter";
        std::string st = o.out ? "On the road" : o.ready ? (o.service == DELIVERY ? "Waiting for a driver" : (c && c->state == C_WAITING ? "READY - hand it over" : "Ready")) : std::to_string(boxed) + " of " + std::to_string(o.tickets.size()) + " boxed";
        ui::text(std::string(svc) + " - " + st, row.x + 10, row.y + 32, 18, o.ready ? ui::shade(ui::GREEN, 0.6f) : ui::GREY);
        float mins = o.minutes(S.t);
        char tm[16]; snprintf(tm, sizeof tm, "%d:%02d", (int)mins, (int)(mins * 60) % 60);
        ui::text(tm, row.x + w - 12, row.y + 14, 30, mins >= LATE_MIN ? ui::DKRED : mins >= 15 ? Color{190, 130, 30, 255} : INK, 2);
        y += 64;
        if (++shown >= 9) break;
    }
    if (!shown) ui::text("No orders up.", x0, 120, 24, ui::GREY);
}

void pizzaTab(float x0, float w, std::vector<Spec>& D, bool& changed) {
    Store& S = G.S;
    S.talk.cur = std::clamp(S.talk.cur, 0, (int)D.size() - 1);
    Spec& P = D[S.talk.cur];
    ui::text("Pizza", x0, 48, 38, INK);
    char b[32]; snprintf(b, sizeof b, "%d of %d", S.talk.cur + 1, (int)D.size());
    ui::text(b, x0 + 112, 62, 22, ui::GREY);
    if (pbtn({x0 + 252, 50, 150, 46}, "New Pizza", false, D.size() < 4, GREEN_B)) { D.push_back(Spec{}); S.talk.cur = (int)D.size() - 1; changed = true; sfx(SFX_CLICK); return; }
    if (pbtn({x0 + 496, 50, 154, 46}, "Delete Pizza", false, D.size() > 1, RED_B)) { D.erase(D.begin() + S.talk.cur); S.talk.cur = 0; changed = true; sfx(SFX_CLICK); return; }
    float y = 104;
    ui::text("Select the pizza size, crust and toppings", x0, y, 22, ui::GREY);
    y += 26;
    auto section = [&](const char* label, float h) {
        DrawRectangleRounded({x0, y, w, h}, 0.08f, 6, SECT);
        DrawRectangleRoundedLinesEx({x0, y, w, h}, 0.08f, 6, 2, Color{120, 132, 150, 255});
        ui::text(label, x0 + 10, y + 3, 19, INK);
        float iy = y + 26;
        y += h + 6;
        return iy;
    };
    float iy = section("Size", 80);
    for (int i = 0; i < 4; i++)
        if (pbtn({x0 + 8 + i * 159.f, iy, 152, 46}, std::string(sizeName(i)) + " " + sizeInches(i), P.size == i, crustComesIn(P.crust, i))) { P.size = i; changed = true; sfx(SFX_CLICK); }
    iy = section("Crust", 80);
    static const char* const CR[] = {"Hand Tossed", "Thin Crust", "Pan"};
    for (int i = 0; i < 3; i++)
        if (pbtn({x0 + 8 + i * 212.f, iy, 204, 46}, CR[i], P.crust == i)) {
            P.crust = i;
            for (int d = 0; d < 4 && !crustComesIn(i, P.size); d++) { if (P.size + d < SIZE_N && crustComesIn(i, P.size + d)) P.size += d; else if (P.size - d >= 0 && crustComesIn(i, P.size - d)) P.size -= d; }
            changed = true; sfx(SFX_CLICK);
        }
    iy = section("Sauce", 80);
    for (int i = 0; i < SAUCE_N; i++)
        if (pbtn({x0 + 8 + i * 106.f, iy, 100, 46}, info(i).name, P.sauce == i, true, BTN, 20)) { P.sauce = i; changed = true; sfx(SFX_CLICK); }
    static const char* const AMT3[] = {"Light", "Norm", "Extra"};
    for (int i = 0; i < 3; i++)
        if (pbtn({x0 + 440 + i * 66.f, iy, 62, 46}, AMT3[i], P.sauceLevel == i + 1, true, BTN, 19)) { P.sauceLevel = i + 1; changed = true; sfx(SFX_CLICK); }
    iy = section("Cheese", 80);
    static const char* const CH[] = {"No cheese", "Light", "Normal", "Extra"};
    Item* mz = P.find(MOZZ);
    for (int i = 0; i < 4; i++)
        if (pbtn({x0 + 8 + i * 158.f, iy, 150, 46}, CH[i], (mz ? mz->level : NONE) == i)) {
            P.remove(MOZZ);
            if (i != NONE) P.items.insert(P.items.begin(), {MOZZ, WHOLE, i});
            changed = true; sfx(SFX_CLICK);
        }
    iy = section("Toppings", 216);
    static const char* const PT[] = {"Whole", "Left half", "Right half"};
    static const char* const LV[] = {"Light", "Normal", "Extra"};
    for (int i = 0; i < 3; i++) {
        if (pbtn({x0 + 8 + i * 102.f, iy, 98, 38}, PT[i], G.posPart == i, true, BTN, 20)) { G.posPart = i; sfx(SFX_CLICK); }
        if (pbtn({x0 + 336 + i * 102.f, iy, 98, 38}, LV[i], G.posLevel == i + 1, true, BTN, 20)) { G.posLevel = i + 1; sfx(SFX_CLICK); }
    }
    static const int GRID[3][6] = {{PEPPERONI, SAUSAGE, HAM, BACON, BEEF, CHICKEN}, {ONION, PEPPER, PINEAPPLE, TOMATO, MUSHROOM, OLIVE}, {MOZZ_BALLS, PROVOLONE, CHEDDAR, PARMESAN, FETA, -1}};
    static const char* const SHORT[ING_N] = {"", "", "", "", "", "Mozz Balls", "Provolone", "Cheddar", "Parmesan", "Feta", "Onions", "Peppers", "Pineapple", "Tomatoes", "Mushrooms", "Olives", "Sausage", "Pepperoni", "Can. Bacon", "Bacon", "Beef", "Chicken"};
    for (int r = 0; r < 3; r++) for (int c = 0; c < 6; c++) {
        int ing = GRID[r][c];
        if (ing < 0) continue;
        Item* it = P.find(ing);
        Rectangle bt{x0 + 8 + c * 105.f, iy + 46 + r * 50.f, 100, 44};
        if (pbtn(bt, SHORT[ing], it != nullptr, true, BTN, 21)) {
            if (it && it->part == G.posPart && it->level == G.posLevel) P.remove(ing);
            else if (it) { it->part = G.posPart; it->level = G.posLevel; }
            else P.items.push_back({ing, G.posPart, G.posLevel});
            changed = true; sfx(SFX_CLICK);
        }
    }
}

void drawPOS() {
    Store& S = G.S;
    ClearBackground(SCREENBG);
    Customer* c = S.talk.active() ? S.cust(S.talk.cust) : nullptr;
    std::vector<Spec> D = S.talk.draft;
    bool changed = false;
    float x0 = 246, w = 650;
    // ---- the top bar
    DrawRectangle(0, 0, (int)PW, 44, NAVYBAR);
    char hd[48]; snprintf(hd, sizeof hd, "CSR  You    Day %d", S.day);
    ui::text(hd, 16, 9, 24, ui::PAPER);
    ui::text("Late 20   Very late 30   Extreme 45", PW / 2 + 20, 10, 23, Color{160, 184, 222, 255}, 1);
    ui::text(S.clock(), PW - 16, 9, 24, ui::PAPER, 2);
    // ---- the middle
    if (!c) ordersList(x0, w);
    else if (G.posTab == 0) {
        ui::text("Customer", x0, 48, 38, INK);
        ui::text(c->who.name, x0, 112, 30, INK);
        ui::text(c->phone ? "Phone order" : "At the counter", x0, 150, 22, ui::GREY);
        if (c->phone) {
            DrawRectangleRounded({x0, 196, w, 120}, 0.08f, 6, SECT);
            ui::text("Service", x0 + 12, 202, 19, INK);
            if (pbtn({x0 + 12, 236, 300, 60}, "Carryout", S.talk.draftService != DELIVERY, true, BTN, 24)) { Command d = mk(CMD_SET_DRAFT); d.a = CARRYOUT; d.text = specsToJson(D); sendCommand(d); sfx(SFX_CLICK); }
            if (pbtn({x0 + 330, 236, 300, 60}, "Delivery", S.talk.draftService == DELIVERY, true, BTN, 24)) { Command d = mk(CMD_SET_DRAFT); d.a = DELIVERY; d.text = specsToJson(D); sendCommand(d); sfx(SFX_CLICK); }
        }
        ui::wrap("Enter what the customer says on the Pizzas tab. What you enter here is what gets made: the customer's order isn't shown to you.", x0, 340, w, 21, ui::GREY);
    } else if (G.posTab == 1) {
        ui::text("Greeting / Specials", x0, 48, 38, INK);
        if (pbtn({x0, 130, 300, 60}, "Ask them to repeat it", false, true, BTN, 22)) { sendCommand(mk(CMD_ASK_REPEAT)); sfx(SFX_CLICK); }
        if (pbtn({x0 + 330, 130, 300, 60}, "Read the order back", false, true, BTN, 22)) { sendCommand(mk(CMD_ASK_READBACK)); sfx(SFX_CLICK); }
        ui::wrap("Asking costs their patience. There are no specials on the board yet.", x0, 220, w, 21, ui::GREY);
    } else pizzaTab(x0, w, D, changed);
    // ---- the order so far, down the left
    DrawRectangle(0, 44, 236, (int)PH - 44, TANBG);
    float ly = 54, total = 0;
    if (c) {
        ui::text(c->who.name, 12, ly, 25, INK); ly += 30;
        ui::text(c->phone ? (S.talk.draftService == DELIVERY ? "Delivery" : "Carryout") : "Counter", 12, ly, 20, ui::DKRED); ly += 30;
        DrawLineEx({8, ly}, {228, ly}, 2, Color{150, 134, 100, 255}); ly += 8;
        for (int i = 0; i < (int)D.size(); i++) {
            std::vector<std::string> lines;
            orderLines(D[i], lines);
            float h = 30 + 21 * lines.size() + 8;
            if (ly + h > PH - 80) break;
            Rectangle blk{0, ly - 3, 236, h};
            bool sel = S.talk.cur == i;
            if (sel) { DrawRectangleRec(blk, Color{240, 214, 160, 255}); DrawTriangle({236, ly + 8}, {222, ly + 22}, {236, ly + 36}, POSOR); }
            std::string title = std::to_string(i + 1) + " " + sizeInches(D[i].size) + " " + crustName(D[i].crust);
            float fs = 18; while (fs > 12 && ui::textW(title, fs) > 150) fs--;
            ui::text(title, 10, ly, fs, INK);
            ui::text(money(price(D[i])), 226, ly, 18, INK, 2);
            float yy = ly + 28;
            for (auto& l : lines) { float f2 = 16; while (f2 > 11 && ui::textW(l, f2) > 214) f2--; ui::text(l, 16, yy, f2, Color{60, 48, 40, 255}); yy += 21; }
            if (ui::hover(blk) && ui::pressed && !ui::clickUsed) { ui::clickUsed = true; S.talk.cur = i; sfx(SFX_CLICK); }
            total += price(D[i]);
            ly += h;
        }
        DrawLineEx({8, PH - 74}, {228, PH - 74}, 2, Color{150, 134, 100, 255});
        ui::text("Paid  $0.00", 12, PH - 68, 19, INK);
        char tb[48]; snprintf(tb, sizeof tb, "(%d %s) Total", (int)D.size(), D.size() == 1 ? "Item" : "Items");
        ui::text(tb, 12, PH - 44, 19, INK);
        ui::text(money(total), 226, PH - 44, 22, INK, 2);
    } else ui::text("No order open", 12, 60, 21, Color{110, 96, 70, 255});
    // ---- the tabs down the right
    struct Tab { const char* a; const char* b; Color col; bool on; };
    bool talkOn = c != nullptr;
    Tab tabs[8] = {{"Exit", "Order", RED_B, true}, {"Customer", "", BTN, talkOn}, {"Greeting/", "Specials", BTN, talkOn}, {"Pizzas", "", BTN, talkOn},
        {"Everything", "Else", BTN, false}, {"Coupons", "", BTN, false}, {"Payments", "", BTN, false}, {"Finish", "", BTN, talkOn && !D.empty()}};
    for (int i = 0; i < 8; i++) {
        Rectangle r{906, 48 + i * 90.f, 114, 84};
        Color fill = tabs[i].col;
        bool sel = (i == 1 && G.posTab == 0) || (i == 2 && G.posTab == 1) || (i == 3 && G.posTab == 2);
        if (sel && talkOn) fill = POSOR;
        if (i == 7) fill = GREEN_B;
        if (!tab(r, tabs[i].a, tabs[i].b, fill, tabs[i].on)) continue;
        sfx(SFX_CLICK);
        if (i == 0) setMode(M_WALK);
        else if (i == 1) G.posTab = 0;
        else if (i == 2) G.posTab = 1;
        else if (i == 3) G.posTab = 2;
        else if (i == 7) {
            if (changed) { Command d = mk(CMD_SET_DRAFT); d.a = S.talk.draftService; d.text = specsToJson(D); sendCommand(d); changed = false; }
            sendCommand(mk(CMD_SEND_ORDER)); sfx(SFX_DING); G.posPart = WHOLE; G.posLevel = NORMAL; G.posTab = 2; setMode(M_WALK);
            return;
        }
    }
    if (changed) { Command d = mk(CMD_SET_DRAFT); d.a = S.talk.draftService; d.text = specsToJson(D); sendCommand(d); }
}
}  // namespace

bool sendCommand(const Command& c) {
    bool ok = apply(G.S, c);
    if (G.cmdLog.size() < 50000) G.cmdLog.push_back(c);
    return ok;
}

namespace {
RenderTexture2D gPos{};
bool gPosMade = false;
}
Texture2D posTexture() {
    if (!gPosMade) { gPos = LoadRenderTexture(1024, 768); SetTextureFilter(gPos.texture, TEXTURE_FILTER_BILINEAR); gPosMade = true; }
    return gPos.texture;
}
void posRender() {
    posTexture();
    Vector2 keepM = ui::M; bool kp = ui::pressed, kd = ui::down;
    Vector2 m;
    bool live = G.mode == M_REGISTER && !G.paused && !G.chart && !G.office && posMouse(m);
    if (live) ui::M = m; else { ui::M = {-999, -999}; ui::pressed = ui::down = false; }
    BeginTextureMode(gPos);
    drawPOS();
    EndTextureMode();
    ui::M = keepM; ui::pressed = kp; ui::down = kd;   // (a click the terminal took stays taken)
}

void subtitles() {
    Store& S = G.S;
    if (!S.talk.active()) return;
    auto& log = S.talk.log;
    int n = (int)log.size();
    if (n == 0) return;
    DrawRectangleGradientV(0, 620, 920, 280, {10, 8, 6, 0}, {10, 8, 6, 190});
    Customer* c = S.cust(S.talk.cust);
    float y = 872;
    int shownLines = 0;
    for (int i = n - 1; i >= 0 && shownLines < 3; i--, shownLines++) {
        bool last = i == n - 1;
        std::string t = (log[i].you ? "You: " : "") + log[i].text;
        int chars = last && !log[i].you && S.talk.speaking ? (int)S.talk.shown : -1;
        float h = ui::wrap(t, 0, 0, 820, 30, ui::PAPER, false);
        y -= h + 8;
        unsigned char a = (unsigned char)(255 - shownLines * 85);
        ui::wrap(t, 62, y + 2, 820, 30, {0, 0, 0, a}, true, chars);
        ui::wrap(t, 60, y, 820, 30, log[i].you ? Color{246, 194, 66, a} : Color{255, 250, 238, a}, true, chars);
    }
    if (c) ui::text(c->who.name + (c->phone ? " (on the phone)" : ""), 60, y - 34, 22, {200, 190, 176, 220});
}

void registerOverlay() {
    if (!G.hud) { subtitles(); return; }
    dialogue();
    if (!G.S.talk.active() && ui::button({20, 80, 190, 46}, "Step back (Esc)", ui::CREAM, true, 19)) setMode(M_WALK);
}
