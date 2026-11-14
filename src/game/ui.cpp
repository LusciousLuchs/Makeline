// Makeline - drawing helpers.
#include "ui.hpp"
#include "assets.inc"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

const unsigned char* logoJpg(int* size) { *size = (int)sizeof OAB_LOGO_JPG; return OAB_LOGO_JPG; }

namespace ui {

Vector2 M{0, 0};
bool pressed = false, down = false, released = false, rdown = false, clickUsed = false;
float time = 0, scale = 1;
Vector2 offset{0, 0};
Vector2 toScreen(Vector2 c) { return {c.x * scale + offset.x, c.y * scale + offset.y}; }
Vector2 fromScreen(Vector2 p) { return {(p.x - offset.x) / scale, (p.y - offset.y) / scale}; }

namespace {
Font fBig{}, fSmall{};
struct Step { int frame; Vector2 a, b; bool drag; };
std::vector<Step> gScript;
int gFrame = 0;
bool scripted = false;
}  // namespace

void loadFonts() {
    fBig = LoadFontFromMemory(".otf", FONT_OTF, (int)sizeof FONT_OTF, 56, nullptr, 0);
    fSmall = LoadFontFromMemory(".otf", FONT_OTF, (int)sizeof FONT_OTF, 26, nullptr, 0);
    SetTextureFilter(fBig.texture, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(fSmall.texture, TEXTURE_FILTER_BILINEAR);
}

void script(const std::string& s) {
    scripted = true;
    size_t at = 0;
    while (at < s.size()) {
        size_t end = s.find(';', at);
        if (end == std::string::npos) end = s.size();
        std::string one = s.substr(at, end - at);
        Step st{};
        float x1, y1, x2, y2;
        if (sscanf(one.c_str(), "%d:%f,%f>%f,%f", &st.frame, &x1, &y1, &x2, &y2) == 5) { st.a = {x1, y1}; st.b = {x2, y2}; st.drag = true; gScript.push_back(st); }
        else if (sscanf(one.c_str(), "%d:%f,%f", &st.frame, &x1, &y1) == 3) { st.a = st.b = {x1, y1}; gScript.push_back(st); }
        at = end + 1;
    }
}

void beginFrame() {
    time = (float)GetTime();
    scale = std::min(GetScreenWidth() / W, GetScreenHeight() / H);
    offset = {(GetScreenWidth() - W * scale) / 2, (GetScreenHeight() - H * scale) / 2};
    clickUsed = false;
    gFrame++;
    if (scripted) {   // automated runs: clicks last 2 frames, drags 12
        pressed = down = released = rdown = false;
        for (auto& st : gScript) {
            int len = st.drag ? 12 : 2, k = gFrame - st.frame;
            if (k < 0 || k > len) continue;
            float u = (float)k / len;
            M = {st.a.x + (st.b.x - st.a.x) * u, st.a.y + (st.b.y - st.a.y) * u};
            pressed = k == 0; down = k < len; released = k == len;
        }
        return;
    }
    M = fromScreen(GetMousePosition());
    pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    down = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    released = IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
    rdown = IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
}

static const Font& fontFor(float size) { return size <= 30 ? fSmall : fBig; }
float textW(const std::string& s, float size) { return MeasureTextEx(fontFor(size), s.c_str(), size, 0).x; }
void text(const std::string& s, float x, float y, float size, Color c, int align) {
    if (align) { float w = textW(s, size); x -= align == 1 ? w / 2 : w; }
    DrawTextEx(fontFor(size), s.c_str(), {std::round(x), std::round(y)}, size, 0, c);
}
float wrap(const std::string& s, float x, float y, float w, float size, Color c, bool draw, int maxChars) {
    float line = size * 1.22f, cy = y;
    std::string cur;
    int used = 0;
    size_t at = 0;
    auto flush = [&]() {
        if (draw && !cur.empty()) {
            std::string out = cur;
            if (maxChars >= 0) { int left = maxChars - used; if (left <= 0) out.clear(); else if ((int)out.size() > left) out.resize(left); }
            text(out, x, cy, size, c);
        }
        used += (int)cur.size() + 1;
        cy += line; cur.clear();
    };
    while (at < s.size()) {
        size_t sp = s.find(' ', at);
        if (sp == std::string::npos) sp = s.size();
        std::string word = s.substr(at, sp - at);
        std::string trial = cur.empty() ? word : cur + " " + word;
        if (!cur.empty() && textW(trial, size) > w) { flush(); cur = word; }
        else cur = trial;
        at = sp + 1;
    }
    if (!cur.empty()) flush();
    return cy - y;
}

Color mix(Color a, Color b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    return {(unsigned char)(a.r + (b.r - a.r) * t), (unsigned char)(a.g + (b.g - a.g) * t), (unsigned char)(a.b + (b.b - a.b) * t), (unsigned char)(a.a + (b.a - a.a) * t)};
}
Color shade(Color c, float k) {
    auto f = [&](unsigned char v) { return (unsigned char)std::clamp(v * k, 0.f, 255.f); };
    return {f(c.r), f(c.g), f(c.b), c.a};
}
float ease(float t) { t = std::clamp(t, 0.f, 1.f); return t * t * (3 - 2 * t); }

void panel(Rectangle r, Color fill, float radius, float line, bool shadow) {
    float rr = std::min(1.f, radius * 2 / std::min(r.width, r.height));
    if (shadow) DrawRectangleRounded({r.x + 4, r.y + 6, r.width, r.height}, rr, 8, SHADOW);
    if (line > 0) DrawRectangleRounded({r.x - line, r.y - line, r.width + 2 * line, r.height + 2 * line}, std::min(1.f, (radius + line) * 2 / std::min(r.width + 2 * line, r.height + 2 * line)), 8, INK);
    DrawRectangleRounded(r, rr, 8, fill);
}
bool hover(Rectangle r) { return CheckCollisionPointRec(M, r); }
bool button(Rectangle r, const std::string& label, Color fill, bool enabled, float size, bool lit) {
    bool h = enabled && hover(r), held = h && down;
    float dy = held ? 3 : 0;
    Color f = !enabled ? mix(fill, CREAM, 0.65f) : h ? shade(fill, 1.1f) : fill;
    float rr = std::min(1.f, 20.f / std::min(r.width, r.height));
    DrawRectangleRounded({r.x - 3, r.y - 3 + 5, r.width + 6, r.height + 6}, rr, 6, enabled ? INK : GREY);            // the lip it sits on
    DrawRectangleRounded({r.x - 3, r.y - 3 + dy, r.width + 6, r.height + 6}, rr, 6, lit ? Color{255, 255, 255, 255} : (enabled ? INK : GREY));
    DrawRectangleRounded({r.x, r.y + dy, r.width, r.height}, rr, 6, f);
    float lum = 0.3f * f.r + 0.6f * f.g + 0.1f * f.b;
    Color tc = !enabled ? GREY : lum < 150 ? PAPER : INK;
    float s = size;
    while (s > 12 && textW(label, s) > r.width - 10) s -= 1;
    text(label, r.x + r.width / 2, r.y + dy + (r.height - s) / 2, s, tc, 1);
    if (h && pressed && !clickUsed) { clickUsed = true; return true; }
    return false;
}
void star(float cx, float cy, float r, Color c) {
    Vector2 p[10];
    for (int i = 0; i < 10; i++) {
        float a = -PI / 2 + i * PI / 5, rr = i % 2 ? r * 0.45f : r;
        p[i] = {cx + std::cos(a) * rr, cy + std::sin(a) * rr};
    }
    for (int i = 0; i < 10; i++) DrawTriangle({cx, cy}, p[(i + 1) % 10], p[i], c);
}
void stars(float x, float y, float size, float value, Color off) {
    for (int i = 0; i < 5; i++) {
        float cx = x + size * (i * 1.05f + 0.5f), cy = y + size * 0.5f;
        star(cx, cy, size * 0.5f, off);
        float fill = std::clamp(value - i, 0.f, 1.f);
        if (fill > 0.05f) {
            star(cx, cy, size * 0.5f * (0.45f + 0.55f * fill), YELLOW);
        }
    }
}

}  // namespace ui
