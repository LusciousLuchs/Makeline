// Makeline - the game's state and its screens.
#pragma once
#include "raylib.h"
#include "store.hpp"
#include "ui.hpp"
#include <string>
#include <vector>

enum Station { ST_FRONT, ST_MAKE, ST_OVEN };
enum Sfx { SFX_CLICK, SFX_RING, SFX_DING, SFX_TALK, SFX_BAD, SFX_GOOD, SFX_OVEN, SFX_CUT, SFX_PLOP, SFX_DOOR, SFX_N };

struct Toast { std::string text; int stars; float age; };   // stars 0: a plain note

struct Game {
    pz::Store S;
    int station = ST_FRONT;
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
};
extern Game G;

void sfx(int which);
void audioInit();
void audioUpdate(bool ringing);

// art.cpp
void drawPizza(const pz::Pizza& p, Vector2 c, float r, float bakeShow = -1);
float pizzaRadius(int size, float base);   // on-screen radius for a size, `base` = a large
void drawIngredientIcon(int ing, Vector2 c, float s);
void drawCustomer(const pz::Person& who, float x, float baseY, float scale, float anger, float bob);
Color ingColor(int ing);
void drawTicket(const pz::Ticket& k, const pz::Order* o, Rectangle r, bool selected, float size);

// the stations
void frontStation();
void makeStation();
void ovenStation();
void portionChart();
