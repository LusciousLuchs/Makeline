// Makeline - the shop in 3D: the room, the counter, the makeline, the oven, the cut table, the heat rack, the people,
// and you walking around behind the counter.
#include "game.hpp"
#include "rlgl.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

using namespace pz;
using ui::INK;

const g3::Frame MAKE_F{{-4.5f, 0.92f, 2.9f}, 90};
const g3::Frame CUT_F{{2.55f, 0.92f, 5.45f}, 180};

namespace {
const float EYE = 1.62f;
// the register's monitor: a 4:3 screen on a stand, tilted back 18 degrees, showing the order terminal
const float MON_W = 0.64f, MON_H = 0.48f, TILT = -18.f, D2RR = 0.0174533f;
const Vector3 MON_C{-0.55f, 1.43f, 0.02f};
Vector3 monN() { return {0, -std::sin(TILT * D2RR), std::cos(TILT * D2RR)}; }
Vector3 monUp() { return {0, std::cos(TILT * D2RR), std::sin(TILT * D2RR)}; }
const Vector3 REG_SPOT{-1.2f, 0, -0.95f}, DOOR{3.6f, 0, -5.7f}, PHONE{1.6f, 1.06f, 0.02f}, REGISTER{-0.55f, 1.2f, 0.05f};
struct Target { int kind; int id; Vector3 at; float reach; std::string label; };   // kind 0 register, 1 phone, 2 makeline, 3 cut table, 4 a customer
int facing = -1;
std::vector<Target> targets;

Vector3 lineSpot(int i) { return {REG_SPOT.x, 0, REG_SPOT.z - 0.85f * i}; }
Vector3 lobbySpot(int j) { return {0.5f + (j % 4) * 0.95f, 0, -1.45f - (j / 4) * 1.1f - (j % 2) * 0.25f}; }

Camera3D wanted() {
    Camera3D c{};
    c.projection = CAMERA_PERSPECTIVE;
    c.up = {0, 1, 0};
    if (G.mode == M_MAKE) {
        c.position = MAKE_F.at(0, 1.18f, 0.3f); c.target = MAKE_F.at(0, 0, 0.02f); c.up = MAKE_F.dir(0, 0, -1); c.fovy = 45;
    } else if (G.mode == M_CUT) {
        c.position = CUT_F.at(0, 1.3f, 0.32f); c.target = CUT_F.at(0, 0, 0.02f); c.up = CUT_F.dir(0, 0, -1); c.fovy = 45;
    } else if (G.mode == M_REGISTER) {
        // straight at the monitor, placed so the terminal fills the right side of the screen and the customer
        // and what they're saying fit on the left
        Vector3 n = monN(), up = monUp();
        float W = (float)GetScreenWidth(), H = (float)GetScreenHeight(), tanV = std::tan(45 * 0.5f * D2RR);
        Vector2 ra = ui::toScreen({918, 104}), rb = ui::toScreen({1584, 604});
        float rh = rb.y - ra.y, cx = (ra.x + rb.x) / 2, cy = (ra.y + rb.y) / 2;
        float d = H * MON_H / (2 * tanV * rh);
        Vector3 sc{MON_C.x + n.x * 0.0255f, MON_C.y + n.y * 0.0255f, MON_C.z + n.z * 0.0255f};   // the glass, a little proud of the bezel
        float X = (cx - W / 2) / (H / 2) * d * tanV, Y = (H / 2 - cy) / (H / 2) * d * tanV;
        Vector3 axis{sc.x - X, sc.y - up.y * Y, sc.z - up.z * Y};
        c.position = {axis.x + n.x * d, axis.y + n.y * d, axis.z + n.z * d};
        c.target = axis; c.up = up; c.fovy = 45;
    } else {
        c.position = {G.px, EYE, G.pz};
        float cp = std::cos(G.pitch);
        c.target = {G.px + std::sin(G.yaw) * cp, EYE + std::sin(G.pitch), G.pz + std::cos(G.yaw) * cp};
        c.fovy = 68;
    }
    return c;
}

void people(float dt) {
    Store& S = G.S;
    auto line = S.line();
    auto lobby = S.lobby();
    Customer* talking = S.talk.active() ? S.cust(S.talk.cust) : nullptr;
    bool atReg = talking && !talking->phone;
    for (auto& c : S.custs) {
        if (c.phone && c.state != C_WAITING && !(c.state == C_DONE && c.leaving > 0)) continue;
        Vis* v = nullptr;
        for (auto& p : G.people) if (p.id == c.id) v = &p;
        bool leaving = c.state == C_DONE;
        if (!v) {
            if (leaving) continue;
            G.people.push_back({c.id, DOOR.x, DOOR.z, 0, 0, false});
            v = &G.people.back();
        }
        Vector3 goal = DOOR;
        if (!leaving) {
            if (c.state == C_ORDERING) goal = c.server >= 0 ? Vector3{-2.6f - 1.2f * (c.server % 2), 0, REG_SPOT.z} : lineSpot(0);
            else if (c.state == C_INLINE) { auto it = std::find(line.begin(), line.end(), c.id); goal = lineSpot((int)(it - line.begin()) + (atReg ? 1 : 0)); }
            else { auto it = std::find(lobby.begin(), lobby.end(), c.id); goal = lobbySpot((int)(it - lobby.begin())); }
        }
        float dx = goal.x - v->x, dz = goal.z - v->z, d = std::sqrt(dx * dx + dz * dz), speed = leaving && c.leftStars < 3 ? 2.4f : 1.5f;
        if (d > 0.04f) {
            float step = std::min(d, speed * dt);
            v->x += dx / d * step; v->z += dz / d * step;
            v->yaw = std::atan2(dx, dz) * 57.2958f;
            v->walk += dt;
        } else {
            v->walk = 0;
            float face = 0, diff = face - v->yaw;   // turn to face the counter
            while (diff > 180) diff -= 360;
            while (diff < -180) diff += 360;
            v->yaw += diff * std::min(1.f, dt * 8);
            if (leaving) v->out = true;
        }
    }
    G.people.erase(std::remove_if(G.people.begin(), G.people.end(), [&](const Vis& v) { return v.out || !S.cust(v.id); }), G.people.end());
}

void findTargets() {
    Store& S = G.S;
    targets.clear();
    if (!S.talk.active()) {
        if (S.firstInLine() >= 0) targets.push_back({0, S.firstInLine(), REGISTER, 2.8f, "Take the next order"});
        else targets.push_back({0, -1, REGISTER, 2.8f, "Check the orders"});
        if (S.ringing() >= 0) targets.push_back({1, S.ringing(), PHONE, 2.8f, "Answer the phone"});
    } else targets.push_back({0, -1, REGISTER, 2.8f, "Back to the customer"});
    int open = (int)S.openTickets().size();
    targets.push_back({2, -1, {MAKE_F.o.x, 1.0f, MAKE_F.o.z}, 2.6f, S.bench >= 0 ? "Back to the pizza" : open ? "Make a pizza" : "The makeline"});
    targets.push_back({3, -1, {3.0f, 1.0f, 5.45f}, 2.8f, S.cutBoard >= 0 || !S.exitQ.empty() ? "Cut and box" : "The cut table"});
    for (auto& v : G.people) {
        Customer* c = S.cust(v.id);
        Order* o = c && c->state == C_WAITING ? S.order(c->order) : nullptr;
        if (o && o->ready && !o->done) { char b[64]; snprintf(b, sizeof b, "Hand over order #%d", o->num); targets.push_back({4, c->id, {v.x, 1.45f, v.z}, 7.5f, b}); }
    }
}

void interact(const Target& t) {
    Store& S = G.S;
    switch (t.kind) {
        case 0: if (t.id >= 0) S.beginTalk(t.id); setMode(M_REGISTER); break;
        case 1: if (S.beginTalk(t.id)) setMode(M_REGISTER); break;
        case 2: setMode(M_MAKE); break;
        case 3: setMode(M_CUT); break;
        case 4: S.handOut(t.id); break;
    }
    sfx(SFX_CLICK);
}
}  // namespace

