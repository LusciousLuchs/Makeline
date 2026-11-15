// Makeline - the 3D drawing kit.
#include "g3d.hpp"
#include "game.hpp"
#include "rlgl.h"
#include <algorithm>
#include <cmath>

using namespace pz;

namespace g3 {
namespace {
Shader lit{};
const char* VS = R"(#version 330
in vec3 vertexPosition; in vec2 vertexTexCoord; in vec3 vertexNormal; in vec4 vertexColor;
uniform mat4 mvp;
out vec2 uv; out vec4 col; out vec3 nrm; out vec3 pos;
void main() { uv = vertexTexCoord; col = vertexColor; nrm = vertexNormal; pos = vertexPosition; gl_Position = mvp * vec4(vertexPosition, 1.0); })";
const char* FS = R"(#version 330
in vec2 uv; in vec4 col; in vec3 nrm; in vec3 pos;
uniform sampler2D texture0; uniform vec4 colDiffuse; uniform vec3 eye;
out vec4 finalColor;
void main() {
    vec4 base = texture(texture0, uv) * colDiffuse * col;
    vec3 c = base.rgb;
    if (dot(nrm, nrm) > 0.1) {
        vec3 n = normalize(nrm);
        vec3 v = normalize(eye - pos);
        if (dot(n, v) < 0.0) n = -n;                                  // thin shells are lit from whichever side you see
        float key = max(dot(n, normalize(vec3(0.30, 0.80, 0.52))), 0.0);
        float fill = max(dot(n, normalize(vec3(-0.6, 0.25, -0.5))), 0.0);
        float sky = 0.5 + 0.5 * n.y;
        vec3 light = vec3(0.50, 0.50, 0.54) * (0.78 + 0.22 * sky) + vec3(1.0, 0.95, 0.86) * key * 0.50 + vec3(0.60, 0.66, 0.80) * fill * 0.14;
        float rim = pow(1.0 - max(dot(n, v), 0.0), 3.0);
        c = base.rgb * light + vec3(1.0, 0.95, 0.88) * rim * 0.05 * (0.3 + base.rgb);
        c = min(c, vec3(1.0));
    }
    finalColor = vec4(c, base.a);
})";
int eyeLoc = -1;
const float D2R = 0.0174533f;
void col(Color c) { rlColor4ub(c.r, c.g, c.b, c.a); }
}  // namespace

void init() { lit = LoadShaderFromMemory(VS, FS); eyeLoc = GetShaderLocation(lit, "eye"); }
void begin(const Camera3D& cam) {
    BeginMode3D(cam);
    float e[3] = {cam.position.x, cam.position.y, cam.position.z};
    SetShaderValue(lit, eyeLoc, e, SHADER_UNIFORM_VEC3);
    BeginShaderMode(lit);
    rlDisableBackfaceCulling();
}
void end() {
    EndShaderMode();
    rlEnableBackfaceCulling();
    EndMode3D();
}

