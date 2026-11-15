// Makeline - the menu table.
#include "menu.hpp"
#include <algorithm>
#include <cstdio>

namespace pz {
namespace {
const IngInfo TABLE[ING_N] = {
    // name              say                 say2                    kind    piece  S     M     L     XL    perBit rate
    {"Marinara",        "regular sauce",    "red sauce",            SAUCE,  false, {2.5f, 3.5f, 5.0f, 6.5f}, 1, 2.2f},
    {"Alfredo",         "alfredo sauce",    "alfredo",              SAUCE,  false, {2.5f, 3.5f, 5.0f, 6.5f}, 1, 2.2f},
    {"Garlic Parm",     "garlic parm sauce", "that garlic parmesan sauce", SAUCE, false, {2.5f, 3.5f, 5.0f, 6.5f}, 1, 2.2f},
    {"BBQ",             "barbecue sauce",   "BBQ sauce",            SAUCE,  false, {2.5f, 3.5f, 5.0f, 6.5f}, 1, 2.2f},
    {"Mozzarella",      "cheese",           "mozzarella",           CHEESE, false, {35, 50, 70, 90}, 0.1f, 32},
    {"Mozz Balls",      "fresh mozzarella", "those mozzarella balls", CHEESE, true, {6, 8, 11, 14}, 1, 0},
    {"Provolone",       "provolone",        "provolone cheese",     CHEESE, false, {15, 20, 28, 36}, 0.1f, 16},
    {"Cheddar",         "cheddar",          "cheddar cheese",       CHEESE, false, {15, 20, 28, 36}, 0.1f, 16},
    {"Parmesan",        "parmesan",         "parm",                 CHEESE, false, {10, 14, 20, 26}, 0.05f, 14},
    {"Feta",            "feta",             "feta cheese",          CHEESE, false, {12, 16, 22, 28}, 0.1f, 14},
    {"Onions",          "onions",           "onion",                VEG,    false, {14, 20, 28, 36}, 0.1f, 15},
    {"Green Peppers",   "green peppers",    "bell peppers",         VEG,    false, {14, 20, 28, 36}, 0.1f, 15},
    {"Pineapple",       "pineapple",        "pineapple",            VEG,    false, {12, 18, 24, 32}, 0.15f, 14},
    {"Tomatoes",        "tomatoes",         "tomato",               VEG,    false, {12, 18, 24, 32}, 0.15f, 14},
    {"Mushrooms",       "mushrooms",        "mushroom",             VEG,    false, {12, 18, 24, 32}, 0.1f, 14},
    {"Black Olives",    "black olives",     "olives",               VEG,    false, {14, 20, 28, 36}, 0.1f, 15},
    {"Italian Sausage", "sausage",          "Italian sausage",      MEAT,   false, {12, 18, 24, 32}, 0.15f, 14},
    {"Pepperoni",       "pepperoni",        "pepperoni",            MEAT,   true,  {18, 24, 32, 40}, 1, 0},
    {"Canadian Bacon",  "Canadian bacon",   "Canadian bacon",       MEAT,   true,  {10, 14, 18, 24}, 1, 0},
    {"Bacon",           "bacon",            "bacon",                MEAT,   false, {14, 20, 28, 36}, 0.1f, 15},
    {"Beef",            "beef",             "hamburger",            MEAT,   false, {12, 18, 24, 32}, 0.15f, 14},
    {"Chicken",         "chicken",          "chicken",              MEAT,   false, {10, 14, 20, 26}, 0.2f, 13},
};
}  // namespace

const IngInfo& info(int ing) { return TABLE[std::clamp(ing, 0, ING_N - 1)]; }
const char* sizeName(int s) { static const char* N[] = {"Small", "Medium", "Large", "X-Large"}; return N[std::clamp(s, 0, 3)]; }
const char* sizeInches(int s) { static const char* N[] = {"10\"", "12\"", "14\"", "16\""}; return N[std::clamp(s, 0, 3)]; }
const char* crustName(int c) { static const char* N[] = {"Hand Tossed", "Thin", "Pan"}; return N[std::clamp(c, 0, 2)]; }
const char* levelName(int l) { static const char* N[] = {"None", "Light", "Normal", "Extra"}; return N[std::clamp(l, 0, 3)]; }
const char* partName(int p) { static const char* N[] = {"Whole", "Left", "Right"}; return N[std::clamp(p, 0, 2)]; }
float levelMult(int l) { static const float M[] = {0, 0.5f, 1, 1.5f}; return M[std::clamp(l, 0, 3)]; }
int slicesFor(int size) { return size == SMALL ? 6 : 8; }
bool crustComesIn(int crust, int size) {
    if (crust == THIN) return size == MEDIUM || size == LARGE;
    if (crust == PAN) return size <= LARGE;
    return true;
}

const Item* Spec::find(int ing) const { for (auto& i : items) if (i.ing == ing) return &i; return nullptr; }
Item* Spec::find(int ing) { for (auto& i : items) if (i.ing == ing) return &i; return nullptr; }
void Spec::remove(int ing) { items.erase(std::remove_if(items.begin(), items.end(), [&](const Item& i) { return i.ing == ing; }), items.end()); }

Spec mirrored(const Spec& s) {
    Spec m = s;
    for (auto& i : m.items) i.part = i.part == LEFT ? RIGHT : i.part == RIGHT ? LEFT : WHOLE;
    return m;
}
namespace {
bool sameOneWay(const Spec& a, const Spec& b) {
    if (a.size != b.size || a.crust != b.crust || a.sauce != b.sauce || a.sauceLevel != b.sauceLevel) return false;
    if (a.items.size() != b.items.size()) return false;
    for (auto& i : a.items) {
        const Item* j = b.find(i.ing);
        if (!j || j->part != i.part || j->level != i.level) return false;
    }
    return true;
}
}  // namespace
bool sameSpec(const Spec& a, const Spec& b) { return sameOneWay(a, b) || sameOneWay(a, mirrored(b)); }

float price(const Spec& s) {
    static const float BASE[] = {7.99f, 9.99f, 11.99f, 13.99f}, TOP[] = {1.0f, 1.25f, 1.5f, 1.75f};
    float p = BASE[std::clamp(s.size, 0, 3)];
    for (auto& i : s.items) {
        if (i.ing == MOZZ) { if (i.level == EXTRA) p += TOP[s.size]; continue; }
        p += TOP[s.size] * (i.part == WHOLE ? 1.f : 0.5f) * (i.level == EXTRA ? 1.5f : 1.f);
    }
    return p;
}
float portion(int ing, int size, int level) { return info(ing).normal[std::clamp(size, 0, 3)] * levelMult(level); }
std::string amountText(int ing, float bits) {
    char b[32];
    const IngInfo& in = info(ing);
    if (in.kind == SAUCE) snprintf(b, sizeof b, "%.1f oz", bits);
    else if (in.piece) snprintf(b, sizeof b, "%d", (int)(bits + 0.5f));
    else snprintf(b, sizeof b, "%.1f oz", bits * in.perBit);
    return b;
}

}  // namespace pz