void setMode(int mode) {
    G.mode = mode;
    G.cutting = false; G.chart = false;
    if (gShotMode) return;
    if (mode == M_WALK) DisableCursor(); else EnableCursor();
}

void worldReset() {
    G.px = 0; G.pz = 2.4f; G.yaw = 3.14159f; G.pitch = -0.08f;
    G.people.clear();
    G.camSnap = true;
    setMode(M_WALK);
}

bool project(Vector3 w, Vector2& out) {
    Vector3 f{G.cam.target.x - G.cam.position.x, G.cam.target.y - G.cam.position.y, G.cam.target.z - G.cam.position.z};
    Vector3 d{w.x - G.cam.position.x, w.y - G.cam.position.y, w.z - G.cam.position.z};
    if (f.x * d.x + f.y * d.y + f.z * d.z < 0.15f) return false;
    out = ui::fromScreen(GetWorldToScreen(w, G.cam));
    return true;
}
bool mouseOnPlane(float y, Vector3& hit) {
    Ray r = GetScreenToWorldRay(ui::toScreen(ui::M), G.cam);
    if (std::fabs(r.direction.y) < 1e-4f) return false;
    float t = (y - r.position.y) / r.direction.y;
    if (t < 0) return false;
    hit = {r.position.x + r.direction.x * t, y, r.position.z + r.direction.z * t};
    return true;
}
Vector2 binSpot(int ing) {
    const float P = BIN + 0.012f;
    if (ing < MOZZ) return {-0.44f + (ing % 2) * P, -0.304f + (ing / 2) * P};                      // sauces: top left
    if (ing < ONION) { int k = ing - MOZZ; return {-0.44f + (k % 2) * P, 0.0f + (k / 2) * P}; }       // cheeses: below them
    int k = ing - ONION;                                                                           // vegetables, then meats: 3 across
    return {0.40f + (k % 3) * P, -0.228f + (k / 3) * P};
}

bool posMouse(Vector2& out) {
    Ray r = GetScreenToWorldRay(ui::toScreen(ui::M), G.cam);
    Vector3 n = monN(), up = monUp();
    float den = r.direction.x * n.x + r.direction.y * n.y + r.direction.z * n.z;
    if (std::fabs(den) < 1e-5f) return false;
    Vector3 sc{MON_C.x + n.x * 0.0255f, MON_C.y + n.y * 0.0255f, MON_C.z + n.z * 0.0255f};
    float t = ((sc.x - r.position.x) * n.x + (sc.y - r.position.y) * n.y + (sc.z - r.position.z) * n.z) / den;
    if (t < 0) return false;
    Vector3 d{r.position.x + r.direction.x * t - sc.x, r.position.y + r.direction.y * t - sc.y, r.position.z + r.direction.z * t - sc.z};
    float u = d.x / MON_W + 0.5f, v = 0.5f - (d.x * up.x + d.y * up.y + d.z * up.z) / MON_H;
    if (u < 0 || u > 1 || v < 0 || v > 1) return false;
    out = {u * 1024, v * 768};
    return true;
}