void box(Vector3 c, Vector3 s, Color color, float yaw) {
    rlPushMatrix();
    rlTranslatef(c.x, c.y, c.z);
    if (yaw != 0) rlRotatef(yaw, 0, 1, 0);
    float x = s.x / 2, y = s.y / 2, z = s.z / 2;
    rlBegin(RL_TRIANGLES);
    col(color);
    auto face = [&](Vector3 n, Vector3 a, Vector3 b, Vector3 cc, Vector3 d) {
        rlNormal3f(n.x, n.y, n.z);
        rlVertex3f(a.x, a.y, a.z); rlVertex3f(b.x, b.y, b.z); rlVertex3f(cc.x, cc.y, cc.z);
        rlVertex3f(a.x, a.y, a.z); rlVertex3f(cc.x, cc.y, cc.z); rlVertex3f(d.x, d.y, d.z);
    };
    face({0, 1, 0}, {-x, y, -z}, {-x, y, z}, {x, y, z}, {x, y, -z});
    face({0, -1, 0}, {-x, -y, -z}, {x, -y, -z}, {x, -y, z}, {-x, -y, z});
    face({0, 0, 1}, {-x, -y, z}, {x, -y, z}, {x, y, z}, {-x, y, z});
    face({0, 0, -1}, {-x, -y, -z}, {-x, y, -z}, {x, y, -z}, {x, -y, -z});
    face({1, 0, 0}, {x, -y, -z}, {x, y, -z}, {x, y, z}, {x, -y, z});
    face({-1, 0, 0}, {-x, -y, -z}, {-x, -y, z}, {-x, y, z}, {-x, y, -z});
    rlEnd();
    rlPopMatrix();
}
void cyl(Vector3 b, float r, float h, Color color, int segs, float rTop) {
    if (rTop < 0) rTop = r;
    rlBegin(RL_TRIANGLES);
    col(color);
    for (int i = 0; i < segs; i++) {
        float a0 = 6.28319f * i / segs, a1 = 6.28319f * (i + 1) / segs;
        float c0 = std::cos(a0), s0 = std::sin(a0), c1 = std::cos(a1), s1 = std::sin(a1);
        rlNormal3f(0, 1, 0);
        rlVertex3f(b.x, b.y + h, b.z); rlVertex3f(b.x + c1 * rTop, b.y + h, b.z + s1 * rTop); rlVertex3f(b.x + c0 * rTop, b.y + h, b.z + s0 * rTop);
        float nx = std::cos((a0 + a1) / 2), nz = std::sin((a0 + a1) / 2);
        rlNormal3f(nx, 0, nz);
        rlVertex3f(b.x + c0 * r, b.y, b.z + s0 * r); rlVertex3f(b.x + c0 * rTop, b.y + h, b.z + s0 * rTop); rlVertex3f(b.x + c1 * rTop, b.y + h, b.z + s1 * rTop);
        rlVertex3f(b.x + c0 * r, b.y, b.z + s0 * r); rlVertex3f(b.x + c1 * rTop, b.y + h, b.z + s1 * rTop); rlVertex3f(b.x + c1 * r, b.y, b.z + s1 * r);
    }
    rlEnd();
}
void disc(Vector3 c, float r, Color color, int segs) {
    rlBegin(RL_TRIANGLES);
    col(color);
    rlNormal3f(0, 1, 0);
    for (int i = 0; i < segs; i++) {
        float a0 = 6.28319f * i / segs, a1 = 6.28319f * (i + 1) / segs;
        rlVertex3f(c.x, c.y, c.z); rlVertex3f(c.x + std::cos(a1) * r, c.y, c.z + std::sin(a1) * r); rlVertex3f(c.x + std::cos(a0) * r, c.y, c.z + std::sin(a0) * r);
    }
    rlEnd();
}
void ring(Vector3 c, float r0, float r1, Color color, float a0d, float a1d, int segs) {
    rlBegin(RL_TRIANGLES);
    col(color);
    rlNormal3f(0, 1, 0);
    for (int i = 0; i < segs; i++) {
        float a0 = (a0d + (a1d - a0d) * i / segs) * D2R, a1 = (a0d + (a1d - a0d) * (i + 1) / segs) * D2R;
        float c0 = std::cos(a0), s0 = std::sin(a0), c1 = std::cos(a1), s1 = std::sin(a1);
        rlVertex3f(c.x + c0 * r0, c.y, c.z + s0 * r0); rlVertex3f(c.x + c1 * r1, c.y, c.z + s1 * r1); rlVertex3f(c.x + c0 * r1, c.y, c.z + s0 * r1);
        rlVertex3f(c.x + c0 * r0, c.y, c.z + s0 * r0); rlVertex3f(c.x + c1 * r0, c.y, c.z + s1 * r0); rlVertex3f(c.x + c1 * r1, c.y, c.z + s1 * r1);
    }
    rlEnd();
}
void ball(Vector3 c, float r, Color color, float squash) {
    const int RINGS = 5, SEGS = 8;
    rlBegin(RL_TRIANGLES);
    col(color);
    auto v = [&](int i, int j) {
        float th = 3.14159f * i / RINGS, ph = 6.28319f * j / SEGS;
        float x = std::sin(th) * std::cos(ph), y = std::cos(th), z = std::sin(th) * std::sin(ph);
        rlNormal3f(x, y, z);
        rlVertex3f(c.x + x * r, c.y + y * r * squash, c.z + z * r);
    };
    for (int i = 0; i < RINGS; i++) for (int j = 0; j < SEGS; j++) {
        v(i, j); v(i, j + 1); v(i + 1, j + 1);
        v(i, j); v(i + 1, j + 1); v(i + 1, j);
    }
    rlEnd();
}
void quadUp(Vector3 c, float w, float d, Color color) {
    rlBegin(RL_TRIANGLES);
    col(color);
    rlNormal3f(0, 1, 0);
    float x = w / 2, z = d / 2;
    rlVertex3f(c.x - x, c.y, c.z - z); rlVertex3f(c.x - x, c.y, c.z + z); rlVertex3f(c.x + x, c.y, c.z + z);
    rlVertex3f(c.x - x, c.y, c.z - z); rlVertex3f(c.x + x, c.y, c.z + z); rlVertex3f(c.x + x, c.y, c.z - z);
    rlEnd();
}

