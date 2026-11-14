// Makeline - the makeline: tickets on the rail, dough, sauce, cheese and toppings by hand.
#include "game.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace pz;
using ui::INK;

namespace {
const Vector2 PC{690, 590};      // the middle of the pizza on the bench
const float PBASE = 245;         // a large, in pixels
const float RIN[] = {5, 6, 7, 8};

void rail() {
    Store& S = G.S;
    DrawRectangle(0, 64, 1600, 196, {96, 104, 112, 255});
    DrawRectangle(0, 82, 1600, 12, {200, 206, 212, 255});
    DrawRectangle(0, 80, 1600, 3, INK); DrawRectangle(0, 93, 1600, 3, INK);
    auto open = S.openTickets();
    if (open.empty() && S.bench < 0) ui::text("No tickets. Go take some orders.", 800, 150, 26, {220, 224, 228, 255}, 1);
    float x = 24;
    for (int id : open) {
        Ticket* k = S.ticket(id);
        Rectangle r{x, 88, 176, 166};
        bool sel = G.pickTicket == id;
        if (sel) r.y += 6;
        drawTicket(*k, S.order(k->order), r, sel, 15);
        if (S.bench < 0 && ui::hover(r)) {
            DrawRectangleLinesEx({r.x - 4, r.y - 4, r.width + 8, r.height + 8}, 3, ui::YELLOW);
            if (ui::pressed && !ui::clickUsed) { ui::clickUsed = true; G.pickTicket = id; G.doughSize = G.doughCrust = -1; sfx(SFX_CLICK); }
        }
        x += 190;
        if (x > 1500) break;
    }
}

void bins() {
    Rectangle area{1050, 272, 530, 520};
    ui::panel(area, {150, 158, 166, 255}, 10, 4, true);
    static const char* HEAD[] = {"SAUCES", "CHEESES", "VEGETABLES", "MEATS"};
    static const int ROWS[4][6] = {{MARINARA, ALFREDO, GARLIC_PARM, BBQ, -1, -1}, {MOZZ, MOZZ_BALLS, PROVOLONE, CHEDDAR, PARMESAN, FETA},
        {ONION, PEPPER, PINEAPPLE, TOMATO, MUSHROOM, OLIVE}, {SAUSAGE, PEPPERONI, HAM, BACON, BEEF, CHICKEN}};
    float y = area.y + 8;
    for (int r = 0; r < 4; r++) {
        ui::text(HEAD[r], area.x + 12, y, 16, INK);
        y += 20;
        float bw = (area.width - 20 - 5 * 6) / 6, bh = 100;
        for (int i = 0; i < 6; i++) {
            int ing = ROWS[r][i];
            if (ing < 0) continue;
            Rectangle b{area.x + 10 + i * (bw + 6), y, bw, bh};
            bool sel = G.tool == ing, hov = ui::hover(b);
            DrawRectangleRounded({b.x - 3, b.y - 3, b.width + 6, b.height + 6}, 0.2f, 6, sel ? ui::RED : INK);
            DrawRectangleRounded(b, 0.2f, 6, hov ? Color{236, 240, 244, 255} : Color{214, 220, 226, 255});
            DrawRectangleRounded({b.x + 6, b.y + 6, b.width - 12, 58}, 0.25f, 6, {70, 76, 84, 255});   // the pan
            drawIngredientIcon(ing, {b.x + b.width / 2, b.y + 35}, info(ing).kind == SAUCE ? 20 : 17);
            std::string n = info(ing).name;
            size_t sp = n.find(' ');
            float fs = 14;
            if (sp != std::string::npos && ui::textW(n, fs) > b.width - 6) {
                ui::text(n.substr(0, sp), b.x + b.width / 2, b.y + 66, fs, INK, 1);
                ui::text(n.substr(sp + 1), b.x + b.width / 2, b.y + 81, fs, INK, 1);
            } else ui::text(n, b.x + b.width / 2, b.y + 73, fs, INK, 1);
            if (hov && ui::pressed && !ui::clickUsed) { ui::clickUsed = true; G.tool = sel ? -1 : ing; G.sprinkle = 0; sfx(SFX_CLICK); }
        }
        y += bh + 6;
    }
}

// what's on the pizza so far, against the ticket when the guides are on
void readout(const Pizza& p, const Ticket* k) {
    Rectangle box{20, 560, 300, 322};
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
    Rectangle box{360, 300, 660, 560};
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

void work(Pizza& p) {
    Store& S = G.S;
    float r = pizzaRadius(p.size, PBASE), inch = 1.0f / RIN[p.size];
    Vector2 m{(ui::M.x - PC.x) / r, (ui::M.y - PC.y) / r};
    bool over = m.x * m.x + m.y * m.y <= 1.08f * 1.08f && !ui::clickUsed && !G.chart;
    float dt = GetFrameTime();
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
    (void)S;
}

void bench() {
    Store& S = G.S;
    Pizza* p = S.pizza(S.bench);
    if (!p) { doughPicker(); return; }
    Ticket* k = S.ticket(p->ticket);
    // the ticket being made, pinned up large
    if (k) drawTicket(*k, S.order(k->order), {24, 276, 292, 270}, false, 20);
    readout(*p, k);
    // the board and the pizza
    float r = pizzaRadius(p->size, PBASE);
    DrawCircleV({PC.x + 6, PC.y + 10}, 300, ui::SHADOW);
    DrawCircleV(PC, 300, INK);
    DrawCircleV(PC, 296, {206, 170, 120, 255});
    work(*p);
    drawPizza(*p, PC, r);
    bool halves = false;
    if (k) for (auto& it : k->spec.items) if (it.part != WHOLE) halves = true;
    if (halves) {   // a guide down the middle for half-and-half
        for (float y = PC.y - r; y < PC.y + r; y += 18) DrawLineEx({PC.x, y}, {PC.x, y + 9}, 2, {255, 255, 255, 150});
        ui::text("L", PC.x - r - 22, PC.y - 12, 26, INK, 1);
        ui::text("R", PC.x + r + 22, PC.y - 12, 26, INK, 1);
    }
    bool room = S.ovenHasRoom();
    if (ui::button({1050, 806, 250, 70}, room ? "INTO THE OVEN" : "OVEN'S FULL", ui::RED, room, 26)) { if (S.benchToOven()) { sfx(SFX_OVEN); G.tool = -1; } }
    if (ui::button({1316, 806, 124, 70}, "Trash it", {200, 190, 176, 255}, true, 20)) { S.trashBench(); sfx(SFX_BAD); G.tool = -1; }
    // what's in hand follows the mouse
    if (G.tool >= 0 && !G.chart) {
        Vector2 at{ui::M.x + 26, ui::M.y + 26};
        DrawCircleV(at, 26, {255, 255, 255, 190});
        DrawRing(at, 26, 29, 0, 360, 24, INK);
        drawIngredientIcon(G.tool, at, 13);
    }
    ui::text("Left mouse: ladle / sprinkle / place        Right mouse: pick toppings off", PC.x, 268, 17, {70, 60, 50, 255}, 1);
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

void makeStation() {
    DrawRectangle(0, 64, 1600, 836, {186, 192, 198, 255});
    for (int i = 0; i < 9; i++) DrawRectangle(0, 300 + i * 70, 1600, 2, {174, 180, 186, 255});   // brushed steel
    rail();
    bins();
    bench();
    if (ui::button({1456, 806, 124, 70}, "Portion chart", ui::BLUE, true, 18)) G.chart = true;
}
