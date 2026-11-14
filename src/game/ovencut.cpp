// Makeline - at the cut table: pull a pizza off the end of the oven, cut it, box it.
#include "game.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace pz;
using ui::INK;

void cutOverlay() {
    Store& S = G.S;
    const g3::Frame& F = CUT_F;
    if (ui::button({20, 76, 190, 44}, "Step back (Esc)", ui::CREAM, true, 19)) { setMode(M_WALK); return; }
    Pizza* p = S.pizza(S.cutBoard);
    Vector3 hit;
    bool onUi = ui::M.y > 790 || ui::M.y < 64 || (ui::M.x < 270 && ui::M.y < 400) || ui::clickUsed;
    bool have = !onUi && mouseOnPlane(F.o.y + 0.02f, hit);
    Vector2 l = have ? F.local(hit) : Vector2{9, 9};
    // what's come out of the oven
    Vector2 lab;
    if (project(F.at(0.6f, 0.02f, -0.42f), lab)) ui::text(S.exitQ.empty() ? "nothing out yet" : "OUT OF THE OVEN", lab.x, lab.y, 20, ui::PAPER, 1);
    for (size_t i = 0; i < S.exitQ.size() && i < 2; i++) {
        Pizza* q = S.pizza(S.exitQ[i]);
        float R = g3::pizzaMetres(q->size), lz = i == 0 ? 0.24f : -0.24f, dx = l.x - 0.6f, dz = l.y - lz;
        Ticket* k = S.ticket(q->ticket);
        Vector2 at;
        if (k && project(F.at(0.6f + R + 0.05f, 0.02f, lz), at)) { char b[24]; snprintf(b, sizeof b, "#%d", k->order); ui::text(b, at.x - 10, at.y - 12, 24, ui::PAPER, 2); }
        if (!p && dx * dx + dz * dz < R * R) {
            if (project(F.at(0.6f, 0.03f, lz), at)) DrawRing(at, 150 * R / 0.178f - 4, 150 * R / 0.178f, 0, 360, 40, ui::YELLOW);
            if (ui::pressed) { ui::clickUsed = true; S.toCutBoard(q->id); sfx(SFX_CLICK); G.cutting = false; return; }
        }
    }
    if (S.exitQ.size() > 2 && project(F.at(0.6f, 0.02f, 0.5f), lab)) ui::text("+" + std::to_string(S.exitQ.size() - 2) + " more waiting", lab.x, lab.y, 20, ui::YELLOW, 1);
    if (!p) {
        Vector2 c;
        if (project(F.at(BOARD_X, 0.02f, 0), c)) ui::text(S.exitQ.empty() ? "Waiting on the oven." : "Click a pizza to pull it over.", c.x, c.y - 12, 24, INK, 1);
        return;
    }
    Ticket* k = S.ticket(p->ticket);
    if (k) drawTicket(*k, S.order(k->order), {20, 132, 236, 250}, false, 16);
    // drag the cutter across
    float R = g3::pizzaMetres(p->size);
    Vector2 m{(l.x - BOARD_X) / R, l.y / R};
    bool near = have && m.x * m.x + m.y * m.y < 1.6f * 1.6f;
    if (ui::pressed && near) { G.cutting = true; G.cutFrom = m; }
    if (G.cutting) {
        Vector2 a;
        if (project(F.at(BOARD_X + G.cutFrom.x * R, 0.03f, G.cutFrom.y * R), a)) { DrawLineEx(a, ui::M, 6, INK); DrawLineEx(a, ui::M, 2.5f, {240, 240, 244, 255}); }
        if (!ui::down) {
            G.cutting = false;
            size_t n = p->cuts.size();
            if (have) p->addCut(G.cutFrom.x, G.cutFrom.y, m.x, m.y);
            if (p->cuts.size() > n) sfx(SFX_CUT);
        }
    }
    char b[64];
    snprintf(b, sizeof b, "%d cut%s", (int)p->cuts.size(), p->cuts.size() == 1 ? "" : "s");
    DrawRectangle(0, 790, 1600, 110, {40, 30, 26, 150});
    ui::text(b, 40, 812, 26, ui::PAPER);
    ui::text("Drag the cutter right across.", 40, 846, 18, {220, 210, 196, 255});
    if (ui::button({330, 806, 260, 70}, "BOX IT", ui::RED, true, 28)) { S.boxCut(); sfx(SFX_DING); G.cutting = false; }
    if (ui::button({612, 816, 220, 52}, "Start the cut over", {226, 214, 190, 255}, !p->cuts.empty(), 19)) p->cuts.clear();
    if (ui::button({852, 816, 220, 52}, "Trash & remake", {200, 190, 176, 255}, true, 19)) { S.trashCut(); sfx(SFX_BAD); }
}
