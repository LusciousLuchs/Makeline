// Makeline - a pizza as it was actually made, and how it's judged against what the customer wanted.
// Positions are in pizza units: (0,0) is the middle, 1 is the outer edge of the crust, +x is the right half.
#pragma once
#include "menu.hpp"
#include <cstdint>
#include <vector>

namespace pz {

struct Rng {
    uint64_t s;
    explicit Rng(uint64_t seed = 1) : s(seed * 0x9E3779B97F4A7C15ull + 0x1234567ull) {}
    uint32_t next() { s ^= s << 13; s ^= s >> 7; s ^= s << 17; return (uint32_t)(s >> 16); }
    float f() { return (next() & 0xFFFFFF) / 16777216.f; }                 // 0..1
    float range(float a, float b) { return a + (b - a) * f(); }
    int below(int n) { return n <= 0 ? 0 : (int)(next() % (uint32_t)n); }
    bool chance(float p) { return f() < p; }
};

constexpr float TOP_EDGE = 0.90f;     // toppings stay inside this (the rest is crust)
constexpr float SAUCE_EDGE = 0.87f;
constexpr float STAMP_R = 0.135f;     // one dab of the ladle

struct Bit { uint8_t ing, var; float x, y, rot; };
struct Stamp { uint8_t sauce; float x, y; };
struct Cut { float ang, off; };       // a cut right across: its direction (0..pi) and how far it passes from the middle

struct Pizza {
    int id = 0, ticket = -1;
    int size = LARGE, crust = HAND;
    float sauceOz[SAUCE_N] = {0, 0, 0, 0};
    std::vector<Stamp> stamps;
    std::vector<Bit> bits;
    std::vector<Cut> cuts;
    float baked = 0;      // 0 raw, 1 done
    float cool = 0;       // how long it has sat since the oven (seconds; slower in a box on the heat rack)
    bool boxed = false;

    // ladle some sauce at a spot. False if it missed the pizza.
    bool addSauce(int sauce, float x, float y, float oz);
    // drop one bit of something. False if it fell off the edge.
    bool addBit(int ing, float x, float y, Rng& rng);
    int removeNear(float x, float y, float r);            // pick toppings back off; how many came off
    void addCut(float x1, float y1, float x2, float y2);  // a stroke of the cutter, carried right across
    float amount(int ing) const;                          // ounces of a sauce, or bits of anything else
    float amountSide(int ing, bool right) const;
    float totalSauce() const;
    int mainSauce() const;                                // -1 when there's none
    float sauceCoverage() const;                          // 0..1 of the sauced area
};

enum IssueCode {
    WRONG_SIZE, WRONG_CRUST, WRONG_SAUCE, NO_SAUCE, SAUCE_LOW, SAUCE_HIGH, SAUCE_PATCHY,
    MISSING, TOO_LITTLE, TOO_MUCH, UNEVEN, UNWANTED, WRONG_HALF,
    UNCUT, BAD_CUT, WRONG_CUT, LUKEWARM, COLD,
    LATE, VERY_LATE, EXTREMELY_LATE, WRONG_SERVICE, MISSING_PIZZA, EXTRA_PIZZA, NEVER_SERVED, NO_ANSWER, RUDE_SERVICE,
    ISSUE_N
};
struct Issue { int code; int ing; float pts; };
struct Score {
    std::vector<Issue> issues;
    float penalty = 0;
    void add(int code, float pts, int ing = -1) { issues.push_back({code, ing, pts}); penalty += pts; }
};

Score scorePizza(const Spec& want, const Pizza& made);
// 0 no cuts, 1 a proper pie cut, 2 a proper square cut, 3 a mess
int cutStyle(const Pizza& p);
// a whole order: the pizzas made against the pizzas wanted (best pairing), plus the service
struct OrderScore { Score score; int stars = 5; };
// `minutes`: the order's timer when the customer got it. Over 20 is late, over 30 very late, over 45 extreme.
constexpr float LATE_MIN = 20, VERY_LATE_MIN = 30, EXTREME_MIN = 45;
OrderScore scoreOrder(const std::vector<Spec>& want, const std::vector<const Pizza*>& made, int wantService, int gotService, float minutes);
int starsFor(float penalty);

// a pizza made exactly right (the tests' yardstick, and the demo's)
Pizza perfectPizza(const Spec& s, Rng& rng);

}  // namespace pz
