// Makeline - the drawings: pizzas and everything on them, customers, tickets.
#include "game.hpp"
#include "rlgl.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace pz;
using ui::INK;

Color ingColor(int ing) {
    static const Color C[ING_N] = {
        {198, 50, 38, 255}, {246, 238, 216, 255}, {236, 220, 160, 255}, {112, 50, 30, 255},
        {250, 238, 190, 255}, {255, 255, 250, 255}, {252, 244, 214, 255}, {245, 160, 50, 255}, {250, 236, 200, 255}, {252, 252, 246, 255},
        {214, 170, 214, 255}, {78, 156, 66, 255}, {250, 212, 74, 255}, {226, 74, 54, 255}, {206, 182, 152, 255}, {40, 34, 44, 255},
        {150, 96, 62, 255}, {190, 42, 36, 255}, {236, 150, 152, 255}, {168, 62, 46, 255}, {106, 72, 52, 255}, {240, 216, 172, 255}};
    return C[std::clamp(ing, 0, ING_N - 1)];
}

float pizzaRadius(int size, float base) { static const float K[] = {5 / 7.f, 6 / 7.f, 1, 8 / 7.f}; return base * K[std::clamp(size, 0, 3)]; }

// one bit of a topping at the origin; s = pixels per inch
static void bitShape(int ing, int var, float s, float baked) {
    Color c = ingColor(ing), dk = ui::shade(c, 0.72f);
    switch (ing) {
        case MOZZ: case PROVOLONE: case CHEDDAR: {
            if (baked > 0.5f) {   // melted together
                Color m = ing == CHEDDAR ? Color{240, 150, 50, 255} : ing == PROVOLONE ? Color{250, 232, 178, 255} : Color{250, 222, 140, 255};
                DrawCircleV({0, 0}, 0.52f * s, m);
                if (var == 0) DrawCircleV({0.1f * s, 0.05f * s}, 0.16f * s, {214, 150, 70, 255});
            } else {
                float len = (ing == PROVOLONE ? 1.0f : 0.85f) * s;
                DrawLineEx({-len / 2, 0}, {len / 2, 0}, 0.2f * s + 2, dk);
                DrawLineEx({-len / 2 + 1, 0}, {len / 2 - 1, 0}, 0.2f * s, c);
            }
            break;
        }
        case MOZZ_BALLS: {
            float r = (baked > 0.5f ? 0.72f : 0.55f) * s;
            DrawCircleV({0, 0}, r + 2, {180, 170, 150, 255});
            DrawCircleV({0, 0}, r, baked > 0.5f ? Color{255, 250, 226, 255} : c);
            DrawCircleV({-r * 0.3f, -r * 0.3f}, r * 0.25f, {255, 255, 255, 255});
            break;
        }
        case PARMESAN:
            for (int i = 0; i < 3; i++) DrawCircleV({(i - 1) * 0.16f * s, (i == 1 ? 0.12f : -0.05f) * s}, 0.075f * s + 0.6f, baked > 0.5f ? Color{240, 210, 150, 255} : c);
            break;
        case FETA:
            DrawRectangleRec({-0.15f * s - 1, -0.13f * s - 1, 0.3f * s + 2, 0.26f * s + 2}, {190, 186, 176, 255});
            DrawRectangleRec({-0.15f * s, -0.13f * s, 0.3f * s, 0.26f * s}, c);
            break;
        case ONION:
            DrawRing({0, -0.2f * s}, 0.32f * s, 0.46f * s, 20, 160, 12, {150, 90, 160, 255});
            DrawRing({0, -0.2f * s}, 0.35f * s, 0.43f * s, 22, 158, 12, {244, 228, 244, 255});
            break;
        case PEPPER:
            DrawRing({0, -0.2f * s}, 0.28f * s, 0.48f * s, 30, 150, 10, ui::shade(c, 0.7f));
            DrawRing({0, -0.2f * s}, 0.31f * s, 0.45f * s, 33, 147, 10, c);
            break;
        case PINEAPPLE:
            DrawPoly({0, 0}, 3, 0.4f * s + 2, 0, {200, 150, 40, 255});
            DrawPoly({0, 0}, 3, 0.4f * s, 0, c);
            break;
        case TOMATO:
            DrawRectangleRounded({-0.27f * s - 1.5f, -0.27f * s - 1.5f, 0.54f * s + 3, 0.54f * s + 3}, 0.5f, 4, dk);
            DrawRectangleRounded({-0.27f * s, -0.27f * s, 0.54f * s, 0.54f * s}, 0.5f, 4, c);
            DrawCircleV({0, 0}, 0.1f * s, {246, 150, 110, 255});
            break;
        case MUSHROOM:
            DrawCircleSector({0, 0}, 0.46f * s + 1.5f, 180, 360, 10, {130, 104, 80, 255});
            DrawRectangleRec({-0.13f * s - 1.5f, -1, 0.26f * s + 3, 0.36f * s + 2}, {130, 104, 80, 255});
            DrawCircleSector({0, 0}, 0.46f * s, 180, 360, 10, c);
            DrawRectangleRec({-0.13f * s, -1, 0.26f * s, 0.36f * s}, {232, 216, 192, 255});
            break;
        case OLIVE:
            DrawRing({0, 0}, 0.13f * s, 0.3f * s, 0, 360, 14, c);
            break;
        case SAUSAGE:
            DrawCircleV({-0.12f * s, 0}, 0.27f * s + 1.5f, {92, 56, 36, 255});
            DrawCircleV({0.14f * s, 0.06f * s}, 0.22f * s + 1.5f, {92, 56, 36, 255});
            DrawCircleV({-0.12f * s, 0}, 0.27f * s, c);
            DrawCircleV({0.14f * s, 0.06f * s}, 0.22f * s, ui::shade(c, 1.1f));
            break;
        case PEPPERONI:
            DrawCircleV({0, 0}, 0.7f * s, {140, 26, 26, 255});
            DrawCircleV({0, 0}, 0.7f * s - 2.5f, baked > 0.5f ? Color{176, 40, 30, 255} : c);
            DrawCircleV({0.25f * s, -0.15f * s}, 0.09f * s, {150, 30, 28, 255});
            DrawCircleV({-0.2f * s, 0.25f * s}, 0.08f * s, {150, 30, 28, 255});
            DrawCircleV({-0.15f * s, -0.3f * s}, 0.07f * s, {214, 80, 60, 255});
            break;
        case HAM:
            DrawCircleV({0, 0}, 0.9f * s, {196, 108, 116, 255});
            DrawCircleV({0, 0}, 0.9f * s - 2.5f, c);
            DrawCircleV({0.2f * s, 0.1f * s}, 0.2f * s, {242, 172, 172, 255});
            break;
        case BACON:
            DrawRectangleRec({-0.32f * s - 1, -0.15f * s - 1, 0.64f * s + 2, 0.3f * s + 2}, {110, 36, 28, 255});
            DrawRectangleRec({-0.32f * s, -0.15f * s, 0.64f * s, 0.3f * s}, c);
            DrawRectangleRec({-0.32f * s, -0.03f * s, 0.64f * s, 0.08f * s}, {236, 190, 160, 255});
            break;
        case BEEF:
            DrawCircleV({0, 0}, 0.24f * s + 1.5f, {64, 40, 28, 255});
            DrawCircleV({0, 0}, 0.24f * s, c);
            DrawCircleV({0.08f * s, -0.06f * s}, 0.1f * s, ui::shade(c, 1.25f));
            break;
        case CHICKEN:
            DrawRectangleRounded({-0.28f * s - 1.5f, -0.2f * s - 1.5f, 0.56f * s + 3, 0.4f * s + 3}, 0.4f, 4, {170, 136, 90, 255});
            DrawRectangleRounded({-0.28f * s, -0.2f * s, 0.56f * s, 0.4f * s}, 0.4f, 4, c);
            DrawLineEx({-0.15f * s, -0.14f * s}, {-0.05f * s, 0.14f * s}, 1.5f, {150, 100, 60, 255});
            DrawLineEx({0.05f * s, -0.14f * s}, {0.15f * s, 0.14f * s}, 1.5f, {150, 100, 60, 255});
            break;
    }
}
static void drawBit(int ing, int var, float x, float y, float rot, float s, float baked) {
    rlPushMatrix();
    rlTranslatef(x, y, 0);
    rlRotatef(rot * 57.2958f, 0, 0, 1);
    bitShape(ing, var, s, baked);
    rlPopMatrix();
}