void softShadow(Vector3 c, float w, float d, float soft, unsigned char alpha) {
    float x0 = c.x - w / 2, x1 = c.x + w / 2, z0 = c.z - d / 2, z1 = c.z + d / 2;
    float X[4] = {x0 - soft, x0, x1, x1 + soft}, Z[4] = {z0 - soft, z0, z1, z1 + soft};
    rlBegin(RL_TRIANGLES);
    rlNormal3f(0, 0, 0);
    auto v = [&](int i, int j) { bool in = (i == 1 || i == 2) && (j == 1 || j == 2); rlColor4ub(16, 12, 10, in ? alpha : 0); rlVertex3f(X[i], c.y, Z[j]); };
    for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) {
        v(i, j); v(i, j + 1); v(i + 1, j + 1);
        v(i, j); v(i + 1, j + 1); v(i + 1, j);
    }
    rlEnd();
}

Vector3 Frame::dir(float x, float y, float z) const {
    float c = std::cos(yaw * D2R), s = std::sin(yaw * D2R);
    return {x * c + z * s, y, -x * s + z * c};
}
Vector3 Frame::at(float x, float y, float z) const { Vector3 d = dir(x, y, z); return {o.x + d.x, o.y + d.y, o.z + d.z}; }
Vector2 Frame::local(Vector3 w) const {
    float c = std::cos(yaw * D2R), s = std::sin(yaw * D2R), dx = w.x - o.x, dz = w.z - o.z;
    return {dx * c - dz * s, dx * s + dz * c};
}

float pizzaMetres(int size) { static const float R[] = {5, 6, 7, 8}; return R[std::clamp(size, 0, 3)] * 0.0254f; }

