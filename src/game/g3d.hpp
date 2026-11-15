// Makeline - the 3D drawing kit: a simple lit shader and low-poly shapes. Sizes are in metres, y is up.
#pragma once
#include "raylib.h"
#include "talk.hpp"

namespace g3 {

void init();
void begin(const Camera3D& cam);     // lit 3D drawing from here...
void end();                          // ...to here
void box(Vector3 centre, Vector3 size, Color c, float yawDeg = 0);
void cyl(Vector3 base, float r, float h, Color c, int segs = 16, float rTop = -1);   // upright, capped on top
void disc(Vector3 centre, float r, Color c, int segs = 20);                           // flat, facing up
void ring(Vector3 centre, float r0, float r1, Color c, float a0 = 0, float a1 = 360, int segs = 16);   // flat ring or arc, facing up
void ball(Vector3 centre, float r, Color c, float squash = 1);
void quadUp(Vector3 centre, float w, float d, Color c);                               // a floor tile

// a place things are laid out around: local x is to the right on screen, local z is toward the player
struct Frame {
    Vector3 o; float yaw;            // degrees about y
    Vector3 at(float x, float y, float z) const;
    Vector3 dir(float x, float y, float z) const;
    Vector2 local(Vector3 world) const;   // world point -> local (x, z)
};

float pizzaMetres(int size);         // its radius
// a pizza lying at `base` (the surface it sits on), its x axis along the frame's local x
void pizza(const pz::Pizza& p, const Frame& f, float lx, float lz, float bakeShow = -1);
void topping(int ing, int var, float inch, float baked);   // one bit at the origin (inside a pushed matrix)
void binFill(int ing, const Frame& f, float lx, float lz, float size);   // a makeline pan and what's in it
void person(const pz::Person& who, Vector3 feet, float yawDeg, float anger, float walk);
void pizzaBox(Vector3 base, float yawDeg, bool open = false);
void softShadow(Vector3 centre, float w, float d, float soft, unsigned char alpha);   // dark under and around something, fading out
// a head-and-shoulders picture of someone for the screens: prepare it outside any drawing mode, then ask for it
void portraitPrepare(const pz::Person& who, bool uniform);
bool portrait(const pz::Person& who, bool uniform, Texture2D& out);

}  // namespace g3
