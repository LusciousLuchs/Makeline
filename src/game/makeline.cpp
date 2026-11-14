// Makeline - working the makeline: tickets, dough, then sauce, cheese and toppings by hand onto the pizza in front of you.
#include "game.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace pz;
using ui::INK;

namespace {
void rail() {
    Store& S = G.S;
    DrawRectangle(0, 64, 1600, 200, {40, 30, 26, 150});
    auto open = S.openTickets();
    if (open.empty()) ui::text("No tickets. Go take some orders.", 800, 150, 26, ui::PAPER, 1);
    float x = 230;
    for (int id : open) {
        Ticket* k = S.ticket(id);
        Rectangle r{x, 80, 176, 170};
        bool sel = G.pickTicket == id;
        if (sel) r.y += 8;
        drawTicket(*k, S.order(k->order), r, sel, 15);
        if (ui::hover(r)) {
            DrawRectangleLinesEx({r.x - 4, r.y - 4, r.width + 8, r.height + 8}, 3, ui::YELLOW);
            if (ui::pressed && !ui::clickUsed) { ui::clickUsed = true; G.pickTicket = id; G.doughSize = G.doughCrust = -1; sfx(SFX_CLICK); }
        }
        x += 190;
        if (x > 1420) break;
    }
}

// what's on the pizza so far, against the ticket when the guides are on
void readout(const Pizza& p, const Ticket* k) {
    Rectangle box{20, 350, 300, 330};
    ui::panel(box, ui::PAPER, 10, 4, true);
    ui::text("ON THE PIZZA", box.x + 14, box.y + 10, 20, INK);
    float y = box.y + 40;
    bool guides = G.S.guides && k;
    auto line = [&](int ing, float amount, float target) {
        if (y > box.y + box.height - 26) return;
        std::string n = info(ing).name;
        float fs = 17;
        while (fs > 12 && ui::textW(n, fs) > 128) fs -= 1;
        ui::text(n, box.x + 14, y, fs, INK);
        ui::text(amountText(ing, amount), box.x + 200, y, 17, INK, 2);
        if (guides) {
            Rectangle bar{box.x + 208, y + 3, 80, 12};
            DrawRectangleRec(bar, {220, 212, 196, 255});
            if (target > 0) {
                float full = target * 1.6f;
                DrawRectangleRec({bar.x + bar.width * 0.8f / 1.6f, bar.y, bar.width * 0.5f / 1.6f, bar.height}, {150, 210, 150, 255});
                float u = std::clamp(amount / full, 0.f, 1.f);
                float r = amount / target;
                DrawRectangleRec({bar.x + u * bar.width - 2, bar.y - 3, 4, bar.height + 6}, r < 0.8f || r > 1.3f ? ui::RED : ui::shade(ui::GREEN, 0.6f));
            } else {
                DrawRectangleRec(bar, {240, 170, 160, 255});
                ui::text("not on ticket", bar.x + 2, bar.y - 1, 12, ui::DKRED);
            }
            DrawRectangleLinesEx(bar, 1.5f, INK);
        }
        y += 22;
    };
    for (int s = 0; s < SAUCE_N; s++) {
        float want = guides && k->spec.sauce == s ? portion(s, k->spec.size, k->spec.sauceLevel) : 0;
        if (p.sauceOz[s] > 0.05f || want > 0) line(s, p.sauceOz[s], want);
    }
    for (int ing = SAUCE_N; ing < ING_N; ing++) {
        float want = 0;
        if (guides) if (const Item* it = k->spec.find(ing)) want = portion(ing, k->spec.size, it->level) * (it->part == WHOLE ? 1.f : 0.5f);
        float got = p.amount(ing);
        if (got > 0 || want > 0) line(ing, got, want);
    }
    if (!guides) ui::wrap("No guides: know your portions (see the chart).", box.x + 14, box.y + box.height - 50, 270, 15, ui::GREY);
}

void doughPicker() {
    Store& S = G.S;
    Ticket* k = S.ticket(G.pickTicket);
    if (!k || k->state != T_WAIT) { G.pickTicket = -1; k = nullptr; }
    Rectangle box{470, 290, 660, 560};
    ui::panel(box, {226, 214, 190, 255}, 14, 4, true);
    if (!k) {
        auto open = S.openTickets();
        ui::text(open.empty() ? "The bench is clear." : "Pick a ticket from the rail.", box.x + box.width / 2, box.y + 230, 32, INK, 1);
        if (!open.empty()) ui::text("Then grab the right dough for it.", box.x + box.width / 2, box.y + 276, 22, ui::GREY, 1);
        return;
    }
    ui::text("GRAB A DOUGH", box.x + 24, box.y + 16, 30, INK);
    ui::text("Size", box.x + 24, box.y + 66, 20, ui::GREY);
    for (int i = 0; i < 4; i++) {
        float cx = box.x + 100 + i * 155, cy = box.y + 190, r = 34 + i * 9.f;
        Rectangle hit{cx - 70, cy - 90, 140, 180};
        bool sel = G.doughSize == i;
        DrawRectangleRounded(hit, 0.15f, 6, sel ? ui::YELLOW : ui::hover(hit) ? Color{244, 236, 216, 255} : Color{236, 226, 204, 255});
        DrawRectangleRoundedLinesEx(hit, 0.15f, 6, 3, INK);
        DrawCircleV({cx, cy - 14}, r + 3, INK);
        DrawCircleV({cx, cy - 14}, r, {244, 230, 196, 255});
        ui::text(sizeName(i), cx, cy + 46, 20, INK, 1);
        ui::text(sizeInches(i), cx, cy + 66, 17, ui::GREY, 1);
        if (ui::hover(hit) && ui::pressed && !ui::clickUsed) { ui::clickUsed = true; G.doughSize = i; sfx(SFX_CLICK); }
    }
    ui::text("Crust", box.x + 24, box.y + 300, 20, ui::GREY);
    for (int i = 0; i < 3; i++)
        if (ui::button({box.x + 30 + i * 205, box.y + 332, 190, 56}, crustName(i), G.doughCrust == i ? ui::YELLOW : Color{236, 226, 204, 255}, true, 22)) { G.doughCrust = i; sfx(SFX_CLICK); }
    bool ok = G.doughSize >= 0 && G.doughCrust >= 0;
    if (ui::button({box.x + 180, box.y + 440, 300, 70}, "SLAP IT OUT", ui::RED, ok, 28) && ok) {
        if (S.startPizza(k->id, G.doughSize, G.doughCrust)) { sfx(SFX_PLOP); G.tool = -1; G.pickTicket = -1; }
    }
}


// the mouse over the makeline: pans pick an ingredient, the pizza takes it
void work(Pizza& p) {
    const g3::Frame& F = MAKE_F;
    float R = g3::pizzaMetres(p.size), inch = 0.0254f / R, dt = GetFrameTime();
    Vector3 hit;
    bool onUi = ui::M.x < 330 || (ui::M.x > 1290 && ui::M.y > 700) || ui::M.y < 64 || ui::clickUsed || G.chart;
    if (onUi || !mouseOnPlane(F.o.y + 0.014f, hit)) { if (!ui::down) G.lastPiece = {-9, -9}; return; }
    Vector2 l = F.local(hit);
    for (int ing = 0; ing < ING_N; ing++) {
        Vector2 b = binSpot(ing);
        if (std::fabs(l.x - b.x) < BIN / 2 && std::fabs(l.y - b.y) < BIN / 2) {
            Vector2 a, c;
            if (project(F.at(b.x - BIN / 2, 0.01f, b.y - BIN / 2), a) && project(F.at(b.x + BIN / 2, 0.01f, b.y + BIN / 2), c))
                DrawRectangleLinesEx({std::min(a.x, c.x), std::min(a.y, c.y), std::fabs(c.x - a.x), std::fabs(c.y - a.y)}, 3, ui::YELLOW);
            if (ui::pressed) { ui::clickUsed = true; G.tool = G.tool == ing ? -1 : ing; G.sprinkle = 0; sfx(SFX_CLICK); }
            return;
        }
    }
    Vector2 m{(l.x - BENCH_X) / R, l.y / R};
    bool over = m.x * m.x + m.y * m.y <= 1.08f * 1.08f;
    if (G.tool >= 0 && over) {
        const IngInfo& in = info(G.tool);
        if (in.kind == SAUCE) {
            if (ui::down) { p.addSauce(G.tool, m.x, m.y, in.rate * dt); if (ui::pressed) sfx(SFX_PLOP); }
        } else if (in.piece) {
            float gap = (G.tool == HAM ? 1.9f : G.tool == PEPPERONI ? 1.5f : 1.3f) * inch;
            float dx = m.x - G.lastPiece.x, dy = m.y - G.lastPiece.y;
            if (ui::pressed || (ui::down && dx * dx + dy * dy > gap * gap)) {
                if (p.addBit(G.tool, m.x, m.y, G.fx)) { G.lastPiece = m; sfx(SFX_PLOP); }
            }
        } else if (ui::down) {
            G.sprinkle += in.rate * dt;
            while (G.sprinkle >= 1) {
                G.sprinkle -= 1;
                float a = G.fx.range(0, 6.2832f), d = std::sqrt(G.fx.f()) * 1.25f * inch;
                p.addBit(G.tool, m.x + std::cos(a) * d, m.y + std::sin(a) * d, G.fx);
            }
            if (ui::pressed) sfx(SFX_PLOP);
        }
    }
    if (!ui::down) G.lastPiece = {-9, -9};
    if (ui::rdown && over) p.removeNear(m.x, m.y, 0.7f * inch);   // pick it back off
}

void labels() {
    for (int ing = 0; ing < ING_N; ing++) {
        Vector2 b = binSpot(ing), at;
        if (!project(MAKE_F.at(b.x, 0.02f, b.y + BIN * 0.36f), at)) continue;
        std::string n = info(ing).name;
        float w = ui::textW(n, 14) + 10;
        DrawRectangleRounded({at.x - w / 2, at.y - 9, w, 19}, 0.5f, 4, G.tool == ing ? ui::RED : Color{40, 30, 26, 200});
        ui::text(n, at.x, at.y - 7, 14, ui::PAPER, 1);
    }
}
}  // namespace

