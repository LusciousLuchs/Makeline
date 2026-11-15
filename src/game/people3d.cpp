// Makeline - people. Each person is built from smooth lofted shapes with human proportions, and a face of their own:
// head and jaw shape, eyes and eye colour, brows, nose, lips, ears, hair, beards, glasses, build, height and clothes
// all come from who they are (age, sex, skin, hair) and their `look` number, so the same person always looks the same.
#include "g3d.hpp"
#include "game.hpp"
#include "rlgl.h"
#include <algorithm>
#include <cmath>
#include <map>

using namespace pz;

namespace g3 {
namespace {
const float D2R = 0.0174533f;
struct Ring { float y, rx, rz, zoff; Color c; float a0, a1; };   // an ellipse at a height; a0..a1 = the arc that's there

void vcol(Color c) { rlColor4ub(c.r, c.g, c.b, c.a); }

// a smooth skin over stacked rings
void loft(const Ring* r, int n, int segs, bool capTop = true, bool capBottom = false, float xoff = 0) {
    rlBegin(RL_TRIANGLES);
    auto pt = [&](int i, float u, bool emit) {
        const Ring& g = r[i];
        float a = (g.a0 + (g.a1 - g.a0) * u) * D2R, cs = std::cos(a), sn = std::sin(a);
        int p = std::max(0, i - 1), q = std::min(n - 1, i + 1);
        float dy = r[q].y - r[p].y, dr = (r[q].rx + r[q].rz) - (r[p].rx + r[p].rz);
        float ny = dy > 1e-5f ? -0.5f * dr / dy : 0;
        Vector3 nn{cs * g.rz, ny * (g.rx + g.rz) * 0.5f, sn * g.rx};
        if (emit) { vcol(g.c); rlNormal3f(nn.x, nn.y, nn.z); rlVertex3f(xoff + cs * g.rx, g.y, g.zoff + sn * g.rz); }
    };
    for (int i = 0; i + 1 < n; i++) for (int s = 0; s < segs; s++) {
        float u0 = (float)s / segs, u1 = (float)(s + 1) / segs;
        pt(i, u0, true); pt(i + 1, u0, true); pt(i + 1, u1, true);
        pt(i, u0, true); pt(i + 1, u1, true); pt(i, u1, true);
    }
    for (int cap = 0; cap < 2; cap++) {
        if (!(cap ? capBottom : capTop)) continue;
        const Ring& g = r[cap ? 0 : n - 1];
        vcol(g.c); rlNormal3f(0, cap ? -1.f : 1.f, 0);
        for (int s = 0; s < segs; s++) {
            float a0 = (g.a0 + (g.a1 - g.a0) * s / segs) * D2R, a1 = (g.a0 + (g.a1 - g.a0) * (s + 1) / segs) * D2R;
            rlVertex3f(xoff, g.y, g.zoff); rlVertex3f(xoff + std::cos(a1) * g.rx, g.y, g.zoff + std::sin(a1) * g.rz); rlVertex3f(xoff + std::cos(a0) * g.rx, g.y, g.zoff + std::sin(a0) * g.rz);
        }
    }
    rlEnd();
}

// a rounded limb from a to b
void limb(Vector3 a, Vector3 b, float ra, float rb, Color ca, Color cb, int segs = 9) {
    Vector3 d{b.x - a.x, b.y - a.y, b.z - a.z};
    float len = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
    if (len < 1e-4f) return;
    d = {d.x / len, d.y / len, d.z / len};
    Vector3 up = std::fabs(d.y) > 0.9f ? Vector3{1, 0, 0} : Vector3{0, 1, 0};
    Vector3 u{d.y * up.z - d.z * up.y, d.z * up.x - d.x * up.z, d.x * up.y - d.y * up.x};
    float ul = std::sqrt(u.x * u.x + u.y * u.y + u.z * u.z);
    u = {u.x / ul, u.y / ul, u.z / ul};
    Vector3 v{d.y * u.z - d.z * u.y, d.z * u.x - d.x * u.z, d.x * u.y - d.y * u.x};
    struct S { float t, r, k; Color c; } st[4] = {{-ra * 0.7f, ra * 0.6f, -0.7f, ca}, {0, ra, 0, ca}, {len, rb, 0, cb}, {len + rb * 0.7f, rb * 0.6f, 0.7f, cb}};
    rlBegin(RL_TRIANGLES);
    auto pt = [&](int i, int s) {
        float a2 = 6.28319f * s / segs, cs = std::cos(a2), sn = std::sin(a2);
        const S& g = st[i];
        Vector3 n{u.x * cs + v.x * sn + d.x * g.k, u.y * cs + v.y * sn + d.y * g.k, u.z * cs + v.z * sn + d.z * g.k};
        vcol(g.c); rlNormal3f(n.x, n.y, n.z);
        rlVertex3f(a.x + d.x * g.t + (u.x * cs + v.x * sn) * g.r, a.y + d.y * g.t + (u.y * cs + v.y * sn) * g.r, a.z + d.z * g.t + (u.z * cs + v.z * sn) * g.r);
    };
    for (int i = 0; i < 3; i++) for (int s = 0; s < segs; s++) {
        pt(i, s); pt(i + 1, s); pt(i + 1, s + 1);
        pt(i, s); pt(i + 1, s + 1); pt(i, s + 1);
    }
    for (int e = 0; e < 2; e++) {   // close the ends
        const S& g = st[e ? 3 : 0];
        vcol(g.c); rlNormal3f(d.x * (e ? 1.f : -1.f), d.y * (e ? 1.f : -1.f), d.z * (e ? 1.f : -1.f));
        Vector3 c{a.x + d.x * g.t, a.y + d.y * g.t, a.z + d.z * g.t};
        for (int s = 0; s < segs; s++) {
            float a0 = 6.28319f * s / segs, a1 = 6.28319f * (s + 1) / segs;
            rlVertex3f(c.x, c.y, c.z);
            rlVertex3f(c.x + (u.x * std::cos(a0) + v.x * std::sin(a0)) * g.r, c.y + (u.y * std::cos(a0) + v.y * std::sin(a0)) * g.r, c.z + (u.z * std::cos(a0) + v.z * std::sin(a0)) * g.r);
            rlVertex3f(c.x + (u.x * std::cos(a1) + v.x * std::sin(a1)) * g.r, c.y + (u.y * std::cos(a1) + v.y * std::sin(a1)) * g.r, c.z + (u.z * std::cos(a1) + v.z * std::sin(a1)) * g.r);
        }
    }
    rlEnd();
}

// an ellipsoid, smooth
void blob(Vector3 c, float rx, float ry, float rz, Color col, int rings = 6, int segs = 10) {
    rlBegin(RL_TRIANGLES);
    vcol(col);
    auto v = [&](int i, int j) {
        float th = 3.14159f * i / rings, ph = 6.28319f * j / segs;
        float x = std::sin(th) * std::cos(ph), y = std::cos(th), z = std::sin(th) * std::sin(ph);
        rlNormal3f(x / rx, y / ry, z / rz);
        rlVertex3f(c.x + x * rx, c.y + y * ry, c.z + z * rz);
    };
    for (int i = 0; i < rings; i++) for (int j = 0; j < segs; j++) {
        v(i, j); v(i, j + 1); v(i + 1, j + 1);
        v(i, j); v(i + 1, j + 1); v(i + 1, j);
    }
    rlEnd();
}

// everything about how one person looks, worked out from who they are
struct Looks {
    Color skin, hair, shirt, shirt2, pants, shoes, iris, lips;
    float tall, build, shoulders, hips, belly;
    float hw, hd, jaw, chin, noseL, noseW, eyeGap, eyeR, brow, lipW, lipH, ear;
    int hairStyle, hairVar, beard, top;      // top: 0 tee, 1 long sleeve, 2 hoodie, 3 jacket
    bool glasses, skirt, uniform, receding;
};
Looks looksOf(const Person& p, bool uniform) {
    static const Color SKIN[] = {{250, 222, 196, 255}, {240, 200, 166, 255}, {220, 172, 130, 255}, {186, 132, 94, 255}, {142, 98, 70, 255}, {98, 66, 48, 255}};
    static const Color HAIR[] = {{92, 62, 42, 255}, {34, 28, 28, 255}, {210, 172, 100, 255}, {164, 78, 44, 255}, {148, 148, 150, 255}, {230, 230, 226, 255}};
    static const Color TOPS[] = {{70, 110, 170, 255}, {84, 140, 90, 255}, {200, 120, 60, 255}, {130, 90, 150, 255}, {176, 70, 80, 255}, {84, 86, 96, 255}, {220, 196, 110, 255}, {60, 130, 136, 255},
        {236, 232, 224, 255}, {40, 44, 60, 255}, {150, 60, 50, 255}, {110, 130, 90, 255}};
    static const Color PANTS[] = {{58, 70, 104, 255}, {72, 66, 60, 255}, {40, 44, 52, 255}, {124, 108, 84, 255}, {90, 100, 120, 255}};
    static const Color EYES[] = {{92, 60, 36, 255}, {70, 44, 28, 255}, {80, 120, 160, 255}, {86, 130, 96, 255}, {130, 100, 60, 255}};
    Rng r(p.look ^ 0xA5A5u);
    Looks L{};
    L.uniform = uniform;
    L.skin = SKIN[std::clamp(p.skin, 0, 5)];
    L.hair = HAIR[std::clamp(p.hair, 0, 5)];
    L.shirt = TOPS[r.below(12)]; L.shirt2 = TOPS[r.below(12)]; L.pants = PANTS[r.below(5)];
    L.shoes = r.chance(0.5f) ? Color{44, 38, 36, 255} : r.chance(0.5f) ? Color{226, 226, 222, 255} : Color{110, 76, 50, 255};
    L.iris = EYES[p.skin >= 3 ? r.below(2) : r.below(5)];
    L.lips = ui::mix(L.skin, p.sex ? Color{176, 60, 70, 255} : Color{150, 84, 80, 255}, p.sex ? 0.6f : 0.4f);
    L.tall = (p.sex ? 0.93f : 1.0f) + r.range(-0.045f, 0.05f) - (p.age > 66 ? 0.03f : 0) - (p.age < 19 ? 0.02f : 0);
    L.build = r.range(0.86f, 1.28f) + (p.age > 40 ? 0.06f : 0);
    L.shoulders = p.sex ? 0.9f : 1.04f;
    L.hips = p.sex ? 1.1f : 0.96f;
    L.belly = std::max(0.f, L.build - 1.05f) * 1.6f + (p.age > 50 ? 0.12f : 0);
    L.hw = 0.074f * r.range(0.94f, 1.07f) * (p.sex ? 0.96f : 1.0f);
    L.hd = 0.092f * r.range(0.96f, 1.05f);
    L.jaw = (p.sex ? 0.84f : 0.94f) * r.range(0.92f, 1.08f) + std::max(0.f, L.build - 1.1f) * 0.3f;
    L.chin = r.range(0.85f, 1.2f);
    L.noseL = r.range(0.020f, 0.032f); L.noseW = r.range(0.010f, 0.016f);
    L.eyeGap = r.range(0.030f, 0.035f); L.eyeR = r.range(0.0125f, 0.015f);
    L.brow = r.range(0.6f, 1.4f) * (p.sex ? 0.7f : 1.0f);
    L.lipW = r.range(0.021f, 0.028f); L.lipH = r.range(0.7f, 1.3f) * (p.sex ? 1.2f : 0.9f);
    L.ear = r.range(0.85f, 1.25f);
    L.hairStyle = p.hairStyle; L.hairVar = r.below(3);
    L.receding = !p.sex && p.age > 42 && r.chance(0.5f);
    float f = r.f();
    L.beard = (!p.sex && p.age > 20) ? (f < 0.48f ? 0 : f < 0.62f ? 1 : f < 0.76f ? 2 : 3) : 0;   // none, moustache, goatee, full
    L.glasses = r.chance(p.age > 45 ? 0.45f : 0.2f);
    L.top = r.below(4);
    L.skirt = p.sex && r.chance(0.15f);
    if (uniform) { L.shirt = {198, 52, 42, 255}; L.shirt2 = {160, 36, 32, 255}; L.pants = {38, 38, 44, 255}; L.shoes = {30, 28, 28, 255}; L.top = 0; L.skirt = false; }
    return L;
}

// the head's outline, chin to crown: height (0..1), width, depth, forward shift
const float HEAD[9][4] = {{0.00f, 0.30f, 0.34f, 0.016f}, {0.07f, 0.60f, 0.62f, 0.012f}, {0.20f, 0.84f, 0.86f, 0.006f}, {0.38f, 0.96f, 0.97f, 0.002f},
    {0.55f, 1.00f, 1.00f, 0}, {0.72f, 0.99f, 1.01f, -0.003f}, {0.86f, 0.88f, 0.92f, -0.006f}, {0.95f, 0.62f, 0.66f, -0.008f}, {1.00f, 0.22f, 0.24f, -0.008f}};
const float HEAD_H = 0.235f;

void head(const Looks& L, const Person& who, float y0, float anger) {
    Color skin = ui::mix(L.skin, {214, 70, 56, 255}, anger * 0.4f), shade = ui::shade(skin, 0.88f);
    Ring rg[9];
    for (int i = 0; i < 9; i++) {
        float jawK = HEAD[i][0] < 0.3f ? L.jaw + (1 - L.jaw) * (HEAD[i][0] / 0.3f) : 1;
        float chinK = i == 0 ? L.chin : 1;
        rg[i] = {y0 + HEAD[i][0] * HEAD_H, L.hw * HEAD[i][1] * jawK * (i == 0 ? chinK : 1), L.hd * HEAD[i][2], HEAD[i][3] * chinK, skin, 0, 360};
    }
    loft(rg, 9, 16, true, true);
    // where the face's surface is, at a height and a distance from the middle
    auto front = [&](float t, float x) {
        int i = 0;
        while (i < 7 && HEAD[i + 1][0] < t) i++;
        float u = std::clamp((t - HEAD[i][0]) / (HEAD[i + 1][0] - HEAD[i][0]), 0.f, 1.f);
        float rx = rg[i].rx + (rg[i + 1].rx - rg[i].rx) * u, rz = rg[i].rz + (rg[i + 1].rz - rg[i].rz) * u, zo = rg[i].zoff + (rg[i + 1].zoff - rg[i].zoff) * u;
        float k = std::clamp(x / rx, -0.98f, 0.98f);
        return zo + rz * std::sqrt(1 - k * k);
    };
    auto Y = [&](float t) { return y0 + t * HEAD_H; };
    Color ink{36, 26, 24, 255};
    // ears
    for (int s = -1; s <= 1; s += 2) blob({s * L.hw * 0.97f, Y(0.5f), -0.008f}, 0.008f, 0.021f * L.ear, 0.013f * L.ear, shade);
    // eyes: whites, iris, pupil, lids that come down when they're angry or old
    float lid = 0.08f + anger * 0.32f + (who.age > 60 ? 0.12f : 0);
    for (int s = -1; s <= 1; s += 2) {
        float x = s * L.eyeGap, z = front(0.56f, x) - L.eyeR * 0.55f;
        blob({x, Y(0.56f), z}, L.eyeR * 1.25f, L.eyeR, L.eyeR, {244, 242, 236, 255});
        blob({x, Y(0.56f) - 0.0005f, z + L.eyeR * 0.62f}, L.eyeR * 0.56f, L.eyeR * 0.56f, L.eyeR * 0.45f, L.iris, 4, 8);
        blob({x, Y(0.56f) - 0.0005f, z + L.eyeR * 0.9f}, L.eyeR * 0.26f, L.eyeR * 0.26f, L.eyeR * 0.2f, {16, 14, 14, 255}, 3, 6);
        blob({x, Y(0.56f) + L.eyeR * (1.0f - lid), z + L.eyeR * 0.2f}, L.eyeR * 1.4f, L.eyeR * 0.8f, L.eyeR * 1.05f, skin, 4, 8);   // upper lid
        blob({x, Y(0.56f) - L.eyeR * 1.35f, z + L.eyeR * 0.05f}, L.eyeR * 1.3f, L.eyeR * 0.55f, L.eyeR * 0.85f, skin, 4, 8);   // lower lid
        // brow
        rlPushMatrix();
        rlTranslatef(x, Y(0.665f) - anger * 0.006f, front(0.66f, x) + 0.001f);
        rlRotatef(-s * (6 + anger * 22), 0, 0, 1);
        rlRotatef(s * 14, 0, 1, 0);
        blob({0, 0, 0}, 0.017f, 0.0032f * L.brow + 0.0012f, 0.004f, who.hair >= 4 ? ui::shade(L.hair, 0.8f) : ui::shade(L.hair, 0.7f), 4, 8);
        rlPopMatrix();
    }
    // nose: bridge to tip, nostrils
    float nz = front(0.40f, 0);
    limb({0, Y(0.60f), front(0.60f, 0) - 0.004f}, {0, Y(0.41f), nz + L.noseL * 0.55f}, 0.006f, L.noseW * 0.72f, skin, skin, 7);
    blob({0, Y(0.395f), nz + L.noseL * 0.62f}, L.noseW * 0.8f, L.noseW * 0.66f, L.noseW * 0.8f, skin, 5, 8);
    for (int s = -1; s <= 1; s += 2) blob({s * L.noseW * 0.85f, Y(0.385f), nz + L.noseL * 0.3f}, L.noseW * 0.55f, L.noseW * 0.45f, L.noseW * 0.6f, shade, 4, 7);
    // mouth: lips, turning down as they lose patience, open when they're shouting
    float my = Y(0.215f), mz = front(0.215f, 0);
    if (anger > 0.8f) blob({0, my - 0.003f, mz - 0.002f}, L.lipW * 0.7f, 0.011f, 0.006f, {60, 20, 22, 255}, 4, 8);
    for (int s = -1; s <= 1; s += 2) {
        rlPushMatrix();
        rlTranslatef(s * L.lipW * 0.5f, my, front(0.215f, s * L.lipW * 0.5f) - 0.0015f);
        rlRotatef(s * (5 - anger * 18), 0, 0, 1);
        rlRotatef(s * 16, 0, 1, 0);
        blob({0, 0.0018f * L.lipH, 0}, L.lipW * 0.56f, 0.0022f * L.lipH, 0.0036f, ui::shade(L.lips, 0.9f), 4, 7);
        blob({0, -0.0022f * L.lipH - (anger > 0.8f ? 0.008f : 0), 0}, L.lipW * 0.5f, 0.0027f * L.lipH, 0.004f, L.lips, 4, 7);
        rlPopMatrix();
    }
    // cheeks fill out with a heavy build
    if (L.build > 1.12f) for (int s = -1; s <= 1; s += 2) blob({s * L.hw * 0.62f, Y(0.3f), front(0.3f, s * L.hw * 0.62f) - 0.012f}, 0.02f, 0.022f, 0.016f, skin, 4, 8);

    // ---- hair
    Color hair = L.hair, hairD = ui::shade(hair, 0.82f);
    int style = L.hairStyle;
    bool cap = style == 4 || L.uniform;
    if (cap && !L.uniform) style = 1;
    if (L.uniform && style == 4) style = 1;
    float line = L.receding ? 0.90f : 0.80f;                // the hairline's height on the forehead
    float thick = style == 0 ? 0 : style == 1 ? (L.hairVar == 0 ? 0.004f : 0.011f) : 0.014f;
    if (who.skin >= 4 && style == 1 && L.hairVar == 2) thick = 0.028f;   // a rounder, fuller cut
    if (style == 0 && who.age > 38) {   // bald on top, a band round the back and sides
        Ring b[3];
        for (int i = 0; i < 3; i++) { int k = 3 + i; b[i] = {y0 + HEAD[k][0] * HEAD_H, rg[k].rx + 0.004f, rg[k].rz + 0.004f, rg[k].zoff, hairD, 175, 365}; }
        loft(b, 3, 12, false);
    }
    if (thick > 0) {
        Ring h[6]; int n = 0;
        for (int k = 3; k < 9; k++) {
            float t = HEAD[k][0], cut = t < 0.6f ? 80 : t < line ? 80 - (t - 0.6f) / (line - 0.6f) * 74 : 0;
            float grow = thick * (t > 0.9f ? 1.15f : 1);
            h[n++] = {y0 + t * HEAD_H + (k == 8 ? thick : 0), rg[k].rx + grow, rg[k].rz + grow, rg[k].zoff - (cut > 0 ? 0.004f : 0), k % 2 ? hair : hairD, 90 + cut, 450 - cut};
        }
        loft(h, n, 16, true);
        if (L.hairVar == 1 && style != 0 && !cap) blob({-0.012f, Y(0.86f), front(0.84f, 0) - 0.006f}, L.hw * 0.8f, 0.014f, 0.018f, hair, 4, 8);   // a fringe
    }
    if (style == 3) {   // long: down the back to the shoulders
        Ring d[4] = {{Y(0.6f), L.hw + 0.016f, L.hd + 0.016f, -0.006f, hair, 172, 368}, {Y(0.2f), L.hw + 0.022f, L.hd + 0.012f, -0.012f, hairD, 178, 362},
            {Y(-0.2f), L.hw + 0.03f, L.hd * 0.8f, -0.03f, hair, 185, 355}, {Y(-0.5f - L.hairVar * 0.12f), L.hw + 0.024f, L.hd * 0.5f, -0.04f, hairD, 195, 345}};
        loft(d, 4, 12, false);
    }
    if (style == 2) {   // up in a bun, or a ponytail
        if (L.hairVar == 0) limb({0, Y(0.72f), -L.hd - 0.01f}, {0, Y(0.1f), -L.hd - 0.05f}, 0.02f, 0.012f, hair, hairD, 7);
        else blob({0, Y(0.9f), -L.hd * 0.75f}, 0.034f, 0.032f, 0.034f, hairD, 5, 9);
    }
    if (cap) {   // a visor for the crew, a ball cap for anyone else
        Color cc = L.uniform ? Color{30, 30, 34, 255} : L.shirt2;
        Ring c[3] = {{Y(0.74f), L.hw + 0.016f, L.hd + 0.016f, -0.002f, cc, 0, 360}, {Y(0.86f), L.hw * 0.9f + 0.016f, L.hd * 0.93f + 0.016f, -0.005f, cc, 0, 360},
            {Y(L.uniform ? 0.9f : 1.04f), L.hw * (L.uniform ? 0.86f : 0.3f) + 0.012f, L.hd * (L.uniform ? 0.9f : 0.3f) + 0.012f, -0.007f, cc, 0, 360}};
        loft(c, 3, 16, !L.uniform);
        rlPushMatrix();
        rlTranslatef(0, Y(0.76f), L.hd + 0.03f);
        rlRotatef(12, 1, 0, 0);
        blob({0, 0, 0}, L.hw * 0.95f, 0.004f, 0.05f, ui::shade(cc, 0.8f), 4, 10);
        rlPopMatrix();
        if (L.uniform) blob({0, Y(0.8f), L.hd + 0.016f}, 0.016f, 0.012f, 0.004f, {198, 52, 42, 255}, 3, 8);
    }
    // ---- facial hair
    if (L.beard == 1 || L.beard == 3) blob({0, Y(0.3f), front(0.3f, 0) + 0.001f}, L.lipW * 1.05f, 0.006f, 0.006f, hairD, 4, 8);
    if (L.beard == 2) blob({0, Y(0.07f), front(0.07f, 0) + 0.002f}, 0.02f, 0.022f, 0.012f, hairD, 4, 8);
    if (L.beard == 3) {
        Ring b[4];
        for (int i = 0; i < 4; i++) b[i] = {rg[i].y - (i == 0 ? 0.012f : 0), rg[i].rx + 0.006f + (i < 2 ? 0.006f : 0), rg[i].rz + 0.006f, rg[i].zoff + 0.002f, i % 2 ? hair : hairD, i == 3 ? 150.f : -20.f, i == 3 ? 180.f : 200.f};
        loft(b, 3, 12, false, true);
        for (int s = -1; s <= 1; s += 2) blob({s * L.hw * 0.86f, Y(0.36f), 0.012f}, 0.008f, 0.03f, 0.022f, hairD, 4, 7);   // sideburns into the beard
    }
    if (L.glasses) {
        Color fr = (who.look >> 5) & 1 ? Color{30, 28, 30, 255} : Color{120, 84, 50, 255};
        for (int s = -1; s <= 1; s += 2) {
            float x = s * L.eyeGap, z = front(0.56f, x) + 0.008f;
            rlPushMatrix();
            rlTranslatef(x, Y(0.555f), z);
            rlRotatef(90, 1, 0, 0);
            g3::ring({0, 0, 0}, 0.0135f, 0.0165f, fr, 0, 360, 14);
            g3::disc({0, 0.0005f, 0}, 0.0135f, {210, 228, 240, 70}, 12);
            rlPopMatrix();
            limb({s * (L.eyeGap + 0.016f), Y(0.56f), z}, {s * L.hw * 1.0f, Y(0.57f), 0.0f}, 0.0014f, 0.0014f, fr, fr, 4);
        }
        limb({-L.eyeGap + 0.014f, Y(0.565f), front(0.56f, 0) + 0.009f}, {L.eyeGap - 0.014f, Y(0.565f), front(0.56f, 0) + 0.009f}, 0.0014f, 0.0014f, fr, fr, 4);
    }
}
}  // namespace

void person(const Person& who, Vector3 feet, float yaw, float anger, float walk) {
    bool uniform = anger < 0;
    if (uniform) anger = 0;
    Looks L = looksOf(who, uniform);
    float t = ui::time, ph = walk * 8.5f, sw = walk > 0 ? std::sin(ph) : 0;
    float bob = walk > 0 ? std::fabs(std::cos(ph)) * 0.018f : std::sin(t * (1.4f + anger * 2.5f) + (who.look & 31)) * 0.004f;
    float shake = anger > 0.7f ? std::sin(t * 22 + who.look) * 0.004f * anger : 0;
    // a soft shadow on the floor
    rlBegin(RL_TRIANGLES);
    rlNormal3f(0, 0, 0);
    for (int i = 0; i < 14; i++) {
        float a0 = 6.28319f * i / 14, a1 = 6.28319f * (i + 1) / 14;
        rlColor4ub(20, 14, 12, 96); rlVertex3f(feet.x, feet.y + 0.004f, feet.z);
        rlColor4ub(20, 14, 12, 0); rlVertex3f(feet.x + std::cos(a1) * 0.38f, feet.y + 0.004f, feet.z + std::sin(a1) * 0.38f);
        rlVertex3f(feet.x + std::cos(a0) * 0.38f, feet.y + 0.004f, feet.z + std::sin(a0) * 0.38f);
    }
    rlEnd();
    rlPushMatrix();
    rlTranslatef(feet.x + shake, feet.y + bob * L.tall, feet.z);
    rlRotatef(yaw, 0, 1, 0);
    rlScalef(L.tall, L.tall, L.tall);
    float B = L.build, sh = L.shoulders, hp = L.hips;
    Color skin = L.skin, shirt = L.shirt, pants = L.pants;
    // ---- legs and shoes
    float hipX = 0.085f * hp, stride = 0.2f * sw;
    for (int s = -1; s <= 1; s += 2) {
        float f = s * stride;
        Vector3 hip{s * hipX, 0.92f, 0}, knee{s * hipX, 0.5f, f * 0.55f + (f > 0 ? 0.03f : 0)}, ank{s * hipX * 0.95f, 0.09f, f};
        Color leg = L.skirt ? skin : pants;
        limb(hip, knee, 0.078f * B * hp, 0.052f * (0.6f + 0.4f * B), leg, leg, 9);
        limb(knee, ank, 0.05f * (0.6f + 0.4f * B), 0.036f, leg, leg, 8);
        blob({ank.x, 0.045f, ank.z + 0.045f}, 0.048f, 0.045f, 0.115f, L.shoes, 5, 9);
    }
    // ---- the body: hips, waist, chest, shoulders, as one piece
    float bz = L.belly * 0.05f, bust = who.sex ? 0.024f : 0.006f;
    Color lower = L.skirt ? L.shirt2 : pants;
    Ring torso[9] = {
        {0.80f, 0.150f * hp * B, 0.105f * B, 0, lower, 0, 360}, {0.92f, 0.165f * hp * B, 0.115f * B, 0.004f, lower, 0, 360},
        {0.99f, 0.158f * B * (0.5f + 0.5f * hp), 0.112f * B + bz, bz * 0.6f, lower, 0, 360}, {0.995f, 0.160f * B * (0.5f + 0.5f * hp), 0.114f * B + bz, bz * 0.6f, shirt, 0, 360},
        {1.10f, 0.146f * B + L.belly * 0.02f, 0.102f * B + bz * 1.1f, bz * 0.8f, shirt, 0, 360}, {1.26f, 0.170f * sh * (0.6f + 0.4f * B), 0.112f * B + bust * 0.5f, bust, shirt, 0, 360},
        {1.37f, 0.185f * sh * (0.7f + 0.3f * B), 0.100f * B, 0.004f, shirt, 0, 360}, {1.43f, 0.120f * sh, 0.075f, 0, shirt, 0, 360}, {1.455f, 0.052f, 0.05f, 0, shirt, 0, 360}};
    loft(torso, 9, 16, true, true);
    if (L.skirt) { Ring sk[2] = {{0.9f, 0.17f * hp * B, 0.125f * B, 0, L.shirt2, 0, 360}, {0.52f, 0.22f * hp * B, 0.17f * B, 0, ui::shade(L.shirt2, 0.9f), 0, 360}}; Ring k2[2] = {sk[1], sk[0]}; loft(k2, 2, 14, false); }
    if (L.top == 3 && !uniform) {   // an open jacket: another colour down the front
        Ring j[3] = {{1.0f, 0.06f, 0.116f * B + bz, bz * 0.6f + 0.003f, L.shirt2, 60, 120}, {1.26f, 0.07f, 0.114f * B + bust * 0.5f, bust + 0.003f, L.shirt2, 60, 120}, {1.41f, 0.05f, 0.08f, 0.004f, L.shirt2, 60, 120}};
        loft(j, 3, 5, false);
    }
    if (L.top == 2 && !uniform) blob({0, 1.42f, -0.085f}, 0.1f, 0.07f, 0.06f, ui::shade(shirt, 0.92f), 5, 9);   // a hood, down
    if (uniform) {   // collar and a name tag
        blob({0, 1.44f, 0.03f}, 0.075f, 0.016f, 0.06f, L.shirt2, 4, 10);
        box({0.085f, 1.27f, 0.116f * B + bust * 0.6f}, {0.06f, 0.022f, 0.004f}, {244, 240, 230, 255});
    }
    limb({0, 1.43f, 0}, {0, 1.5f, 0.004f}, 0.047f, 0.044f, skin, skin, 9);   // neck
    // ---- arms: swinging, hanging, or folded when they've had enough
    float shX = 0.19f * sh * (0.7f + 0.3f * B);
    bool sleeves = L.top >= 1 && !uniform;
    for (int s = -1; s <= 1; s += 2) {
        Vector3 S{s * shX, 1.35f, 0}, E, W;
        if (anger > 0.5f) { E = {s * (shX + 0.035f), 1.12f, 0.03f}; W = {-s * 0.07f, 1.14f + s * 0.02f, 0.115f * B + bz + 0.04f}; }
        else { float a = -s * sw * 0.16f; E = {s * (shX + 0.012f), 1.09f, a * 0.6f}; W = {s * (shX + 0.006f), 0.85f, a + 0.03f}; }
        Color up = shirt, fore = sleeves ? shirt : skin;
        limb(S, E, 0.044f * (0.6f + 0.4f * B), 0.037f * (0.6f + 0.4f * B), up, up, 8);
        limb(E, W, 0.038f * (0.6f + 0.4f * B), 0.028f, fore, fore, 8);
        Vector3 d{W.x - E.x, W.y - E.y, W.z - E.z};
        float dl = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
        blob({W.x + d.x / dl * 0.045f, W.y + d.y / dl * 0.045f, W.z + d.z / dl * 0.045f}, 0.03f, 0.05f, 0.022f, skin, 4, 8);   // hand
    }
    head(L, who, 1.5f, anger);
    rlPopMatrix();
}

// ---- portraits: a person's head and shoulders, drawn once into a small picture for the screens
namespace {
struct Shot { RenderTexture2D rt; };
std::map<uint64_t, Shot> shots;
uint64_t keyOf(const Person& p, bool uniform) { return ((uint64_t)p.look << 20) ^ ((uint64_t)p.age << 8) ^ (p.skin << 4) ^ (p.hair << 1) ^ (uniform ? 1 : 0) ^ ((uint64_t)p.hairStyle << 40) ^ ((uint64_t)p.sex << 44); }
}  // namespace
void portraitPrepare(const Person& p, bool uniform) {
    uint64_t k = keyOf(p, uniform);
    if (shots.count(k)) return;
    if (shots.size() > 60) { for (auto& s : shots) UnloadRenderTexture(s.second.rt); shots.clear(); }
    Shot sh{LoadRenderTexture(192, 192)};
    SetTextureFilter(sh.rt.texture, TEXTURE_FILTER_BILINEAR);
    Looks L = looksOf(p, uniform);
    Camera3D c{};
    c.position = {0.1f, 1.63f * L.tall, 0.95f}; c.target = {0, 1.6f * L.tall, 0}; c.up = {0, 1, 0}; c.fovy = 20; c.projection = CAMERA_PERSPECTIVE;
    BeginTextureMode(sh.rt);
    ClearBackground({214, 204, 186, 255});
    begin(c);
    person(p, {0, 0, 0}, 0, uniform ? -1.f : 0.f, 0);
    end();
    EndTextureMode();
    shots[k] = sh;
}
bool portrait(const Person& p, bool uniform, Texture2D& out) {
    auto it = shots.find(keyOf(p, uniform));
    if (it == shots.end()) return false;
    out = it->second.rt.texture;
    return true;
}

}  // namespace g3