namespace {
RenderTexture2D gSign{};
bool gSignMade = false;
}
Texture2D signTexture() {
    if (!gSignMade) { gSign = LoadRenderTexture(512, 266); SetTextureFilter(gSign.texture, TEXTURE_FILTER_BILINEAR); gSignMade = true; }
    return gSign.texture;
}
void signRender() {
    signTexture();
    Store& S = G.S;
    BeginTextureMode(gSign);
    ClearBackground({18, 20, 26, 255});
    ui::text(S.clock(), 22, 8, 64, {255, 206, 90, 255});
    ui::text("Shift ends " + Store::timeText(S.shiftEnd()), 24, 84, 24, {200, 204, 214, 255});
    float rt = S.rating();
    ui::stars(20, 118, 40, rt, {70, 72, 84, 255});
    char b[48];
    if (rt > 0) snprintf(b, sizeof b, "%.1f", rt); else snprintf(b, sizeof b, "no reviews");
    ui::text(b, 262, 124, 30, ui::PAPER);
    int rp = S.reprimandsThisWeek();
    if (rp) { snprintf(b, sizeof b, "Reprimands %d/3", rp); ui::text(b, 490, 124, 24, {240, 100, 90, 255}, 2); }
    if (!S.reviews.empty()) {
        auto& r = S.reviews.back();
        DrawRectangle(10, 170, 492, 2, {60, 64, 76, 255});
        ui::wrap("\"" + r.text + "\"", 20, 178, 472, 22, {200, 204, 214, 255}, true, 90);
    }
    EndTextureMode();
}