void topping(int ing, int var, float s, float baked) {
    Color c = ingColor(ing);
    float lift = var * 0.0004f;
    switch (ing) {
        case MOZZ: case PROVOLONE: case CHEDDAR:
            if (baked > 0.5f) {
                Color m = ing == CHEDDAR ? Color{238, 150, 52, 255} : ing == PROVOLONE ? Color{250, 232, 180, 255} : Color{250, 220, 138, 255};
                cyl({0, 0, 0}, 0.5f * s, 0.0018f + lift, m, 8);
                if (var == 0) disc({0.1f * s, 0.0026f + lift, 0.05f * s}, 0.16f * s, {206, 140, 64, 255}, 6);
            } else box({0, 0.002f + lift, 0}, {(ing == PROVOLONE ? 1.0f : 0.85f) * s, 0.004f, 0.17f * s}, c);
            break;
        case MOZZ_BALLS:
            if (baked > 0.5f) cyl({0, 0, 0}, 0.7f * s, 0.005f, {255, 250, 228, 255}, 10);
            else ball({0, 0.4f * s, 0}, 0.5f * s, c, 0.8f);
            break;
        case PARMESAN:
            for (int i = 0; i < 3; i++) box({(i - 1) * 0.16f * s, 0.0035f + lift, (i == 1 ? 0.12f : -0.05f) * s}, {0.1f * s, 0.002f, 0.1f * s}, baked > 0.5f ? Color{240, 208, 150, 255} : c);
            break;
        case FETA: box({0, 0.13f * s, 0}, {0.3f * s, 0.26f * s, 0.26f * s}, c); break;
        case ONION:
            ring({0, 0.004f + lift, -0.2f * s}, 0.3f * s, 0.46f * s, {176, 110, 180, 255}, 20, 160, 8);
            ring({0, 0.0046f + lift, -0.2f * s}, 0.34f * s, 0.42f * s, {246, 232, 246, 255}, 22, 158, 8);
            break;
        case PEPPER: ring({0, 0.005f + lift, -0.2f * s}, 0.27f * s, 0.48f * s, c, 30, 150, 8); break;
        case PINEAPPLE: box({0, 0.12f * s, 0}, {0.5f * s, 0.24f * s, 0.32f * s}, c); break;
        case TOMATO:
            box({0, 0.1f * s, 0}, {0.5f * s, 0.2f * s, 0.5f * s}, c);
            disc({0, 0.2f * s + 0.0006f, 0}, 0.12f * s, {246, 150, 110, 255}, 6);
            break;
        case MUSHROOM:
            cyl({0, 0, 0}, 0.42f * s, 0.1f * s, c, 8);
            box({0, 0.06f * s, 0.3f * s}, {0.24f * s, 0.11f * s, 0.34f * s}, {232, 216, 192, 255});
            break;
        case OLIVE:
            ring({0, 0.16f * s + lift, 0}, 0.12f * s, 0.3f * s, c, 0, 360, 10);
            cyl({0, 0, 0}, 0.3f * s, 0.15f * s, {30, 26, 34, 255}, 10);
            disc({0, 0.16f * s + lift + 0.0004f, 0}, 0.12f * s, {120, 60, 50, 255}, 8);
            break;
        case SAUSAGE:
            ball({-0.1f * s, 0.2f * s, 0}, 0.27f * s, c, 0.8f);
            ball({0.15f * s, 0.16f * s, 0.06f * s}, 0.2f * s, ui::shade(c, 1.12f), 0.8f);
            break;
        case PEPPERONI:
            cyl({0, 0, 0}, 0.7f * s, 0.0035f + lift, {150, 28, 28, 255}, 14);
            disc({0, 0.004f + lift, 0}, 0.62f * s, baked > 0.5f ? Color{178, 42, 32, 255} : c, 14);
            disc({0.25f * s, 0.0044f + lift, -0.15f * s}, 0.09f * s, {150, 30, 28, 255}, 6);
            disc({-0.2f * s, 0.0044f + lift, 0.25f * s}, 0.08f * s, {214, 84, 62, 255}, 6);
            break;
        case HAM:
            cyl({0, 0, 0}, 0.9f * s, 0.003f + lift, {198, 110, 118, 255}, 14);
            disc({0, 0.0035f + lift, 0}, 0.82f * s, c, 14);
            break;
        case BACON:
            box({0, 0.004f + lift, 0}, {0.64f * s, 0.006f, 0.3f * s}, c);
            box({0, 0.0075f + lift, 0}, {0.64f * s, 0.001f, 0.08f * s}, {236, 190, 160, 255});
            break;
        case BEEF: ball({0, 0.16f * s, 0}, 0.23f * s, c, 0.8f); break;
        case CHICKEN: box({0, 0.14f * s, 0}, {0.56f * s, 0.28f * s, 0.4f * s}, c); break;
    }
}

