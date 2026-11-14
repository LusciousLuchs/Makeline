// Makeline - the oven, the cut table and the heat rack.
#include "game.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace pz;
using ui::INK;

namespace {
const Vector2 CC{470, 640};     // the middle of the cut board
const float CBASE = 190;

void oven() {
    Store& S = G.S;
    Rectangle body{40, 96, 1080, 250};
    ui::panel(body, {120, 126, 134, 255}, 16, 4, true);
    DrawRectangleRounded({body.x + 16, body.y + 14, body.width - 32, 26}, 0.5f, 6, {86, 92, 100, 255});
    ui::text("CONVEYOR OVEN", body.x + 30, body.y + 16, 20, {230, 232, 236, 255});
    Rectangle win{body.x + 30, body.y + 56, body.width - 60, 170};
    DrawRectangleRec({win.x - 4, win.y - 4, win.width + 8, win.height + 8}, INK);
    DrawRectangleGradientV((int)win.x, (int)win.y, (int)win.width, (int)win.height, {70, 30, 20, 255}, {150, 60, 24, 255});
    // the belt
    float off = std::fmod(ui::time * 40, 40.f);
    for (float x = win.x - 40 + off; x < win.x + win.width; x += 40) if (x > win.x) DrawLineEx({x, win.y + 20}, {x, win.y + win.height - 6}, 2, {40, 20, 14, 255});
    for (float x = win.x + 60; x < win.x + win.width; x += 180) {   // the burners' glow
        float g = 0.6f + 0.4f * std::sin(ui::time * 6 + x);
        DrawRectangleRec({x, win.y, std::min(90.f, win.x + win.width - x), 8}, {255, (unsigned char)(140 + 60 * g), 40, 255});
    }
    for (auto& s : S.oven) {
        Pizza* p = S.pizza(s.pizza);
        if (!p) continue;
        float x = win.x + 80 + s.prog * (win.width - 160);
        drawPizza(*p, {x, win.y + 96}, pizzaRadius(p->size, 62), std::clamp(s.prog * 1.1f, 0.f, 1.f));
    }
    char b[64];
    // the exit tray
    Rectangle tray{1140, 110, 440, 230};
    ui::panel(tray, {200, 206, 212, 255}, 12, 4, true);
    ui::text("OUT OF THE OVEN", tray.x + 16, tray.y + 8, 20, INK);
    if (S.exitQ.empty()) ui::text("nothing yet", tray.x + tray.width / 2, tray.y + 110, 20, ui::GREY, 1);
    for (size_t i = 0; i < S.exitQ.size() && i < 3; i++) {
        Pizza* p = S.pizza(S.exitQ[i]);
        Vector2 c{tray.x + 80 + (float)i * 140, tray.y + 124};
        float r = pizzaRadius(p->size, 58);
        drawPizza(*p, c, r);
        Ticket* k = S.ticket(p->ticket);
        if (k) { snprintf(b, sizeof b, "#%d", k->order); ui::text(b, c.x, tray.y + 196, 20, INK, 1); }
        Rectangle hit{c.x - 68, tray.y + 40, 136, 180};
        if (S.cutBoard < 0 && ui::hover(hit)) {
            DrawRectangleRoundedLinesEx(hit, 0.15f, 6, 3, ui::YELLOW);
            if (ui::pressed && !ui::clickUsed) { ui::clickUsed = true; S.toCutBoard(p->id); sfx(SFX_CLICK); G.cutting = false; }
        }
    }
    if (S.exitQ.size() > 3) { snprintf(b, sizeof b, "+%d more", (int)S.exitQ.size() - 3); ui::text(b, tray.x + tray.width - 14, tray.y + 10, 18, ui::DKRED, 2); }
}

void cutTable() {
    Store& S = G.S;
    Rectangle table{40, 372, 880, 508};
    ui::panel(table, {206, 170, 120, 255}, 14, 4, true);
    ui::text("CUT TABLE", table.x + 20, table.y + 12, 24, INK);
    Pizza* p = S.pizza(S.cutBoard);
    if (!p) {
        ui::text(S.exitQ.empty() ? "Waiting on the oven." : "Click a pizza coming out of the oven.", CC.x, CC.y - 16, 26, {110, 80, 50, 255}, 1);
        return;
    }
    float r = pizzaRadius(p->size, CBASE);
    Ticket* k = S.ticket(p->ticket);
    if (k) drawTicket(*k, S.order(k->order), {table.x + table.width - 196, table.y + 16, 180, 210}, false, 15);
    drawPizza(*p, CC, r);
    // drag the cutter across
    Vector2 m{(ui::M.x - CC.x) / r, (ui::M.y - CC.y) / r};
    bool near = m.x * m.x + m.y * m.y < 1.5f * 1.5f;
    if (ui::pressed && near && !ui::clickUsed) { G.cutting = true; G.cutFrom = m; }
    if (G.cutting) {
        Vector2 a{CC.x + G.cutFrom.x * r, CC.y + G.cutFrom.y * r};
        DrawLineEx(a, ui::M, 5, INK);
        DrawLineEx(a, ui::M, 2, {240, 240, 244, 255});
        if (!ui::down) {
            G.cutting = false;
            size_t n = p->cuts.size();
            p->addCut(G.cutFrom.x, G.cutFrom.y, m.x, m.y);
            if (p->cuts.size() > n) sfx(SFX_CUT);
        }
    }
    char b[64];
    snprintf(b, sizeof b, "%d cut%s", (int)p->cuts.size(), p->cuts.size() == 1 ? "" : "s");
    ui::text(b, table.x + 24, table.y + 50, 20, {90, 60, 40, 255});
    ui::wrap("Drag the cutter right across the pizza.", table.x + 24, table.y + table.height - 60, 220, 17, {90, 60, 40, 255});
    float bx = table.x + table.width - 196;
    if (ui::button({bx, table.y + 250, 180, 70}, "BOX IT", ui::RED, true, 28)) { S.boxCut(); sfx(SFX_DING); G.cutting = false; }
    if (ui::button({bx, table.y + 340, 180, 46}, "Start the cut over", {226, 214, 190, 255}, !p->cuts.empty(), 17)) p->cuts.clear();
    if (ui::button({bx, table.y + 404, 180, 46}, "Trash & remake", {200, 190, 176, 255}, true, 17)) { S.trashCut(); sfx(SFX_BAD); }
}

void rack() {
    Store& S = G.S;
    Rectangle box{940, 372, 640, 508};
    ui::panel(box, {70, 74, 84, 255}, 14, 4, true);
    ui::text("HEAT RACK", box.x + 20, box.y + 12, 24, ui::PAPER);
    char b[96];
    snprintf(b, sizeof b, "Drivers in: %d of 2", S.driversFree);
    ui::text(b, box.x + box.width - 20, box.y + 16, 19, {200, 210, 220, 255}, 2);
    float y = box.y + 54;
    int shown = 0;
    for (auto& o : S.orders) {
        if (o.done) continue;
        int boxed = 0;
        for (int id : o.tickets) { Ticket* k = S.ticket(id); if (k && k->state == T_BOXED) boxed++; }
        if (!boxed) continue;
        Customer* c = S.cust(o.cust);
        Rectangle row{box.x + 14, y, box.width - 28, 62};
        DrawRectangleRounded(row, 0.25f, 6, {96, 100, 112, 255});
        for (int i = 0; i < (int)o.tickets.size(); i++) {   // the boxes
            Rectangle bx{row.x + 10 + i * 58.f, row.y + 10, 50, 42};
            DrawRectangleRec({bx.x - 2, bx.y - 2, bx.width + 4, bx.height + 4}, INK);
            DrawRectangleRec(bx, i < boxed ? Color{240, 232, 214, 255} : Color{110, 114, 126, 255});
            if (i < boxed) { DrawRectangleRec({bx.x, bx.y + 16, bx.width, 6}, ui::RED); DrawCircleV({bx.x + 25, bx.y + 19}, 8, ui::RED); }
        }
        float tx = row.x + 20 + std::max(2, (int)o.tickets.size()) * 58.f;
        snprintf(b, sizeof b, "#%d  %s", o.num, c ? c->who.name.c_str() : "");
        ui::text(b, tx, row.y + 8, 21, ui::PAPER);
        std::string st = o.out ? "On the road" : !o.ready ? "Waiting on the rest" : o.service == DELIVERY ? "Waiting for a driver" :
            (c && c->state == C_WAITING ? "Customer's at the counter!" : "Waiting for the customer");
        ui::text((o.service == DELIVERY ? "Delivery - " : o.service == CARRYOUT ? "Carryout - " : "Counter - ") + st, tx, row.y + 34, 17, o.ready ? Color{170, 230, 170, 255} : Color{200, 204, 214, 255});
        y += 70;
        if (++shown >= 6) break;
    }
    if (!shown) ui::text("Empty.", box.x + box.width / 2, box.y + 230, 22, {150, 156, 170, 255}, 1);
}
}  // namespace

void ovenStation() {
    DrawRectangle(0, 64, 1600, 836, {214, 204, 186, 255});
    for (int i = 0; i < 14; i++) for (int j = 0; j < 8; j++) if ((i + j) & 1) DrawRectangle(i * 120, 64 + j * 110, 120, 110, {206, 196, 178, 255});
    oven();
    cutTable();
    rack();
}
