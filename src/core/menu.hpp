// Makeline - the menu: sizes, crusts, every ingredient and its portions. No graphics here.
#pragma once
#include <string>
#include <vector>

namespace pz {

enum Size { SMALL, MEDIUM, LARGE, XLARGE, SIZE_N };
enum Crust { HAND, THIN, PAN, CRUST_N };
enum Level { NONE, LIGHT, NORMAL, EXTRA };
enum Part { WHOLE, LEFT, RIGHT };
enum Kind { SAUCE, CHEESE, VEG, MEAT };
enum Service { COUNTER, CARRYOUT, DELIVERY };

enum Ing {
    MARINARA, ALFREDO, GARLIC_PARM, BBQ,
    MOZZ, MOZZ_BALLS, PROVOLONE, CHEDDAR, PARMESAN, FETA,
    ONION, PEPPER, PINEAPPLE, TOMATO, MUSHROOM, OLIVE,
    SAUSAGE, PEPPERONI, HAM, BACON, BEEF, CHICKEN,
    ING_N
};
constexpr int SAUCE_N = 4;

struct IngInfo {
    const char* name;      // on the bins and the order screen
    const char* say;       // how a customer says it
    const char* say2;      // another way they say it
    Kind kind;
    bool piece;            // placed one at a time (pepperoni), not sprinkled
    float normal[4];       // a normal whole-pizza portion per size: ounces for sauces, bits for the rest
    float perBit;          // what one bit shows as on the readout (ounces; 1 for pieces)
    float rate;            // bits a second while sprinkling
};

const IngInfo& info(int ing);
const char* sizeName(int s);      // "Large"
const char* sizeInches(int s);    // "14\""
const char* crustName(int c);
const char* levelName(int l);
const char* partName(int p);
float levelMult(int l);           // none 0, light 0.5, normal 1, extra 1.5
int slicesFor(int size);          // pie cut: small 6, the rest 8
bool crustComesIn(int crust, int size);   // thin: medium and large; pan: medium only

// what's ordered on one pizza
struct Item { int ing; int part = WHOLE; int level = NORMAL; };
struct Spec {
    int size = LARGE, crust = HAND;
    int sauce = MARINARA, sauceLevel = NORMAL;
    std::vector<Item> items = {{MOZZ, WHOLE, NORMAL}};   // cheese counts as an item; leave it out for no cheese
    const Item* find(int ing) const;
    Item* find(int ing);
    void remove(int ing);
};
Spec mirrored(const Spec& s);          // left and right swapped ("half and half" has no real left)
bool sameSpec(const Spec& a, const Spec& b);   // the same pizza, either way round
float price(const Spec& s);
// the portion wanted of one ingredient on a pizza this size: bits (or ounces of sauce) for the whole pizza
float portion(int ing, int size, int level);
std::string amountText(int ing, float bits);   // "6.8 oz" or "32"

}  // namespace pz
