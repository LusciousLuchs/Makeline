// Makeline - the game's state and its screens.
#pragma once
#include "raylib.h"
#include "store.hpp"
#include "ui.hpp"
#include "g3d.hpp"
#include <string>
#include <vector>

enum Mode { M_WALK, M_REGISTER, M_MAKE, M_CUT };
enum Sfx { SFX_CLICK, SFX_RING, SFX_DING, SFX_TALK, SFX_BAD, SFX_GOOD, SFX_OVEN, SFX_CUT, SFX_PLOP, SFX_DOOR, SFX_N };

struct Toast { std::string text; int stars; float age; };   // stars 0: a plain note
struct Vis { int id; float x, z, yaw, walk; bool out; };    // where a customer is standing in the shop right now

struct Game {
    pz::Store S;
    int mode = M_WALK;
    // you
    float px = 0, pz = 2.4f, yaw = 3.14159f, pitch = -0.1f;   // yaw 0 looks toward +z (the back wall)
    Camera3D cam{};
    bool camSnap = true;
    std::vector<Vis> people;
    std::string prompt;                  // what E does right now
    // the order screen
    int posPart = pz::WHOLE, posLevel = pz::NORMAL;
    // the makeline
    int pickTicket = -1;                 // the ticket picked on the rail
    int doughSize = -1, doughCrust = -1;
    int tool = -1;                       // the ingredient in hand
    float sprinkle = 0;                  // bits owed while the button is held
    Vector2 lastPiece{-9, -9};
    bool chart = false;
    pz::Rng fx{7};
    // the cut table
    bool cutting = false;
    Vector2 cutFrom{0, 0};
    std::vector<Toast> toasts;
    bool paused = false, help = false;
    bool office = false; int officeTab = 0;   // the manager's office (staff, hiring, the delivery map)
    bool playing = false;                     // a shift is on (not the end-of-day report)
};
extern Game G;
extern bool gShotMode;

void sfx(int which);
void audioInit();
void audioUpdate(bool ringing);

// art.cpp
void drawIngredientIcon(int ing, Vector2 c, float s);
Color ingColor(int ing);
void drawTicket(const pz::Ticket& k, const pz::Order* o, Rectangle r, bool selected, float size);

// world.cpp: the shop in 3D
extern const g3::Frame MAKE_F, CUT_F;
void worldReset();
void worldUpdate(float dt);          // walking, looking, what you're facing, the camera
void worldDraw();                    // the whole 3D scene
void worldHud();                     // crosshair, prompts, tags over people's heads
void setMode(int mode);
bool project(Vector3 world, Vector2& out);          // onto the 1600x900 canvas; false when behind you
bool mouseOnPlane(float y, Vector3& hit);           // where the mouse points on a level surface
Vector2 binSpot(int ing);                           // a makeline pan, in the makeline's frame
constexpr float BIN = 0.14f;
constexpr float BENCH_X = 0.07f, BOARD_X = -0.45f;  // the pizza on the makeline, and on the cut board

// the stations' screens (drawn over the 3D view)
void registerOverlay();
void makeOverlay();
void cutOverlay();
void portionChart();
void officeOverlay();