void portionChart() {
    DrawRectangle(0, 0, 1600, 900, {30, 20, 16, 150});
    Rectangle box{250, 80, 1100, 760};
    ui::panel(box, ui::PAPER, 14, 4, true);
    ui::text("PORTION CHART", box.x + 30, box.y + 18, 34, ui::DKRED);
    ui::text("a normal, whole pizza", box.x + 330, box.y + 32, 20, ui::GREY);
    float cx[] = {box.x + 330, box.x + 460, box.x + 590, box.x + 720};
    for (int s = 0; s < 4; s++) { ui::text(sizeName(s), cx[s], box.y + 70, 20, INK, 1); ui::text(sizeInches(s), cx[s], box.y + 90, 16, ui::GREY, 1); }
    float y = box.y + 116;
    auto row = [&](const std::string& name, int ing) {
        ui::text(name, box.x + 40, y, 19, INK);
        for (int s = 0; s < 4; s++) ui::text(amountText(ing, info(ing).normal[s]), cx[s], y, 19, INK, 1);
        y += 25;
    };
    row("Any sauce", MARINARA);
    DrawLineEx({box.x + 30, y + 1}, {box.x + 790, y + 1}, 1.5f, ui::GREY); y += 5;
    for (int ing = MOZZ; ing < ING_N; ing++) {
        if (ing == ONION || ing == SAUSAGE) { DrawLineEx({box.x + 30, y + 1}, {box.x + 790, y + 1}, 1.5f, ui::GREY); y += 5; }
        row(info(ing).name, ing);
    }
    float nx = box.x + 820, ny = box.y + 80, nw = 250;
    auto note = [&](const std::string& h, const std::string& t) {
        ui::text(h, nx, ny, 21, ui::DKRED); ny += 26;
        ny += ui::wrap(t, nx, ny, nw, 18, INK) + 14;
    };
    note("Light / Extra", "Light is half a portion. Extra is one and a half.");
    note("Half and half", "A topping on one half gets half its portion, all of it on that half.");
    note("Close enough", "Within about a fifth either way is fine. Spread it evenly, edge to edge.");
    note("Cutting", "Pie cut: Small is 6 slices (3 cuts), the rest are 8 (4 cuts), all through the middle. THIN crust is cut in squares.");
    note("Sizes", "10\" Small, 12\" Medium, 14\" Large, 16\" X-Large. Thin comes in M and L, Pan in M.");
    if (ui::button({box.x + box.width - 150, box.y + 16, 130, 44}, "Close", ui::YELLOW, true, 22) || IsKeyPressed(KEY_TAB)) G.chart = false;
}


