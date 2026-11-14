// Makeline - the manager's office: your staff, the applications on your desk, and the delivery map.
#include "game.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace pz;
using ui::INK;

namespace {
const char* moodWord(float m) { return m > 0.75f ? "Happy" : m > 0.5f ? "Fine" : m > 0.3f ? "Fed up" : m > 0.15f ? "Miserable" : "About to quit"; }
Color moodColor(float m) { return m > 0.5f ? ui::shade(ui::GREEN, 0.7f) : m > 0.3f ? Color{190, 130, 30, 255} : ui::DKRED; }
const char* seems(float skill) { return skill > 0.8f ? "Sharp. Knows the job." : skill > 0.62f ? "Solid enough." : skill > 0.45f ? "Green, but willing." : "Showed up late to the interview."; }

void face(const Person& p, float x, float y, float r) {   // a little portrait
    static const Color SKIN[] = {{252, 226, 200, 255}, {242, 204, 170, 255}, {222, 174, 132, 255}, {188, 134, 96, 255}, {146, 100, 72, 255}, {100, 68, 50, 255}};
    static const Color HAIR[] = {{84, 56, 38, 255}, {30, 26, 28, 255}, {206, 168, 96, 255}, {160, 76, 42, 255}, {150, 150, 152, 255}, {232, 232, 228, 255}};
    Color hair = HAIR[std::clamp(p.hair, 0, 5)];
    if (p.hairStyle == 3) DrawRectangleRounded({x - r - 3, y - r * 0.3f, 2 * r + 6, r * 1.9f}, 0.5f, 6, hair);
    DrawCircleV({x, y}, r + 2.5f, INK);
    DrawCircleV({x, y}, r, SKIN[std::clamp(p.skin, 0, 5)]);
    if (p.hairStyle >= 1 && p.hairStyle <= 3) DrawCircleSector({x, y - 1}, r + 1.5f, 180, 360, 14, hair);
    if (p.hairStyle == 2) DrawCircleV({x, y - r - 3}, r * 0.34f, hair);
    if (p.hairStyle == 0) DrawRing({x, y}, r - 2, r + 1.5f, 200, 340, 10, hair);
    if (p.hairStyle == 4) { DrawCircleSector({x, y - 2}, r + 2.5f, 180, 360, 14, ui::RED); DrawRectangleRounded({x - r - 3, y - 5, r * 1.5f, 6}, 0.8f, 4, ui::DKRED); }
    DrawCircleV({x - r * 0.36f, y + 1}, r * 0.1f, INK);
    DrawCircleV({x + r * 0.36f, y + 1}, r * 0.1f, INK);
    DrawLineEx({x - r * 0.22f, y + r * 0.5f}, {x + r * 0.22f, y + r * 0.5f}, 2, INK);
    if (p.age > 55) { DrawLineEx({x - r * 0.6f, y - r * 0.25f}, {x - r * 0.2f, y - r * 0.25f}, 1.5f, {120, 90, 70, 255}); DrawLineEx({x + r * 0.2f, y - r * 0.25f}, {x + r * 0.6f, y - r * 0.25f}, 1.5f, {120, 90, 70, 255}); }
}

void staffTab(Rectangle box) {
    Store& S = G.S;
    float y = box.y + 20;
    if (S.staff.empty()) { ui::wrap("Nobody works here but you. Look at the applications.", box.x + 30, y + 20, 800, 24, INK); return; }
    char b[160];
    for (int i = 0; i < (int)S.staff.size(); i++) {
        Worker& w = S.staff[i];
        Rectangle row{box.x + 20, y, box.width - 40, 78};
        DrawRectangleRounded(row, 0.2f, 6, i & 1 ? Color{240, 232, 214, 255} : ui::PAPER);
        face(w.who, row.x + 42, row.y + 42, 26);
        ui::text(w.who.name, row.x + 84, row.y + 8, 24, INK);
        snprintf(b, sizeof b, "%d, %s", w.who.age, roleName(w.role));
        ui::text(b, row.x + 84, row.y + 40, 19, ui::GREY);
        ui::text(moodWord(w.mood), row.x + 330, row.y + 8, 22, moodColor(w.mood));
        snprintf(b, sizeof b, "$%.0f a day%s", w.wage, w.writeUps ? (std::string("   ") + std::to_string(w.writeUps) + (w.writeUps == 1 ? " write-up" : " write-ups")).c_str() : "");
        ui::text(b, row.x + 330, row.y + 40, 19, ui::GREY);
        if (w.role == DRIVER) snprintf(b, sizeof b, w.out ? "Out on a run: %d stop%s" : "In the store", (int)w.stops.size(), w.stops.size() == 1 ? "" : "s");
        else if (w.role == CSR) snprintf(b, sizeof b, w.cust >= 0 ? "Taking an order" : "Waiting for a customer");
        else snprintf(b, sizeof b, w.ticket >= 0 ? "Making a pizza" : "Nothing to make");
        ui::text(b, row.x + 560, row.y + 8, 20, INK);
        if (w.role == DRIVER) snprintf(b, sizeof b, "Today: %d %s, $%.2f tips", w.jobs, w.jobs == 1 ? "delivery" : "deliveries", w.tips);
        else if (w.role == CSR) snprintf(b, sizeof b, "Today: %d orders, yelled at %d times", w.jobs, w.yelled);
        else snprintf(b, sizeof b, "Today: %d pizzas", w.jobs);
        ui::text(b, row.x + 560, row.y + 34, 17, ui::GREY);
        if (w.mistakes) { snprintf(b, sizeof b, "%d mistake%s today", w.mistakes, w.mistakes == 1 ? "" : "s"); ui::text(b, row.x + 560, row.y + 54, 17, ui::DKRED); }
        if (ui::button({row.x + row.width - 290, row.y + 16, 130, 46}, w.wroteUpToday ? "Written up" : "Write up", ui::YELLOW, !w.wroteUpToday, 19)) { S.writeUp(i); sfx(SFX_BAD); }
        if (ui::button({row.x + row.width - 146, row.y + 16, 130, 46}, w.out ? "On a run" : "Fire", ui::RED, !w.out, 19)) { S.note("You fired " + w.who.name + "."); S.fireWorker(i); sfx(SFX_BAD); return; }
        y += 84;
    }
    ui::wrap("A write-up makes them careful for a couple of days and unhappy for longer. Unhappy staff quit: drivers who make no tips, anyone who gets yelled at all night.", box.x + 30, box.y + box.height - 64, box.width - 60, 18, ui::GREY);
}

void hireTab(Rectangle box) {
    Store& S = G.S;
    float y = box.y + 20;
    char b[160];
    ui::text("Applications on your desk (new ones every morning)", box.x + 30, y, 22, ui::GREY);
    y += 40;
    for (int i = 0; i < (int)S.applicants.size(); i++) {
        Worker& w = S.applicants[i];
        Rectangle row{box.x + 20, y, box.width - 40, 78};
        DrawRectangleRounded(row, 0.2f, 6, i & 1 ? Color{240, 232, 214, 255} : ui::PAPER);
        face(w.who, row.x + 42, row.y + 42, 26);
        ui::text(w.who.name, row.x + 84, row.y + 8, 24, INK);
        snprintf(b, sizeof b, "%d years old", w.who.age);
        ui::text(b, row.x + 84, row.y + 40, 19, ui::GREY);
        ui::text(std::string("Wants: ") + roleName(w.role), row.x + 330, row.y + 8, 22, INK);
        ui::text(seems(w.skill), row.x + 330, row.y + 40, 19, ui::GREY);
        snprintf(b, sizeof b, "$%.0f a day", w.wage);
        ui::text(b, row.x + 760, row.y + 22, 24, INK);
        bool room = S.staff.size() < 8;
        if (ui::button({row.x + row.width - 176, row.y + 16, 160, 46}, room ? "Hire" : "Full crew", ui::GREEN, room, 21)) { S.note("You hired " + w.who.name + " (" + roleName(w.role) + ")."); S.hire(i); sfx(SFX_GOOD); return; }
        y += 84;
    }
    if (S.applicants.empty()) ui::text("Nobody else has applied today.", box.x + 30, y, 22, INK);
    ui::wrap("Customer service takes orders at the counter and on the phone. Makeline makes, cuts and boxes. Drivers deliver. They all make mistakes, and their mistakes land on your score.", box.x + 30, box.y + box.height - 64, box.width - 60, 18, ui::GREY);
}

void mapTab(Rectangle box) {
    Store& S = G.S;
    const RoadMap& R = S.roads;
    float cx = box.x + 360, cy = box.y + box.height / 2 + 6, k = 30.5f;   // pixels a mile
    DrawRectangleRounded({cx - 330, cy - 330, 660, 660}, 0.03f, 6, {214, 226, 204, 255});
    DrawRing({cx, cy}, 10 * k - 1, 10 * k + 1, 0, 360, 60, {150, 170, 150, 255});
    auto P = [&](int n) { return Vector2{cx + R.x[n] * k, cy + R.y[n] * k}; };
    for (int pass = 0; pass < 2; pass++)
        for (int n = 0; n < RoadMap::N * RoadMap::N; n++) for (int d = 0; d < 2; d++) {
            unsigned char mph = d ? R.down[n] : R.right[n];
            if (!mph || (mph == 45) != (pass == 1)) continue;
            int m = d ? n + RoadMap::N : n + 1;
            DrawLineEx(P(n), P(m), mph == 45 ? 5 : mph >= 35 ? 3 : 2, mph == 45 ? Color{240, 196, 90, 255} : mph >= 35 ? Color{250, 250, 246, 255} : Color{236, 236, 230, 255});
        }
    DrawRectangleRounded({cx - 12, cy - 12, 24, 24}, 0.3f, 4, INK);
    DrawRectangleRounded({cx - 9, cy - 9, 18, 18}, 0.3f, 4, ui::RED);
    char b[160];
    for (auto& o : S.orders) {   // doors waiting on a pizza
        Customer* c = S.cust(o.cust);
        if (o.done || o.service != DELIVERY || !c || c->addr < 0) continue;
        Vector2 p = P(c->addr);
        DrawCircleV(p, 9, INK); DrawCircleV(p, 6.5f, o.out ? ui::BLUE : o.ready ? ui::GREEN : ui::YELLOW);
        snprintf(b, sizeof b, "#%d", o.num);
        ui::text(b, p.x + 11, p.y - 10, 17, INK);
    }
    float y = box.y + 30, lx = box.x + 730;
    ui::text("DRIVERS", lx, y, 26, INK); y += 40;
    bool any = false;
    for (auto& w : S.staff) {
        if (w.role != DRIVER) continue;
        any = true;
        ui::text(w.who.name, lx, y, 22, INK);
        if (w.out) {
            // where they are along the route right now
            Vector2 at{cx, cy};
            for (size_t i = 1; i < w.route.size(); i++) {
                DrawLineEx({cx + w.route[i - 1].x * k, cy + w.route[i - 1].y * k}, {cx + w.route[i].x * k, cy + w.route[i].y * k}, 2.5f, {86, 134, 190, 160});
                if (w.runT >= w.route[i - 1].at && w.runT <= w.route[i].at) {
                    float u = w.route[i].at > w.route[i - 1].at ? (w.runT - w.route[i - 1].at) / (w.route[i].at - w.route[i - 1].at) : 0;
                    at = {cx + (w.route[i - 1].x + (w.route[i].x - w.route[i - 1].x) * u) * k, cy + (w.route[i - 1].y + (w.route[i].y - w.route[i - 1].y) * u) * k};
                }
            }
            DrawCircleV(at, 10, INK); DrawCircleV(at, 7.5f, ui::BLUE);
            ui::text(w.who.name.substr(0, w.who.name.find(' ')), at.x + 12, at.y + 2, 17, ui::shade(ui::BLUE, 0.6f));
            int left = 0; for (auto& st : w.stops) if (w.runT < st.at) left++;
            snprintf(b, sizeof b, left ? "On the road: %d door%s to go, back in %.0f min" : "Heading back: %.0f min", left ? left : (int)(w.runLen - w.runT), left == 1 ? "" : "s", w.runLen - w.runT);
            if (!left) snprintf(b, sizeof b, "Heading back: %.0f min", w.runLen - w.runT);
            ui::text(b, lx, y + 26, 18, ui::GREY);
        } else ui::text("In the store, waiting for a run", lx, y + 26, 18, ui::GREY);
        snprintf(b, sizeof b, "%d %s, $%.2f in tips", w.jobs, w.jobs == 1 ? "delivery" : "deliveries", w.tips);
        ui::text(b, lx, y + 48, 18, ui::GREY);
        y += 84;
    }
    if (!any) ui::wrap("You have no drivers. Deliveries will sit on the rack until someone quits waiting.", lx, y, 500, 20, ui::DKRED);
    y = box.y + box.height - 170;
    ui::text("The map", lx, y, 22, INK);
    ui::wrap("New roads every shift, out to 10 miles. Yellow roads are 45 mph, the rest 25 to 35, and some streets don't go through. A driver takes up to three orders a run, nearest door first, and each door adds time.", lx, y + 30, 520, 18, ui::GREY);
}
}  // namespace

void officeOverlay() {
    DrawRectangle(-2000, -2000, 5600, 4900, {30, 20, 16, 170});
    Rectangle box{130, 84, 1340, 770};
    ui::panel(box, ui::CREAM, 16, 5, true);
    ui::text("MANAGER'S OFFICE", box.x + 30, box.y + 18, 34, ui::DKRED);
    static const char* TABS[] = {"Staff", "Hiring", "Deliveries"};
    for (int i = 0; i < 3; i++)
        if (ui::button({box.x + 420 + i * 190.f, box.y + 16, 176, 46}, TABS[i], G.officeTab == i ? ui::YELLOW : Color{226, 214, 190, 255}, true, 22)) G.officeTab = i;
    if (ui::button({box.x + box.width - 150, box.y + 16, 130, 46}, "Close", ui::RED, true, 22)) { G.office = false; if (G.playing) setMode(G.mode); return; }
    Rectangle in{box.x, box.y + 74, box.width, box.height - 74};
    if (G.officeTab == 0) staffTab(in); else if (G.officeTab == 1) hireTab(in); else mapTab(in);
}