void drawPizza(const Pizza& p, Vector2 c, float r, float bakeShow) {
    static const float RIN[] = {5, 6, 7, 8};
    float b = bakeShow >= 0 ? bakeShow : p.baked, s = r / RIN[std::clamp(p.size, 0, 3)];
    float rim = p.crust == THIN ? 0.045f : 0.085f;
    DrawCircleV({c.x + 5, c.y + 8}, r + 2, ui::SHADOW);
    if (p.crust == PAN) {   // in its pan
        DrawCircleV(c, r + 12, INK);
        DrawCircleV(c, r + 9, {70, 70, 76, 255});
    }
    DrawCircleV(c, r + 3, INK);
    DrawCircleV(c, r, ui::mix({238, 220, 178, 255}, p.crust == PAN ? Color{190, 128, 66, 255} : Color{210, 154, 88, 255}, b));
    DrawCircleV(c, r * (1 - rim), ui::mix({246, 232, 198, 255}, {234, 200, 136, 255}, b));
    for (auto& st : p.stamps) {
        Color col = ingColor(st.sauce);
        if (b > 0.5f) col = ui::shade(col, 0.92f);
        DrawCircleV({c.x + st.x * r, c.y + st.y * r}, STAMP_R * r, col);
    }
    for (auto& bt : p.bits) drawBit(bt.ing, bt.var, c.x + bt.x * r, c.y + bt.y * r, bt.rot, s, b);
    for (auto& ct : p.cuts) {
        float nx = -std::sin(ct.ang), ny = std::cos(ct.ang), dx = std::cos(ct.ang), dy = std::sin(ct.ang);
        float h = std::sqrt(std::max(0.f, 1 - ct.off * ct.off)) * r;
        Vector2 m{c.x + nx * ct.off * r, c.y + ny * ct.off * r};
        DrawLineEx({m.x - dx * h, m.y - dy * h}, {m.x + dx * h, m.y + dy * h}, 4, {90, 48, 26, 230});
        DrawLineEx({m.x - dx * h, m.y - dy * h}, {m.x + dx * h, m.y + dy * h}, 1.5f, {250, 226, 170, 200});
    }
}