void pizza(const Pizza& p, const Frame& f, float lx, float lz, float bakeShow) {
    float b = bakeShow >= 0 ? bakeShow : p.baked, R = pizzaMetres(p.size), s = 0.0254f;
    float rim = p.crust == THIN ? 0.045f : 0.085f, h = p.crust == PAN ? 0.022f : p.crust == THIN ? 0.007f : 0.012f;
    rlPushMatrix();
    Vector3 o = f.at(lx, 0, lz);
    rlTranslatef(o.x, o.y, o.z);
    rlRotatef(f.yaw, 0, 1, 0);
    Color crust = p.crust == THIN ? ui::mix({238, 222, 182, 255}, {216, 172, 104, 255}, b) : ui::mix({238, 220, 178, 255}, p.crust == PAN ? Color{190, 128, 66, 255} : Color{208, 150, 84, 255}, b);
    if (p.crust == PAN) {   // in its pan
        cyl({0, 0, 0}, R + 0.012f, h + 0.004f, {58, 58, 64, 255}, 28);
        ring({0, h + 0.0045f, 0}, R, R + 0.012f, {80, 80, 88, 255}, 0, 360, 28);
    }
    cyl({0, 0, 0}, R, h + 0.004f, crust, 28, R * 0.985f);
    float top = h + 0.0005f;
    disc({0, h + 0.0045f, 0}, R * (1 - rim), ui::mix({246, 232, 198, 255}, {234, 200, 136, 255}, b), 28);
    top = h + 0.005f;
    if (p.crust == THIN) {   // a stamped cracker: rows of docking holes, a flat edge, no puff
        for (float gx = -R * 0.95f; gx < R * 0.95f; gx += 0.036f) for (float gz = -R * 0.95f; gz < R * 0.95f; gz += 0.036f)
            if (gx * gx + gz * gz < R * R * 0.86f) g3::disc({gx, top + 0.0001f, gz}, 0.0017f, {150, 108, 62, 255}, 5);
    }
    // sauce: flat dabs
    for (size_t i = 0; i < p.stamps.size(); i++) {
        auto& st = p.stamps[i];
        Color c = ingColor(st.sauce);
        if (b > 0.5f) c = ui::shade(c, 0.92f);
        disc({st.x * R, top + 0.0002f + (i % 5) * 0.00006f, st.y * R}, STAMP_R * R, c, 10);
    }
    top += 0.0008f;
    for (auto& bt : p.bits) {
        rlPushMatrix();
        rlTranslatef(bt.x * R, top, bt.y * R);
        rlRotatef(-bt.rot * 57.2958f, 0, 1, 0);
        topping(bt.ing, bt.var, s, b);
        rlPopMatrix();
    }
    for (auto& ct : p.cuts) {   // a cut shows as a dark groove
        float ang = ct.ang, half = std::sqrt(std::max(0.f, 1 - ct.off * ct.off)) * R;
        float nx = -std::sin(ang), ny = std::cos(ang);
        rlPushMatrix();
        rlTranslatef(nx * ct.off * R, top + 0.011f, ny * ct.off * R);
        rlRotatef(-ang * 57.2958f, 0, 1, 0);
        box({0, 0, 0}, {half * 2, 0.0012f, 0.004f}, {84, 44, 24, 255});
        rlPopMatrix();
    }
    rlPopMatrix();
}

void binFill(int ing, const Frame& f, float lx, float lz, float size) {
    Vector3 c = f.at(lx, 0, lz);
    box({c.x, c.y - 0.03f, c.z}, {size, 0.06f, size}, {72, 78, 86, 255});                 // the pan
    Color fill = info(ing).kind == SAUCE ? ingColor(ing) : ui::shade(ingColor(ing), 0.8f);
    box({c.x, c.y + 0.001f, c.z}, {size - 0.014f, 0.004f, size - 0.014f}, fill);
    if (info(ing).kind == SAUCE) {   // a ladle handle
        box({c.x + 0.02f, c.y + 0.03f, c.z + 0.02f}, {0.012f, 0.06f, 0.012f}, {200, 204, 210, 255});
        return;
    }
    static const float OFF[7][3] = {{-0.3f, -0.28f, 20}, {0.26f, -0.3f, 110}, {0, 0, 60}, {-0.28f, 0.28f, 200}, {0.3f, 0.26f, 300}, {0.02f, -0.34f, 160}, {-0.04f, 0.34f, 250}};
    int n = info(ing).piece ? 4 : 7;
    float sc = (ing == HAM ? 0.6f : ing == PEPPERONI ? 0.75f : 1.0f) * 0.0254f * 1.25f;
    for (int i = 0; i < n; i++) {
        rlPushMatrix();
        rlTranslatef(c.x + OFF[i][0] * size, c.y + 0.004f, c.z + OFF[i][1] * size);
        rlRotatef(OFF[i][2], 0, 1, 0);
        topping(ing, i % 4, sc, 0);
        rlPopMatrix();
    }
}

void pizzaBox(Vector3 b, float yaw, bool open) {
    box({b.x, b.y + 0.022f, b.z}, {0.42f, 0.044f, 0.42f}, {236, 226, 206, 255}, yaw);
    box({b.x, b.y + 0.045f, b.z}, {0.3f, 0.002f, 0.12f}, {206, 60, 46, 255}, yaw);
    (void)open;
}

}  // namespace g3