void worldUpdate(float dt) {
    Store& S = G.S;
    people(dt);
    if (G.mode == M_WALK && !G.paused) {
        if (!gShotMode) {
            Vector2 md = GetMouseDelta();
            G.yaw -= md.x * 0.0026f;
            G.pitch = std::clamp(G.pitch - md.y * 0.0026f, -1.3f, 1.2f);
        }
        float f = (IsKeyDown(KEY_W) ? 1 : 0) - (IsKeyDown(KEY_S) ? 1 : 0), s = (IsKeyDown(KEY_D) ? 1 : 0) - (IsKeyDown(KEY_A) ? 1 : 0);
        float len = std::sqrt(f * f + s * s);
        if (len > 0) {
            float sp = (IsKeyDown(KEY_LEFT_SHIFT) ? 5.2f : 3.2f) * dt / len;
            G.px += (std::sin(G.yaw) * f - std::cos(G.yaw) * s) * sp;
            G.pz += (std::cos(G.yaw) * f + std::sin(G.yaw) * s) * sp;
        }
        G.px = std::clamp(G.px, -3.75f, 4.05f);
        G.pz = std::clamp(G.pz, 0.72f, 4.45f);
        // what you're facing
        findTargets();
        facing = -1;
        float best = 0.42f;
        Vector3 eye{G.px, EYE, G.pz}, fw{std::sin(G.yaw) * std::cos(G.pitch), std::sin(G.pitch), std::cos(G.yaw) * std::cos(G.pitch)};
        for (size_t i = 0; i < targets.size(); i++) {
            Vector3 d{targets[i].at.x - eye.x, targets[i].at.y - eye.y, targets[i].at.z - eye.z};
            float dist = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
            if (dist > targets[i].reach || dist < 0.01f) continue;
            float ang = std::acos(std::clamp((d.x * fw.x + d.y * fw.y + d.z * fw.z) / dist, -1.f, 1.f));
            if (targets[i].kind == 4) ang *= 1.6f;   // people need a steadier aim than a whole table
            if (ang < best) { best = ang; facing = (int)i; }
        }
        G.prompt = facing >= 0 ? targets[facing].label : "";
        if (facing >= 0 && (IsKeyPressed(KEY_E) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) interact(targets[facing]);
    } else G.prompt.clear();
    (void)S;
    // the camera glides between you and the stations
    Camera3D w = wanted();
    float k = G.camSnap ? 1 : 1 - std::exp(-dt * (G.mode == M_WALK ? 30.f : 9.f));
    G.camSnap = false;
    auto mix = [&](Vector3& a, Vector3 b) { a.x += (b.x - a.x) * k; a.y += (b.y - a.y) * k; a.z += (b.z - a.z) * k; };
    mix(G.cam.position, w.position); mix(G.cam.target, w.target); mix(G.cam.up, w.up);
    G.cam.fovy += (w.fovy - G.cam.fovy) * k;
    G.cam.projection = CAMERA_PERSPECTIVE;
}

namespace {
void room() {
    Store& S = G.S;
    using g3::box;
    // floors: checkered tile out front, quarry tile in the kitchen
    for (int i = -9; i < 9; i++) for (int j = -10; j < 0; j++)
        g3::quadUp({i * 0.6f + 0.3f, 0, j * 0.6f - 0.05f}, 0.6f, 0.6f, (i + j) & 1 ? Color{196, 62, 48, 255} : Color{240, 228, 206, 255});
    for (int i = -5; i < 5; i++) for (int j = 0; j < 6; j++)
        g3::quadUp({i * 1.04f + 0.52f, 0, j * 1.04f + 0.37f}, 1.04f, 1.04f, (i + j) & 1 ? Color{150, 100, 80, 255} : Color{158, 108, 86, 255});
    // walls
    Color wall{238, 214, 170, 255}, kwall{226, 228, 226, 255};
    box({0, 1.5f, 6.15f}, {10.4f, 3, 0.1f}, kwall);
    box({-5.15f, 1.5f, 3.2f}, {0.1f, 3, 6}, kwall);
    box({5.15f, 1.5f, 3.2f}, {0.1f, 3, 6}, kwall);
    box({-5.15f, 1.5f, -3}, {0.1f, 3, 6.4f}, wall);
    box({5.15f, 1.5f, -3}, {0.1f, 3, 6.4f}, wall);
    box({0, 1.5f, -6.15f}, {10.4f, 3, 0.1f}, wall);
    box({0, 2.72f, -6.08f}, {10.3f, 0.5f, 0.06f}, {206, 60, 46, 255});
    rlBegin(RL_TRIANGLES);   // the ceiling, evenly lit
    rlNormal3f(0, 0, 0); rlColor4ub(226, 220, 208, 255);
    rlVertex3f(-5.2f, 3, -6.2f); rlVertex3f(5.2f, 3, -6.2f); rlVertex3f(5.2f, 3, 6.2f);
    rlVertex3f(-5.2f, 3, -6.2f); rlVertex3f(5.2f, 3, 6.2f); rlVertex3f(-5.2f, 3, 6.2f);
    rlColor4ub(255, 255, 246, 255);
    for (int i = 0; i < 3; i++) for (int j = 0; j < 4; j++) {
        float x = -3.2f + i * 3.2f, z = -4.5f + j * 3.0f;
        rlVertex3f(x - 0.6f, 2.995f, z - 0.25f); rlVertex3f(x + 0.6f, 2.995f, z - 0.25f); rlVertex3f(x + 0.6f, 2.995f, z + 0.25f);
        rlVertex3f(x - 0.6f, 2.995f, z - 0.25f); rlVertex3f(x + 0.6f, 2.995f, z + 0.25f); rlVertex3f(x - 0.6f, 2.995f, z + 0.25f);
    }
    rlEnd();
    // the front: windows onto the street and the door
    float dusk = std::clamp((S.clockMin() - 17.5f * 60) / (3 * 60), 0.f, 1.f);   // sunset around 7 PM, dark by 8:30
    Color sky = ui::mix({150, 204, 240, 255}, {60, 50, 100, 255}, dusk), low = ui::mix({250, 220, 160, 255}, {236, 120, 80, 255}, dusk);
    for (int i = 0; i < 2; i++) {
        float x = -3.2f + i * 3.0f;
        box({x, 1.75f, -6.09f}, {2.5f, 1.7f, 0.04f}, {60, 44, 38, 255});
        box({x, 2.05f, -6.07f}, {2.34f, 0.94f, 0.04f}, sky);
        box({x, 1.3f, -6.07f}, {2.34f, 0.6f, 0.04f}, low);
        box({x, 1.02f, -6.065f}, {2.34f, 0.2f, 0.04f}, {70, 70, 80, 255});
        box({x, 1.75f, -6.05f}, {0.06f, 1.6f, 0.04f}, {60, 44, 38, 255});
    }
    {   // a digital sign on the front wall: the time, your rating and the latest review
        box({2.0f, 2.0f, -6.07f}, {1.78f, 0.93f, 0.05f}, {30, 30, 36, 255});
        rlSetTexture(signTexture().id);
        rlBegin(RL_QUADS);
        rlColor4ub(255, 255, 255, 255); rlNormal3f(0, 0, 0);
        rlTexCoord2f(0, 0); rlVertex3f(1.12f, 1.56f, -6.04f);
        rlTexCoord2f(1, 0); rlVertex3f(2.88f, 1.56f, -6.04f);
        rlTexCoord2f(1, 1); rlVertex3f(2.88f, 2.44f, -6.04f);
        rlTexCoord2f(0, 1); rlVertex3f(1.12f, 2.44f, -6.04f);
        rlEnd();
        rlSetTexture(0);
    }
    box({DOOR.x, 1.1f, -6.09f}, {1.2f, 2.2f, 0.06f}, {60, 44, 38, 255});
    box({DOOR.x, 1.3f, -6.06f}, {1.0f, 1.6f, 0.04f}, low);
    box({DOOR.x - 0.4f, 1.05f, -6.02f}, {0.05f, 0.3f, 0.05f}, {220, 200, 120, 255});
    // the menu board over the lobby's side wall, a bench, a plant
    box({-5.08f, 2.0f, -2.6f}, {0.06f, 1.0f, 2.6f}, {50, 60, 56, 255});
    for (int i = 0; i < 4; i++) box({-5.04f, 2.3f - i * 0.2f, -2.6f}, {0.02f, 0.06f, 2.0f - i * 0.25f}, {240, 232, 210, 255});
    box({-5.04f, 2.42f, -2.6f}, {0.02f, 0.1f, 1.2f}, {246, 194, 66, 255});
    box({-4.6f, 0.42f, -4.6f}, {0.5f, 0.08f, 2.0f}, {184, 128, 82, 255});
    box({-4.6f, 0.2f, -5.4f}, {0.4f, 0.4f, 0.08f}, {134, 88, 56, 255});
    box({-4.6f, 0.2f, -3.8f}, {0.4f, 0.4f, 0.08f}, {134, 88, 56, 255});
    g3::cyl({4.5f, 0, -1.2f}, 0.22f, 0.45f, {170, 96, 70, 255}, 10, 0.28f);
    g3::ball({4.5f, 0.8f, -1.2f}, 0.38f, {70, 140, 80, 255}, 1.2f);
    // dark where things meet the floor
    g3::softShadow({0, 0.003f, 0}, 10.2f, 0.62f, 0.5f, 110);
    g3::softShadow({-4.6f, 0.003f, 2.9f}, 0.98f, 3.5f, 0.45f, 110);
    g3::softShadow({-0.7f, 0.003f, 5.4f}, 3.8f, 1.0f, 0.5f, 120);
    g3::softShadow({3.3f, 0.003f, 5.45f}, 1.9f, 0.95f, 0.45f, 110);
    g3::softShadow({4.75f, 0.003f, 2.3f}, 0.56f, 2.44f, 0.4f, 90);
    g3::softShadow({-4.6f, 0.003f, -4.6f}, 0.5f, 2.0f, 0.3f, 90);
    for (float z : {-6.1f, 6.1f}) g3::softShadow({0, 0.003f, z}, 10.4f, 0.1f, 0.6f, 70);
    for (float x : {-5.1f, 5.1f}) g3::softShadow({x, 0.003f, 0}, 0.1f, 12.4f, 0.6f, 70);
    // the lobby: a drinks fridge, a table and chairs, a bin, a mat at the door
    box({4.6f, 0.95f, -3.4f}, {0.7f, 1.9f, 0.8f}, {70, 74, 84, 255});
    box({4.24f, 1.05f, -3.4f}, {0.02f, 1.5f, 0.66f}, {170, 210, 230, 255});
    for (int r = 0; r < 4; r++) for (int c = 0; c < 4; c++) g3::cyl({4.3f, 0.36f + r * 0.38f, -3.66f + c * 0.17f}, 0.035f, 0.14f, (r + c) % 3 == 0 ? Color{206, 60, 46, 255} : (r + c) % 3 == 1 ? Color{60, 120, 190, 255} : Color{240, 190, 60, 255}, 8);
    g3::softShadow({4.6f, 0.003f, -3.4f}, 0.7f, 0.8f, 0.35f, 110);
    g3::cyl({-2.6f, 0, -4.4f}, 0.06f, 0.72f, {90, 92, 100, 255}, 8);
    g3::cyl({-2.6f, 0.72f, -4.4f}, 0.5f, 0.04f, {206, 60, 46, 255}, 20);
    g3::softShadow({-2.6f, 0.003f, -4.4f}, 0.7f, 0.7f, 0.35f, 80);
    for (int i = 0; i < 2; i++) { float x = -3.3f + i * 1.4f; box({x, 0.44f, -4.4f}, {0.4f, 0.05f, 0.4f}, {230, 224, 210, 255}); box({x + (i ? 0.19f : -0.19f), 0.68f, -4.4f}, {0.04f, 0.5f, 0.4f}, {230, 224, 210, 255}); for (float dx : {-0.17f, 0.17f}) for (float dz : {-0.17f, 0.17f}) box({x + dx, 0.21f, -4.4f + dz}, {0.03f, 0.42f, 0.03f}, {90, 92, 100, 255}); }
    g3::cyl({2.4f, 0, -5.6f}, 0.2f, 0.7f, {84, 88, 96, 255}, 12, 0.23f);
    box({DOOR.x, 0.006f, -5.4f}, {1.4f, 0.012f, 0.8f}, {60, 54, 50, 255});
    for (float x = -4.4f; x < 5; x += 2.2f) box({x, 0.06f, -6.08f}, {2.2f, 0.12f, 0.04f}, {120, 84, 60, 255});   // skirting
    // the kitchen: tiled walls to shoulder height
    box({0, 0.75f, 6.09f}, {10.3f, 1.5f, 0.02f}, {236, 238, 236, 255});
    for (float x = -5; x <= 5; x += 0.5f) box({x, 0.75f, 6.075f}, {0.012f, 1.5f, 0.012f}, {200, 204, 204, 255});
    for (float y = 0.5f; y < 1.6f; y += 0.5f) box({0, y, 6.075f}, {10.3f, 0.012f, 0.012f}, {200, 204, 204, 255});
    // the counter, with the register and the phone on it
    box({0, 0.5f, 0}, {10.2f, 1.0f, 0.62f}, {176, 120, 76, 255});
    box({0, 1.02f, 0}, {10.2f, 0.05f, 0.74f}, {232, 226, 214, 255});
    box({0, 0.5f, -0.33f}, {10.2f, 0.16f, 0.04f}, {206, 60, 46, 255});
    for (int i = 0; i < 6; i++) {   // shelves under the counter on your side: boxes, cups, bags
        float x = -4.2f + i * 1.7f;
        box({x, 0.5f, 0.32f}, {1.5f, 0.8f, 0.02f}, {120, 80, 52, 255});
        box({x, 0.62f, 0.33f}, {1.5f, 0.03f, 0.04f}, {150, 104, 68, 255});
        for (int k = 0; k < 3; k++) box({x - 0.45f + k * 0.45f, 0.75f, 0.335f}, {0.3f, 0.2f + (k + i) % 2 * 0.06f, 0.03f}, (k + i) % 3 == 0 ? Color{236, 226, 206, 255} : (k + i) % 3 == 1 ? Color{206, 60, 46, 255} : Color{190, 170, 140, 255});
    }
    box({REGISTER.x, 1.09f, 0.08f}, {0.34f, 0.08f, 0.3f}, {60, 64, 74, 255});
    rlPushMatrix();
    rlTranslatef(MON_C.x, MON_C.y, MON_C.z);
    rlRotatef(TILT, 1, 0, 0);
    box({0, 0, 0}, {MON_W + 0.06f, MON_H + 0.06f, 0.05f}, {40, 44, 52, 255});
    rlSetTexture(posTexture().id);   // the order terminal, drawn each frame into its own picture
    rlBegin(RL_QUADS);
    rlColor4ub(255, 255, 255, 255); rlNormal3f(0, 0, 0);
    rlTexCoord2f(0, 0); rlVertex3f(-MON_W / 2, -MON_H / 2, 0.0255f);
    rlTexCoord2f(1, 0); rlVertex3f(MON_W / 2, -MON_H / 2, 0.0255f);
    rlTexCoord2f(1, 1); rlVertex3f(MON_W / 2, MON_H / 2, 0.0255f);
    rlTexCoord2f(0, 1); rlVertex3f(-MON_W / 2, MON_H / 2, 0.0255f);
    rlEnd();
    rlSetTexture(0);
    rlPopMatrix();
    box({MON_C.x, 1.17f, 0.06f}, {0.06f, 0.2f, 0.05f}, {60, 64, 74, 255});
    bool ring = S.ringing() >= 0, onPhone = S.talk.active() && S.cust(S.talk.cust) && S.cust(S.talk.cust)->phone;
    float j = ring ? std::sin(ui::time * 70) * 0.008f : 0;
    box({PHONE.x + j, 1.08f, PHONE.z}, {0.2f, 0.06f, 0.24f}, {54, 54, 62, 255});
    if (!onPhone) box({PHONE.x + j, 1.13f + (ring ? std::fabs(std::sin(ui::time * 30)) * 0.012f : 0), PHONE.z}, {0.07f, 0.045f, 0.26f}, {206, 60, 46, 255});
    box({PHONE.x + j, 1.112f, PHONE.z + 0.02f}, {0.1f, 0.004f, 0.1f}, ring ? Color{255, 230, 120, 255} : Color{190, 194, 204, 255});
}

void makeline() {
    Store& S = G.S;
    using g3::box;
    const g3::Frame& F = MAKE_F;
    box({-4.6f, 0.44f, 2.9f}, {0.98f, 0.88f, 3.5f}, {150, 156, 164, 255});
    box({-4.6f, 0.9f, 2.9f}, {1.02f, 0.04f, 3.54f}, {206, 212, 218, 255});
    box({-5.06f, 1.2f, 2.9f}, {0.06f, 0.56f, 3.54f}, {190, 196, 202, 255});
    // the well the pans sit in
    Vector3 w0 = F.at(0, -0.004f, 0);
    box({w0.x, w0.y - 0.02f + 0.0235f, w0.z}, {0.98f, 0.002f, 1.5f}, {176, 182, 190, 255});
    for (int ing = 0; ing < ING_N; ing++) {
        Vector2 b = binSpot(ing);
        g3::binFill(ing, F, b.x, b.y, BIN);
        if (G.tool == ing && G.mode == M_MAKE) {
            Vector3 c = F.at(b.x, 0.006f, b.y);
            g3::ring(c, BIN * 0.70f, BIN * 0.80f, {214, 66, 50, 255}, 45, 405, 4);
        }
    }
    // the board, and the pizza being made
    Vector3 bc = F.at(BENCH_X, 0, 0);
    g3::cyl({bc.x, bc.y - 0.008f, bc.z}, 0.235f, 0.008f, {206, 170, 120, 255}, 28);
    if (Pizza* p = S.pizza(S.bench)) g3::pizza(*p, F, BENCH_X, 0);
    // dough trays at the far end, the ticket rail on the wall
    for (int i = 0; i < 3; i++) {
        box({-4.6f, 0.95f + i * 0.09f, 4.25f}, {0.7f, 0.08f, 0.5f}, i == 2 ? Color{90, 130, 180, 255} : Color{80, 118, 166, 255});
    }
    for (int i = 0; i < 3; i++) g3::cyl({-4.78f + i * 0.2f, 1.19f, 4.25f}, 0.07f, 0.03f, {244, 230, 198, 255}, 10);
    box({-5.0f, 1.62f, 2.9f}, {0.05f, 0.04f, 3.2f}, {120, 126, 134, 255});
    auto open = S.openTickets();
    for (size_t i = 0; i < open.size() && i < 12; i++) {
        Ticket* k = S.ticket(open[i]);
        Order* o = S.order(k->order);
        Color svc = !o ? ui::GREY : o->service == DELIVERY ? ui::BLUE : o->service == CARRYOUT ? ui::GREEN : ui::YELLOW;
        float z = 1.5f + (float)i * 0.24f;
        box({-4.985f, 1.47f, z}, {0.01f, 0.26f, 0.18f}, {255, 250, 238, 255});
        box({-4.978f, 1.58f, z}, {0.01f, 0.05f, 0.18f}, svc);
    }
}

void ovenAndCut() {
    Store& S = G.S;
    using g3::box;
    // the belt runs the length of the back wall, through the oven
    box({-0.45f, 0.89f, 5.4f}, {5.7f, 0.06f, 0.92f}, {70, 72, 78, 255});
    float off = std::fmod(ui::time * 0.19f, 0.3f);
    for (float x = -3.3f + off; x < 2.4f; x += 0.3f) box({x, 0.921f, 5.4f}, {0.02f, 0.002f, 0.9f}, {40, 40, 46, 255});
    for (float x : {-3.2f, 2.3f}) for (float z : {5.0f, 5.8f}) box({x, 0.43f, z}, {0.06f, 0.86f, 0.06f}, {120, 126, 134, 255});
    box({-0.7f, 0.43f, 5.4f}, {3.8f, 0.86f, 1.0f}, {128, 134, 142, 255});
    box({-0.7f, 1.52f, 5.4f}, {3.8f, 0.6f, 1.04f}, {138, 144, 152, 255});
    box({-0.7f, 1.07f, 5.88f}, {3.8f, 0.3f, 0.06f}, {250, 130, 40, 255});                       // the glow at the back
    for (float x = -2.4f; x < 1.1f; x += 0.5f) box({x, 1.2f, 5.4f}, {0.24f, 0.02f, 0.9f}, {255, (unsigned char)(150 + 60 * std::sin(ui::time * 6 + x * 5)), 50, 255});
    for (float x : {-2.6f, 1.2f}) box({x, 1.07f, 4.92f}, {0.08f, 0.3f, 0.06f}, {110, 116, 124, 255});
    box({-0.7f, 1.6f, 4.87f}, {1.4f, 0.14f, 0.02f}, {60, 64, 72, 255});
    for (auto& s : S.oven) if (Pizza* p = S.pizza(s.pizza)) {
        g3::Frame f{{-3.0f + s.prog * 4.6f, 0.922f, 5.4f}, 180};
        g3::pizza(*p, f, 0, 0, std::clamp(s.prog * 1.1f, 0.f, 1.f));
    }
    // what's come out waits on the end of the belt
    for (size_t i = 0; i < S.exitQ.size() && i < 2; i++) if (Pizza* p = S.pizza(S.exitQ[i])) g3::pizza(*p, CUT_F, 0.6f, i == 0 ? 0.24f : -0.24f);
    // the cut table
    box({3.3f, 0.44f, 5.45f}, {1.9f, 0.88f, 0.95f}, {150, 156, 164, 255});
    box({3.3f, 0.9f, 5.45f}, {1.94f, 0.04f, 0.99f}, {214, 190, 150, 255});
    Vector3 bc = CUT_F.at(BOARD_X, 0, 0);
    g3::cyl({bc.x, bc.y, bc.z}, 0.25f, 0.01f, {196, 150, 96, 255}, 28);
    if (Pizza* p = S.pizza(S.cutBoard)) { g3::Frame f = CUT_F; f.o.y += 0.01f; g3::pizza(*p, f, BOARD_X, 0); }
    for (int i = 0; i < 5; i++) box({4.0f, 0.93f + i * 0.012f, 5.5f}, {0.42f, 0.01f, 0.42f}, {230, 220, 200, 255}, i * 3.f);   // flat boxes
    // the rocker knife, hung on the wall
    box({3.6f, 1.5f, 6.08f}, {0.5f, 0.1f, 0.02f}, {200, 204, 210, 255});
    box({3.6f, 1.57f, 6.06f}, {0.4f, 0.05f, 0.04f}, {60, 50, 46, 255});
}

void rack() {
    Store& S = G.S;
    using g3::box;
    for (float z : {1.1f, 3.5f}) for (float x : {4.5f, 5.0f}) box({x, 0.95f, z}, {0.05f, 1.9f, 0.05f}, {120, 126, 134, 255});
    for (int s = 0; s < 4; s++) box({4.75f, 0.35f + s * 0.45f, 2.3f}, {0.56f, 0.03f, 2.44f}, {150, 156, 164, 255});
    box({4.75f, 1.84f, 2.3f}, {0.6f, 0.12f, 2.5f}, {206, 60, 46, 255});   // the heat lamp hood
    int slot = 0;
    for (auto& o : S.orders) {
        if (o.done || o.out) continue;
        int n = 0;
        for (int id : o.tickets) { Ticket* k = S.ticket(id); if (k && k->state == T_BOXED) n++; }
        if (!n) continue;
        int shelf = 2 - (slot / 5) % 3, col = slot % 5;
        for (int i = 0; i < n; i++) g3::pizzaBox({4.75f, 0.365f + shelf * 0.45f + i * 0.046f, 1.4f + col * 0.46f}, (float)((o.num * 7) % 9 - 4));
        slot++;
    }
}
}  // namespace

void worldDraw() {
    Store& S = G.S;
    g3::begin(G.cam);
    room();
    makeline();
    ovenAndCut();
    rack();
    for (auto& v : G.people) if (Customer* c = S.cust(v.id)) {
        float anger = c->state == C_DONE ? (c->leftStars >= 4 ? 0.0f : c->leftStars == 3 ? 0.4f : 1.0f) : 1 - c->patience;
        g3::person(c->who, {v.x, 0, v.z}, v.yaw, anger, v.walk);
    }
    if (getenv("MAKELINE_LINEUP")) {   // (screenshots) a row of different people, calm to furious, to check faces
        Rng r(atoi(getenv("MAKELINE_LINEUP")));
        for (int i = 0; i < 7; i++) { Person p = makePerson(r); g3::person(p, {-1.65f + i * 0.55f, 0, -0.75f}, 0, i == 6 ? -1.f : i / 6.f, 0); }
    }
    // the crew, in uniform, where they work
    int nc = 0, nk = 0, nd = 0;
    for (auto& w : S.staff) {
        if (w.role == CSR) g3::person(w.who, {-2.6f - 1.2f * (nc++ % 2), 0, 0.72f}, 180, -1, 0);
        else if (w.role == COOK) g3::person(w.who, {-3.78f, 0, 1.7f + 0.9f * nk++}, 270, -1, w.ticket >= 0 ? ui::time : 0);
        else if (!w.out) g3::person(w.who, {4.05f, 0, 1.5f + 0.8f * nd++}, 90, -1, 0);
    }
    g3::end();
}

namespace {
void bar(float x, float y, float w, float p) {
    DrawRectangleRounded({x - w / 2 - 2, y - 2, w + 4, 12}, 1, 4, INK);
    DrawRectangleRounded({x - w / 2, y, std::max(4.f, w * std::clamp(p, 0.f, 1.f)), 8}, 1, 4, p > 0.5f ? ui::GREEN : p > 0.25f ? ui::YELLOW : ui::RED);
}
void bubble(const std::string& s, float x, float y, float size) {
    float w = std::min(280.f, ui::textW(s, size) + 4), h = ui::wrap(s, 0, 0, w, size, INK, false);
    Rectangle r{x - w / 2 - 12, y - h - 24, w + 24, h + 16};
    ui::panel(r, ui::PAPER, 12, 3, true);
    DrawTriangle({x - 8, r.y + r.height}, {x, r.y + r.height + 14}, {x + 10, r.y + r.height}, INK);
    ui::wrap(s, r.x + 12, r.y + 8, w, size, INK);
}
}  // namespace

void worldHud() {
    Store& S = G.S;
    // over people's heads: how patient they still are, what they're muttering, whose order is ready
    if (G.mode == M_WALK || G.mode == M_REGISTER) {
        for (auto& v : G.people) {
            Customer* c = S.cust(v.id);
            Vector2 p;
            if (!c || c->state == C_DONE || !project({v.x, 1.95f, v.z}, p)) continue;
            if (G.mode == M_REGISTER && p.x > 930) continue;   // behind the order screen
            Order* o = S.order(c->order);
            if (G.hud) bar(p.x, p.y, 70, c->patience);
            float y = p.y - 8;
            if (o && G.hud) {
                char b[32];
                bool ready = o->ready && !o->done;
                float mins = o->minutes(S.t);
                if (ready) snprintf(b, sizeof b, "READY #%d", o->num); else snprintf(b, sizeof b, "#%d  %d:%02d", o->num, (int)mins, (int)(mins * 60) % 60);
                float w = ui::textW(b, 19) + 20, bounce = ready ? std::fabs(std::sin(ui::time * 5)) * 8 : 0;
                ui::panel({p.x - w / 2, y - 30 - bounce, w, 26}, ready ? ui::GREEN : ui::PAPER, 8, 2, false);
                ui::text(b, p.x, y - 27 - bounce, 19, ready ? ui::PAPER : INK, 1);
                y -= 36;
            }
            if (c->barkT > 0 && !(o && o->ready)) bubble(c->bark, p.x, y, 17);
        }
        Vector2 p;
        int ring = S.ringing();
        if (G.hud && ring >= 0 && project({PHONE.x, 1.4f, PHONE.z}, p) && !(G.mode == M_REGISTER && p.x > 930)) {
            float pulse = 0.5f + 0.5f * std::sin(ui::time * 14);
            ui::text("RING!", p.x - 40 - pulse * 8, p.y - 40, 30, ui::RED, 1);
            ui::text("RING!", p.x + 50 + pulse * 8, p.y - 60, 24, ui::RED, 1);
            bar(p.x, p.y, 90, S.cust(ring)->patience);
        }
    }
    if (G.mode != M_WALK) return;
    // the crosshair and what E does
    DrawCircleV({800, 450}, 5, INK);
    DrawCircleV({800, 450}, 3, G.prompt.empty() ? ui::PAPER : ui::YELLOW);
    if (!G.prompt.empty() && !G.hud) DrawRing({800, 450}, 11, 14, 0, 360, 24, ui::YELLOW);   // you can use what you're looking at
    if (!G.prompt.empty() && G.hud) {
        float w = ui::textW(G.prompt, 26) + 110;
        ui::panel({800 - w / 2, 520, w, 50}, ui::CREAM, 12, 3, true);
        ui::panel({800 - w / 2 + 10, 528, 34, 34}, ui::YELLOW, 8, 2, false);
        ui::text("E", 800 - w / 2 + 27, 532, 26, INK, 1);
        ui::text(G.prompt, 800 - w / 2 + 58, 532, 26, INK);
    }
    if (!G.hud) {   // maximum immersion: no bars, no chips, no prompts
        if (S.t < 30 && S.day == 1) ui::text("WASD walk    Mouse look    E use    M manager's office    Esc pause", 800, 860, 22, {255, 250, 238, 220}, 1);
        return;
    }
    // what needs you, wherever you're looking
    float y = 16;
    auto chip = [&](const std::string& s, Color c, bool flash) {
        if (flash && std::fmod(ui::time, 0.6f) < 0.3f) c = ui::shade(c, 1.25f);
        float w = ui::textW(s, 22) + 28;
        ui::panel({16, y, w, 38}, c, 10, 3, true);
        float lum = 0.3f * c.r + 0.6f * c.g + 0.1f * c.b;
        ui::text(s, 30, y + 7, 22, lum < 150 ? ui::PAPER : INK);
        y += 50;
    };
    if (S.ringing() >= 0) chip("The phone is ringing!", ui::RED, true);
    int line = (int)S.line().size(), ready = 0;
    for (int id : S.lobby()) { Customer* c = S.cust(id); Order* o = c ? S.order(c->order) : nullptr; if (o && o->ready) ready++; }
    if (line) chip(std::to_string(line) + (line == 1 ? " customer in line" : " customers in line"), ui::YELLOW, false);
    if (ready) chip(std::to_string(ready) + (ready == 1 ? " order to hand over" : " orders to hand over"), ui::GREEN, true);
    int open = (int)S.openTickets().size();
    if (open) chip(std::to_string(open) + (open == 1 ? " ticket to make" : " tickets to make"), ui::CREAM, false);
    if (!S.exitQ.empty()) chip(std::to_string(S.exitQ.size()) + " out of the oven", {250, 170, 90, 255}, false);
    if (S.t < 20 && S.day == 1) ui::text("WASD walk    Mouse look    E use    Shift hurry    M manager's office    Esc pause", 800, 850, 22, ui::PAPER, 1);
    else ui::text("M: office", 1584, 866, 18, ui::PAPER, 2);
}