void makeOverlay() {
    Store& S = G.S;
    if (ui::button({20, 76, 190, 44}, "Step back (Esc)", ui::CREAM, true, 19)) { setMode(M_WALK); return; }
    Pizza* p = S.pizza(S.bench);
    if (!p) { rail(); doughPicker(); return; }
    Ticket* k = S.ticket(p->ticket);
    if (k) drawTicket(*k, S.order(k->order), {20, 132, 270, 206}, false, 17);
    readout(*p, k);
    labels();
    work(*p);
    bool halves = false;
    if (k) for (auto& it : k->spec.items) if (it.part != WHOLE) halves = true;
    float R = g3::pizzaMetres(p->size);
    if (halves) {   // a guide down the middle for half-and-half
        Vector2 a, b, l, r;
        if (project(MAKE_F.at(BENCH_X, 0.03f, -R), a) && project(MAKE_F.at(BENCH_X, 0.03f, R), b))
            for (int i = 0; i < 16; i += 2) DrawLineEx({a.x + (b.x - a.x) * i / 16, a.y + (b.y - a.y) * i / 16}, {a.x + (b.x - a.x) * (i + 1) / 16, a.y + (b.y - a.y) * (i + 1) / 16}, 3, {255, 255, 255, 190});
        if (project(MAKE_F.at(BENCH_X - R - 0.035f, 0.02f, 0), l)) ui::text("L", l.x, l.y - 16, 30, ui::PAPER, 1);
        if (project(MAKE_F.at(BENCH_X + R + 0.035f, 0.02f, 0), r)) ui::text("R", r.x, r.y - 16, 30, ui::PAPER, 1);
    }
    bool room = S.ovenHasRoom();
    if (ui::button({1310, 806, 270, 70}, room ? "INTO THE OVEN" : "OVEN'S FULL", ui::RED, room, 26)) { if (S.benchToOven()) { sfx(SFX_OVEN); G.tool = -1; } }
    if (ui::button({1310, 730, 128, 54}, "Trash it", {200, 190, 176, 255}, true, 20)) { S.trashBench(); sfx(SFX_BAD); G.tool = -1; }
    if (ui::button({1452, 730, 128, 54}, "Portions", ui::BLUE, true, 20)) G.chart = true;
    if (G.tool >= 0 && !G.chart) {   // what's in hand follows the mouse
        Vector2 at{ui::M.x + 30, ui::M.y + 30};
        DrawCircleV(at, 26, {255, 255, 255, 200});
        DrawRing(at, 26, 29, 0, 360, 24, INK);
        drawIngredientIcon(G.tool, at, 13);
    }
    ui::text("Left mouse: ladle / sprinkle / place        Right mouse: pick toppings off        Tab: portions", 800, 872, 17, ui::PAPER, 1);
}
