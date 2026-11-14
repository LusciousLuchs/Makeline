// Makeline - drawing helpers: the 1600x900 canvas, text, chunky buttons, the palette.
#pragma once
#include "raylib.h"
#include <string>
// raylib's own colour names would clash with the palette below
#undef RED
#undef GREEN
#undef YELLOW
#undef BLUE

namespace ui {

constexpr float W = 1600, H = 900;
// the palette
constexpr Color INK{52, 36, 30, 255}, CREAM{247, 236, 214, 255}, PAPER{255, 250, 238, 255}, RED{214, 66, 50, 255}, DKRED{160, 40, 34, 255},
    GREEN{92, 158, 92, 255}, YELLOW{246, 194, 66, 255}, STEEL{176, 186, 194, 255}, DKSTEEL{120, 130, 140, 255}, WOOD{184, 128, 82, 255},
    DKWOOD{134, 88, 56, 255}, BLUE{86, 134, 190, 255}, GREY{150, 140, 130, 255}, SHADOW{30, 20, 16, 70};

extern Vector2 M;                       // the mouse on the canvas
extern bool pressed, down, released;    // left button
extern bool rdown;                      // right button
extern float time;                      // seconds since the game started
extern bool clickUsed;                  // a button already took this frame's click

void loadFonts();
void beginFrame();                      // reads the mouse (or the test script's)
void script(const std::string& s);      // "frame:x,y;frame:x,y>x2,y2" clicks and drags for automated runs
void text(const std::string& s, float x, float y, float size, Color c, int align = 0);   // 0 left, 1 centred, 2 right
float textW(const std::string& s, float size);
// wrapped text in a box; returns the height it took
float wrap(const std::string& s, float x, float y, float w, float size, Color c, bool draw = true, int maxChars = -1);
void panel(Rectangle r, Color fill, float radius = 14, float line = 3, bool shadow = true);
bool hover(Rectangle r);
bool button(Rectangle r, const std::string& label, Color fill, bool enabled = true, float size = 22, bool lit = false);
void stars(float x, float y, float size, float value, Color off = Color{200, 186, 160, 255});
void star(float cx, float cy, float r, Color c);
Color mix(Color a, Color b, float t);
Color shade(Color c, float k);          // k < 1 darker, > 1 lighter
float ease(float t);                    // smooth 0..1

}  // namespace ui