void drawIngredientIcon(int ing, Vector2 c, float s) {
    if (info(ing).kind == SAUCE) {
        DrawCircleV(c, s * 1.0f + 2, INK);
        DrawCircleV(c, s * 1.0f, ingColor(ing));
        DrawCircleV({c.x - s * 0.35f, c.y - s * 0.35f}, s * 0.22f, ui::shade(ingColor(ing), 1.25f));
        return;
    }
    static const float OFF[5][3] = {{-0.9f, -0.5f, 0.4f}, {0.8f, -0.6f, 2.0f}, {0, 0.1f, 1.1f}, {-0.8f, 0.7f, 3.0f}, {0.9f, 0.7f, 4.4f}};
    int n = info(ing).piece ? 3 : 5;
    float k = (ing == PEPPERONI || ing == HAM) ? 0.62f : ing == MOZZ_BALLS ? 0.8f : 1.0f;
    for (int i = 0; i < n; i++) drawBit(ing, i, c.x + OFF[i][0] * s * 0.75f, c.y + OFF[i][1] * s * 0.6f, OFF[i][2], s * k, 0);
}

void drawCustomer(const Person& who, float x, float baseY, float k, float anger, float bob) {
    uint32_t L = who.look;
    static const Color SKIN[] = {{250, 220, 190, 255}, {238, 196, 160, 255}, {210, 160, 120, 255}, {160, 110, 80, 255}, {110, 76, 56, 255}};
    static const Color SHIRT[] = {{86, 134, 190, 255}, {92, 158, 92, 255}, {214, 120, 60, 255}, {150, 100, 170, 255}, {200, 80, 90, 255}, {90, 90, 100, 255}, {230, 200, 90, 255}, {70, 150, 150, 255}};
    static const Color HAIR[] = {{60, 40, 30, 255}, {30, 26, 28, 255}, {190, 150, 80, 255}, {150, 70, 40, 255}, {200, 200, 200, 255}};
    Color skin = ui::mix(SKIN[L % 5], {226, 90, 70, 255}, anger * 0.45f), shirt = SHIRT[(L >> 3) % 8], hair = HAIR[(L >> 9) % 5];
    int style = (L >> 6) % 5;
    float wide = 1.0f + ((L >> 12) % 4) * 0.08f;
    float shake = anger > 0.6f ? std::sin(ui::time * 22 + L) * 1.5f * anger : 0;
    x += shake;
    float y = baseY + std::sin(ui::time * (1.6f + anger * 3) + (L & 31)) * 2.5f * k + bob;
    float tw = 120 * k * wide, th = 150 * k, hr = 44 * k;
    float top = y - th, hy = top - hr * 0.72f;
    // torso
    DrawRectangleRounded({x - tw / 2 - 3, top - 3, tw + 6, th + 40}, 0.45f, 8, INK);
    DrawRectangleRounded({x - tw / 2, top, tw, th + 40}, 0.45f, 8, shirt);
    DrawRectangleRec({x - 12 * k, top - 8 * k, 24 * k, 20 * k}, ui::shade(skin, 0.9f));
    if (anger > 0.5f) {   // arms crossed
        DrawLineEx({x - tw / 2 + 6, top + th * 0.55f}, {x + tw / 2 - 12, top + th * 0.42f}, 14 * k, INK);
        DrawLineEx({x - tw / 2 + 6, top + th * 0.55f}, {x + tw / 2 - 12, top + th * 0.42f}, 14 * k - 5, ui::shade(shirt, 0.85f));
        DrawLineEx({x + tw / 2 - 6, top + th * 0.55f}, {x - tw / 2 + 12, top + th * 0.42f}, 14 * k, INK);
        DrawLineEx({x + tw / 2 - 6, top + th * 0.55f}, {x - tw / 2 + 12, top + th * 0.42f}, 14 * k - 5, shirt);
    }
    // head
    if (style == 3) DrawRectangleRounded({x - hr - 5, hy - hr * 0.2f, 2 * hr + 10, hr * 1.9f}, 0.5f, 6, hair);
    DrawCircleV({x, hy}, hr + 3, INK);
    DrawCircleV({x, hy}, hr, skin);
    if (style == 1 || style == 2 || style == 3) DrawCircleSector({x, hy - 2}, hr + 2, 180, 360, 16, hair);
    if (style == 2) { DrawCircleV({x, hy - hr - 6 * k}, 15 * k, INK); DrawCircleV({x, hy - hr - 6 * k}, 15 * k - 3, hair); }
    if (style == 0) DrawRing({x, hy}, hr - 2, hr + 2, 200, 340, 12, hair);
    if (style == 4) {
        DrawCircleSector({x, hy - 4}, hr + 4, 180, 360, 16, ui::shade(shirt, 0.7f));
        DrawRectangleRounded({x - hr - 4, hy - 8, hr * 1.5f, 9 * k}, 0.8f, 4, ui::shade(shirt, 0.55f));
    }
    // face
    float ex = 16 * k, ey = hy + 2 * k;
    DrawCircleV({x - ex, ey}, 4.5f * k, INK);
    DrawCircleV({x + ex, ey}, 4.5f * k, INK);
    float br = anger * 9 * k;
    DrawLineEx({x - ex - 9 * k, ey - 12 * k - br * 0.3f}, {x - ex + 8 * k, ey - 11 * k + br}, 3.5f * k, INK);
    DrawLineEx({x + ex + 9 * k, ey - 12 * k - br * 0.3f}, {x + ex - 8 * k, ey - 11 * k + br}, 3.5f * k, INK);
    if ((L >> 14) & 1) {   // glasses
        DrawRing({x - ex, ey}, 9 * k, 11 * k, 0, 360, 16, INK);
        DrawRing({x + ex, ey}, 9 * k, 11 * k, 0, 360, 16, INK);
        DrawLineEx({x - ex + 10 * k, ey}, {x + ex - 10 * k, ey}, 2, INK);
    }
    float my = hy + 22 * k;
    if (anger > 0.8f) DrawEllipse((int)x, (int)(my + 2 * k), 11 * k, 8 * k, INK);
    else if (anger > 0.3f) DrawRing({x, my + 12 * k}, 11 * k, 14 * k, 215, 325, 10, INK);
    else DrawLineEx({x - 10 * k, my}, {x + 10 * k, my}, 3 * k, INK);
}

void drawTicket(const Ticket& k, const Order* o, Rectangle r, bool selected, float size) {
    DrawRectangleRec({r.x + 3, r.y + 5, r.width, r.height}, ui::SHADOW);
    DrawRectangleRec({r.x - 2, r.y - 2, r.width + 4, r.height + 4}, selected ? ui::RED : INK);
    DrawRectangleRec(r, ui::PAPER);
    Color svc = !o ? ui::GREY : o->service == DELIVERY ? ui::BLUE : o->service == CARRYOUT ? ui::GREEN : ui::YELLOW;
    DrawRectangleRec({r.x, r.y, r.width, size * 1.5f}, svc);
    char b[64];
    snprintf(b, sizeof b, "#%d  %d/%d", k.order, k.idx, k.of);
    ui::text(b, r.x + 8, r.y + size * 0.22f, size * 1.05f, INK);
    ui::text(!o ? "" : o->service == DELIVERY ? "DELIVERY" : o->service == CARRYOUT ? "CARRYOUT" : "COUNTER", r.x + r.width - 8, r.y + size * 0.32f, size * 0.8f, INK, 2);
    float y = r.y + size * 1.75f, x = r.x + 8, lh = size * 1.2f;
    ui::text(std::string(sizeName(k.spec.size)) + " " + sizeInches(k.spec.size), x, y, size * 1.15f, INK); y += lh * 1.15f;
    ui::text(crustName(k.spec.crust), x, y, size, k.spec.crust == HAND ? INK : ui::DKRED); y += lh;
    std::string sauce = info(k.spec.sauce).name;
    if (k.spec.sauceLevel != NORMAL) sauce = std::string(levelName(k.spec.sauceLevel)) + " " + sauce;
    ui::text(sauce + (k.spec.sauce == MARINARA ? " sauce" : ""), x, y, size, k.spec.sauce == MARINARA && k.spec.sauceLevel == NORMAL ? INK : ui::DKRED); y += lh;
    const Item* mz = k.spec.find(MOZZ);
    if (!mz) { ui::text("NO CHEESE", x, y, size, ui::DKRED); y += lh; }
    else if (mz->level != NORMAL) { ui::text(std::string(levelName(mz->level)) + " cheese", x, y, size, ui::DKRED); y += lh; }
    DrawLineEx({x, y + 2}, {r.x + r.width - 8, y + 2}, 1.5f, ui::GREY); y += 6;
    for (auto& it : k.spec.items) {
        if (it.ing == MOZZ) continue;
        if (y + lh > r.y + r.height) { ui::text("...", x, y - 4, size, INK); break; }
        std::string s = it.part == LEFT ? "L: " : it.part == RIGHT ? "R: " : "";
        if (it.level != NORMAL) s += std::string(levelName(it.level)) + " ";
        s += info(it.ing).name;
        float fs = size;
        while (fs > 11 && ui::textW(s, fs) > r.width - 14) fs -= 1;
        ui::text(s, x, y, fs, it.part != WHOLE || it.level != NORMAL ? ui::DKRED : INK); y += lh;
    }
}
