// Makeline - customers and everything they say.
#include "talk.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>

namespace pz {
namespace {
template <size_t N> const char* pick(Rng& r, const char* const (&a)[N]) { return a[r.below((int)N)]; }
std::string join(const std::vector<std::string>& v) {
    std::string s;
    for (size_t i = 0; i < v.size(); i++) {
        if (i) s += (i + 1 == v.size()) ? " and " : ", ";
        s += v[i];
    }
    return s;
}
std::string upper(std::string s) { for (auto& c : s) c = (char)std::toupper((unsigned char)c); return s; }
std::string lower(std::string s) { for (auto& c : s) c = (char)std::tolower((unsigned char)c); return s; }
std::string sayIng(Rng& r, int ing) { return r.chance(0.6f) ? info(ing).say : info(ing).say2; }
std::string sayItem(Rng& r, const Item& it) {
    std::string n = sayIng(r, it.ing);
    if (it.level == EXTRA) return "extra " + n;
    if (it.level == LIGHT) return (r.chance(0.5f) ? "light " : "just a little ") + n;
    return n;
}
std::string saySize(Rng& r, int size, bool inches) {
    static const char* IN[] = {"ten inch", "twelve inch", "fourteen inch", "sixteen inch"};
    static const char* XL[] = {"extra large", "XL"};
    if (inches) return IN[size];
    if (size == XLARGE) return pick(r, XL);
    return lower(sizeName(size));
}
}  // namespace

Person makePerson(Rng& rng) {
    // first names by background and sex (men, then women), so the town looks like a town
    static const char* const NAMES[6][2][8] = {
        {{"Gary", "Todd", "Rick", "Doug", "Kyle", "Bruce", "Earl", "Dale"}, {"Brenda", "Linda", "Sharon", "Tammy", "Carol", "Karen", "Deb", "Marge"}},
        {{"Luis", "Miguel", "Javier", "Carlos", "Hector", "Diego", "Rafael", "Mateo"}, {"Maria", "Rosa", "Carmen", "Lucia", "Elena", "Sofia", "Isabel", "Ana"}},
        {{"Darnell", "Marcus", "Andre", "Tyrone", "Jamal", "Reggie", "Curtis", "Malik"}, {"Latoya", "Keisha", "Denise", "Monique", "Tanya", "Jasmine", "Ebony", "Renee"}},
        {{"Wei", "Kenji", "Minh", "Jun", "Hiro", "Tuan", "Sung", "Bao"}, {"Mei", "Yuki", "Linh", "Hana", "Ji-woo", "Lan", "Xiu", "Aiko"}},
        {{"Raj", "Arjun", "Vikram", "Sanjay", "Imran", "Anil", "Deepak", "Nikhil"}, {"Priya", "Anita", "Sunita", "Divya", "Meera", "Kavita", "Nisha", "Asha"}},
        {{"Omar", "Yusuf", "Karim", "Tariq", "Samir", "Hassan", "Nabil", "Farid"}, {"Layla", "Fatima", "Amira", "Yasmin", "Noor", "Salma", "Dalia", "Rania"}}};
    static const int SKIN_LO[6] = {0, 1, 3, 0, 2, 1}, SKIN_HI[6] = {1, 3, 5, 2, 4, 3};
    Person p;
    float f = rng.f();
    int bg = f < 0.42f ? 0 : f < 0.6f ? 1 : f < 0.76f ? 2 : f < 0.86f ? 3 : f < 0.94f ? 4 : 5;
    p.sex = rng.below(2);
    p.age = 17 + (int)(std::pow(rng.f(), 1.4f) * 62);
    p.name = std::string(NAMES[bg][p.sex][rng.below(8)]) + " " + (char)('A' + rng.below(26)) + ".";
    p.skin = SKIN_LO[bg] + rng.below(SKIN_HI[bg] - SKIN_LO[bg] + 1);
    p.hair = bg == 0 ? rng.below(4) : bg == 1 ? rng.below(2) : 1;
    if (p.age > 68) p.hair = 5;
    else if (p.age > 52 && rng.chance(0.7f)) p.hair = 4;
    f = rng.f();
    if (p.sex == 0) p.hairStyle = (p.age > 45 && f < 0.3f) ? 0 : f < 0.75f ? 1 : f < 0.9f ? 4 : 3;
    else p.hairStyle = f < 0.45f ? 3 : f < 0.75f ? 2 : f < 0.95f ? 1 : 4;
    p.look = rng.next();
    p.mood = rng.below(4);
    return p;
}

Spec randomPizza(Rng& rng, int day) {
    Spec s;
    float f = rng.f();
    s.size = f < 0.15f ? SMALL : f < 0.45f ? MEDIUM : f < 0.85f ? LARGE : XLARGE;
    f = rng.f();
    s.crust = f < 0.65f ? HAND : f < 0.87f ? THIN : PAN;
    if (s.crust == THIN && !crustComesIn(THIN, s.size)) s.size = rng.chance(0.5f) ? MEDIUM : LARGE;
    if (s.crust == PAN) s.size = MEDIUM;
    if (rng.chance(day <= 1 ? 0.2f : 0.35f)) s.sauce = 1 + rng.below(3);
    if (day >= 2) { f = rng.f(); if (f < 0.12f) s.sauceLevel = LIGHT; else if (f < 0.24f) s.sauceLevel = EXTRA; }
    if (day >= 2) {
        f = rng.f();
        if (f < 0.08f) s.items[0].level = EXTRA;
        else if (f < 0.13f) s.items[0].level = LIGHT;
        else if (f < 0.16f) s.items.clear();
    }
    int n;
    f = rng.f();
    if (day <= 1) n = f < 0.25f ? 0 : f < 0.7f ? 1 : 2;
    else if (day == 2) n = f < 0.15f ? 0 : f < 0.5f ? 1 : f < 0.85f ? 2 : 3;
    else n = f < 0.1f ? 0 : f < 0.35f ? 1 : f < 0.65f ? 2 : f < 0.85f ? 3 : f < 0.95f ? 4 : 5;
    // what people actually order: pepperoni, then sausage, then everything else
    static const int POOL[] = {PEPPERONI, PEPPERONI, PEPPERONI, PEPPERONI, SAUSAGE, SAUSAGE, SAUSAGE, MUSHROOM, MUSHROOM, ONION, ONION, PEPPER, PEPPER,
        BACON, BACON, HAM, HAM, PINEAPPLE, PINEAPPLE, OLIVE, OLIVE, BEEF, BEEF, CHICKEN, CHICKEN, TOMATO, CHEDDAR, PROVOLONE, FETA, PARMESAN, MOZZ_BALLS};
    for (int tries = 0; n > 0 && tries < 60; tries++) {
        int ing = POOL[rng.below((int)(sizeof POOL / sizeof POOL[0]))];
        if (s.find(ing)) continue;
        Item it{ing, WHOLE, NORMAL};
        if (day >= 2) { f = rng.f(); if (f < 0.12f) it.level = EXTRA; else if (f < 0.18f) it.level = LIGHT; }
        s.items.push_back(it);
        n--;
    }
    if (day >= 2 && rng.chance(0.25f)) {   // half and half
        std::vector<Item*> tops;
        for (auto& i : s.items) if (i.ing != MOZZ) tops.push_back(&i);
        if (tops.size() >= 2) { tops[0]->part = LEFT; tops[1]->part = RIGHT; }
        else if (tops.size() == 1 && rng.chance(0.6f)) tops[0]->part = LEFT;
    }
    return s;
}
std::vector<Spec> randomOrder(Rng& rng, int day) {
    float f = rng.f();
    int n = day <= 1 ? (f < 0.85f ? 1 : 2) : day == 2 ? (f < 0.75f ? 1 : 2) : (f < 0.62f ? 1 : f < 0.92f ? 2 : 3);
    std::vector<Spec> v;
    for (int i = 0; i < n; i++) v.push_back(randomPizza(rng, day));
    return v;
}

std::string speakPizza(Rng& r, const Spec& s, bool inches) {
    std::string out = saySize(r, s.size, inches);
    if (s.crust == THIN) out += " thin crust";
    else if (s.crust == PAN) out += " pan pizza";
    else if (r.chance(0.3f)) out += " hand tossed";
    // sauce
    if (s.sauce != MARINARA) {
        std::string n = sayIng(r, s.sauce);
        if (s.sauceLevel == LIGHT) out += ", light on the " + n;
        else if (s.sauceLevel == EXTRA) out += ", extra " + n;
        else out += r.chance(0.5f) ? ", with " + n : ", " + n + " instead of the red";
    } else if (s.sauceLevel == LIGHT) out += r.chance(0.5f) ? ", light sauce" : ", easy on the sauce";
    else if (s.sauceLevel == EXTRA) out += ", extra sauce";
    // cheese
    const Item* mz = s.find(MOZZ);
    if (!mz) out += ", no cheese";
    else if (mz->level == LIGHT) out += r.chance(0.5f) ? ", light cheese" : ", easy on the cheese";
    else if (mz->level == EXTRA) out += ", extra cheese";
    // toppings
    std::vector<std::string> W, L, R;
    for (auto& i : s.items) {
        if (i.ing == MOZZ) continue;
        (i.part == WHOLE ? W : i.part == LEFT ? L : R).push_back(sayItem(r, i));
    }
    if (W.empty() && L.empty() && R.empty()) {
        if (mz && mz->level == NORMAL && s.sauce == MARINARA && s.sauceLevel == NORMAL) out += r.chance(0.5f) ? ", just cheese" : ", plain cheese";
        else out += ", nothing else on it";
    } else if (!L.empty() && !R.empty()) {
        out += ", half " + join(L) + " and half " + join(R);
        if (!W.empty()) out += ", and " + join(W) + " on the whole thing";
    } else if (!L.empty() || !R.empty()) {
        auto& H = L.empty() ? R : L;
        if (!W.empty()) out += ", with " + join(W) + ", and " + join(H) + " on just half";
        else out += ", with " + join(H) + " on only half of it";
    } else out += ", with " + join(W);
    return out;
}

std::string streetAddress(Rng& r) {
    static const char* const ST[] = {"Maple", "Dogwood", "Cedar Ridge", "Old Mill", "Pinecrest", "Larkspur", "County Line", "Sawgrass", "Bellview", "Tanager"};
    static const char* const KIND[] = {"Street", "Lane", "Road", "Court", "Drive"};
    return std::to_string(100 + r.below(8900)) + " " + pick(r, ST) + " " + pick(r, KIND);
}

std::vector<std::string> orderScript(Rng& r, const Person& who, const std::vector<Spec>& want, int service, bool phone, int day) {
    std::vector<std::string> L;
    static const char* const HELLO_PHONE_DEL[] = {"Yeah. I need a delivery.", "Delivery. And don't take an hour this time.", "Hi. Delivery. You got a pen?", "I want it delivered. Today, preferably."};
    static const char* const HELLO_PHONE_CARRY[] = {"Yeah, order for pickup.", "Carryout. I'm already in the car.", "Pickup. I'll be there in like five minutes so hurry.", "I'm picking it up. Don't make me wait."};
    static const char* const HELLO_COUNTER[] = {"Finally.", "Yeah, hi.", "About time. Okay.", "You open or what? Okay.", "Hey. Quick order."};
    static const char* const OPEN[] = {"Gimme a ", "Lemme get a ", "I need a ", "I want a ", "Give me a "};
    static const char* const NEXT[] = {"And then a ", "Also a ", "And another one, a ", "Plus a "};
    static const char* const DONE[] = {"That's it.", "That's all.", "That's everything.", "And that's it."};
    static const char* const SNARK[][3] = {
        {"And hurry it up, I got places to be.", "How long? No, don't tell me. Just hurry.", "I don't have all day."},
        {"...you get all that?", "Mhm. Yeah. That.", "Whatever, just make it."},
        {"I think that's right. Yeah. Probably.", "Don't mess it up, I'm not saying it again.", "Was that right? You wrote it down, right?"},
        {"And it better be right this time.", "Last time it was wrong. I WILL be checking.", "I know the owner, so. Get it right."}};
    if (phone) L.push_back(service == DELIVERY ? pick(r, HELLO_PHONE_DEL) : pick(r, HELLO_PHONE_CARRY));
    else L.push_back(pick(r, HELLO_COUNTER));

    int twist = -1, twistKind = 0;   // one change of mind an order, from the second day on
    if (day >= 2 && r.chance(who.mood == 2 ? 0.7f : 0.3f)) { twist = r.below((int)want.size()); twistKind = r.below(3); }
    int garble = (phone && (day >= 3 || who.mood == 1) && r.chance(who.mood == 1 ? 0.5f : 0.2f)) ? r.below((int)want.size()) : -1;

    for (size_t k = 0; k < want.size(); k++) {
        const Spec& truth = want[k];
        Spec said = truth;
        std::string fix;
        if ((int)k == twist) {
            std::vector<int> tops;
            for (auto& i : truth.items) if (i.ing != MOZZ && i.part == WHOLE) tops.push_back(i.ing);
            int decoy = -1;
            for (int t = 0; t < 30 && decoy < 0; t++) { int d = ONION + r.below(ING_N - ONION); if (!truth.find(d)) decoy = d; }
            if (twistKind == 0 && !tops.empty() && decoy >= 0) {   // names the wrong topping, then swaps it
                int x = tops[r.below((int)tops.size())];
                Item* it = said.find(x);
                Item real = *it;
                it->ing = decoy; it->level = NORMAL;
                fix = std::string(r.chance(0.5f) ? "Actually, no " : "Wait. Not ") + info(decoy).say + ". Make that " + sayItem(r, real) + ".";
            } else if (twistKind == 1 && decoy >= 0) {   // adds one, then takes it back
                said.items.push_back({decoy, WHOLE, NORMAL});
                fix = std::string(r.chance(0.5f) ? "You know what, forget the " : "No, scratch the ") + info(decoy).say + ".";
            } else {   // the wrong size first
                int other = truth.size;
                for (int t = 0; t < 20 && (other == truth.size || !crustComesIn(truth.crust, other)); t++) other = r.below(SIZE_N);
                if (other != truth.size && crustComesIn(truth.crust, other)) {
                    said.size = other;
                    fix = std::string(r.chance(0.5f) ? "Actually make that a " : "No wait. A ") + saySize(r, truth.size, false) + ". Same stuff.";
                }
            }
        }
        std::string line = (k == 0 ? pick(r, OPEN) : pick(r, NEXT)) + speakPizza(r, said, r.chance(0.15f)) + ".";
        if ((int)k == garble) {   // the phone cuts out over one topping
            for (auto& i : said.items) {
                if (i.ing == MOZZ) continue;
                size_t at = line.find(info(i.ing).say);
                std::string w = info(i.ing).say;
                if (at == std::string::npos) { at = line.find(info(i.ing).say2); w = info(i.ing).say2; }
                if (at != std::string::npos) { line.replace(at, w.size(), "*kssshh*"); break; }
            }
        }
        // long orders come out in two breaths
        size_t cut = line.find(", ", line.size() / 2);
        if (line.size() > 70 && cut != std::string::npos) {
            L.push_back(line.substr(0, cut + 1));
            std::string rest = line.substr(cut + 2);
            rest[0] = (char)std::toupper((unsigned char)rest[0]);
            L.push_back(rest);
        } else L.push_back(line);
        if (!fix.empty()) L.push_back(fix);
    }
    std::string end = pick(r, DONE);
    if (phone && service == DELIVERY) end += " It's " + streetAddress(r) + ".";
    L.push_back(end);
    L.push_back(SNARK[who.mood & 3][r.below(3)]);
    return L;
}

std::string recap(Rng& r, const std::vector<Spec>& want, int service, bool phone) {
    static const char* const PRE[] = {"Are you serious? Fine. ", "Oh my GOD. Listen this time. ", "I JUST told you. ", "Hello?? Pay attention. ", "Unbelievable. One more time: "};
    static const char* const POST[] = {" Got it?", " Was that slow enough for you?", " Write it DOWN.", " Okay?!"};
    std::string s = pick(r, PRE);
    if (phone) s += service == DELIVERY ? "DELIVERY. " : "PICKUP. ";
    for (size_t k = 0; k < want.size(); k++) {
        if (want.size() > 1) s += (k == 0 ? "First one: " : k == 1 ? "Second one: " : "Third one: ");
        std::string p = speakPizza(r, want[k], false);
        p[0] = (char)std::toupper((unsigned char)p[0]);
        s += p + ". ";
    }
    s.pop_back();
    return s + pick(r, POST);
}

std::string readBack(Rng& r, const std::vector<Spec>& want, const std::vector<Spec>& entered, int wantService, int enteredService, bool phone, bool& ok) {
    ok = false;
    static const char* const NO[] = {"No! ", "NO. ", "What? No. ", "Are you even listening? ", "Wrong. "};
    std::string no = pick(r, NO);
    if (phone && wantService != enteredService) return no + (wantService == DELIVERY ? "I said DELIVERY. I'm not driving down there." : "I said PICKUP. I'm coming to get it.");
    if (entered.size() < want.size()) return no + "I ordered " + std::to_string(want.size()) + " pizzas. You're missing one.";
    if (entered.size() > want.size()) return no + "I only ordered " + std::to_string(want.size()) + (want.size() == 1 ? " pizza." : " pizzas.") + " I'm not paying for extra.";
    for (size_t k = 0; k < want.size(); k++) {
        const Spec& w = want[k];
        // their half-and-half has no left: take whichever way round matches best
        Spec e = entered[k];
        auto diff = [&](const Spec& x) { int d = 0; for (auto& i : w.items) { const Item* j = x.find(i.ing); if (!j || j->part != i.part) d++; } return d; };
        if (diff(mirrored(e)) < diff(e)) e = mirrored(e);
        std::string which = want.size() > 1 ? (k == 0 ? " on the first one" : k == 1 ? " on the second one" : " on the third one") : "";
        if (e.size != w.size) return no + "I said " + upper(sizeName(w.size)) + which + ".";
        if (e.crust != w.crust) return no + "I said " + upper(crustName(w.crust)) + (w.crust == HAND ? "" : " crust") + which + ".";
        if (e.sauce != w.sauce) return no + "It's " + upper(info(w.sauce).say) + which + ".";
        if (e.sauceLevel != w.sauceLevel) return no + "I said " + upper(levelName(w.sauceLevel)) + " sauce" + which + ".";
        for (auto& i : w.items) {
            const Item* j = e.find(i.ing);
            std::string n = info(i.ing).say;
            if (!j) return no + "Where's the " + n + which + "?";
            if (j->level != i.level) return no + "I said " + upper(levelName(i.level)) + " " + n + which + ".";
            if (j->part != i.part) return no + (i.part == WHOLE ? "The " + n + " goes on the WHOLE pizza" : "The " + n + " is only on HALF") + which + ".";
        }
        for (auto& j : e.items) if (!w.find(j.ing)) return no + "I never said " + (j.ing == MOZZ ? std::string("cheese. NO cheese") : std::string(info(j.ing).say)) + which + "!";
    }
    ok = true;
    static const char* const YES[] = {"Yes. That's what I said.", "Yeah. Finally.", "That's right. Was that so hard?", "Mhm. Go make it.", "Yes! Can we move this along?"};
    return pick(r, YES);
}

std::string waitingBark(Rng& r, float patience) {
    static const char* const MILD[] = {"How long's this gonna take?", "*checks phone*", "*sighs loudly*", "It's just pizza...", "Is anyone working here?", "*taps foot*"};
    static const char* const MAD[] = {"I've been standing here FOREVER.", "This is ridiculous.", "Hello?! My pizza?", "I could've made it myself by now.", "Where's your manager?",
        "I'm never coming back here.", "Do you people even care?", "Un-be-lievable."};
    return patience < 0.4f ? pick(r, MAD) : pick(r, MILD);
}

std::string issueLine(const Issue& is) {
    std::string n = is.ing >= 0 ? info(is.ing).name : "";
    switch (is.code) {
        case WRONG_SIZE: return "Wrong size";
        case WRONG_CRUST: return "Wrong crust";
        case WRONG_SAUCE: return "Wrong sauce";
        case NO_SAUCE: return "No sauce";
        case SAUCE_LOW: return "Not enough sauce";
        case SAUCE_HIGH: return "Too much sauce";
        case SAUCE_PATCHY: return "Sauce not spread";
        case MISSING: return "Missing " + n;
        case TOO_LITTLE: return "Too little " + n;
        case TOO_MUCH: return "Too much " + n;
        case UNEVEN: return n + " not spread evenly";
        case UNWANTED: return n + " they didn't order";
        case WRONG_HALF: return n + " over the wrong half";
        case UNCUT: return "Not cut";
        case BAD_CUT: return "Sloppy cut";
        case WRONG_CUT: return "Wrong kind of cut";
        case LUKEWARM: return "Lukewarm";
        case COLD: return "Cold";
        case LATE: return "Late (over 20 minutes)";
        case VERY_LATE: return "Very late (over 30 minutes)";
        case EXTREMELY_LATE: return "Extremely late (over 45 minutes)";
        case WRONG_SERVICE: return "Pickup/delivery mixed up";
        case MISSING_PIZZA: return "A pizza was missing";
        case EXTRA_PIZZA: return "Charged for an extra pizza";
        case NEVER_SERVED: return "Never served";
        case NO_ANSWER: return "Nobody answered the phone";
        case RUDE_SERVICE: return "Gave up waiting in line";
    }
    return "";
}

std::string reviewText(Rng& r, const Score& score, int stars) {
    if (score.issues.empty() || stars >= 5) {
        static const char* const GOOD[] = {"It was fine. Pizza was actually right for once.", "Fast, hot, and what I ordered. Shocked.", "No complaints. Which is rare for me.",
            "Good pizza. Kid at the counter looked tired though.", "They got it right. Five stars, I guess.", "Honestly the best one in town. Don't tell them I said that.",
            "Hot and correct. That's all I ask.", "Came out perfect. Somebody back there knows what they're doing."};
        return pick(r, GOOD);
    }
    // the two worst things, in their words
    std::vector<Issue> is = score.issues;
    std::stable_sort(is.begin(), is.end(), [](const Issue& a, const Issue& b) { return a.pts > b.pts; });
    std::string out;
    for (size_t k = 0; k < is.size() && k < 2; k++) {
        const Issue& i = is[k];
        std::string n = i.ing >= 0 ? lower(info(i.ing).name) : "";
        std::string s;
        switch (i.code) {
            case WRONG_SIZE: s = r.chance(0.5f) ? "Wrong SIZE. How do you get the size wrong?" : "I paid for one size and got another."; break;
            case WRONG_CRUST: s = "Wrong crust. I know what I ordered."; break;
            case WRONG_SAUCE: s = r.chance(0.5f) ? "Wrong sauce on the whole thing." : "That is NOT the sauce I asked for."; break;
            case NO_SAUCE: s = "There was no sauce. None. It was bread with stuff on it."; break;
            case SAUCE_LOW: s = "Dry as cardboard, barely any sauce."; break;
            case SAUCE_HIGH: s = "Swimming in sauce. It was soup."; break;
            case SAUCE_PATCHY: s = "Sauce all in one spot, the rest bare."; break;
            case MISSING: s = r.chance(0.5f) ? "They forgot the " + n + ". The one thing I wanted." : "No " + n + ". I PAID for " + n + "."; break;
            case TOO_LITTLE: s = r.chance(0.5f) ? "I could count the " + n + " on one hand." : "Stingy with the " + n + "."; break;
            case TOO_MUCH: s = r.chance(0.5f) ? "Way too much " + n + ", couldn't taste anything else." : "Buried in " + n + "."; break;
            case UNEVEN: s = "All the " + n + " was piled in one spot."; break;
            case UNWANTED: s = i.ing == PINEAPPLE ? "There was PINEAPPLE on it. I did not order pineapple. Who does that?" : (r.chance(0.5f) ? "It had " + n + " on it. I never asked for " + n + "." : "Why is there " + n + " on my pizza? I'm allergic. Probably."); break;
            case WRONG_HALF: s = "I said HALF. The " + n + " was all over the other side."; break;
            case UNCUT: s = "They didn't even cut it. I had to tear it apart like an animal."; break;
            case BAD_CUT: s = "Looked like it was cut with a shovel."; break;
            case WRONG_CUT: s = "Cut it wrong. Slices were all weird."; break;
            case LUKEWARM: s = "Lukewarm by the time I got it."; break;
            case COLD: s = "Stone cold. Sat around forever."; break;
            case LATE: s = r.chance(0.5f) ? "Took way too long." : "Slow. Very slow."; break;
            case VERY_LATE: s = r.chance(0.5f) ? "Over half an hour for a pizza. Half an HOUR." : "I aged a year waiting for this pizza."; break;
            case EXTREMELY_LATE: s = "Forty-five minutes. I could have driven to Italy."; break;
            case WRONG_SERVICE: s = "I said pickup and they sent a driver. Or the other way. Either way I had no pizza."; break;
            case MISSING_PIZZA: s = "They shorted me a whole pizza."; break;
            case EXTRA_PIZZA: s = "Charged me for a pizza I didn't order."; break;
            case NEVER_SERVED: s = "Paid, waited, and never got my food. NEVER."; break;
            case NO_ANSWER: s = "Phone rang and rang. Nobody picked up. Ordered somewhere else."; break;
            case RUDE_SERVICE: s = "Stood at the counter forever and nobody even looked at me. Walked out."; break;
        }
        if (!out.empty()) out += " ";
        out += s;
    }
    static const char* const END1[] = {" Never again.", " Do better.", " Zero stars if I could.", " Calling corporate.", " Unreal."};
    static const char* const END3[] = {" Meh.", " Not great.", " Expected better.", ""};
    if (stars <= 2) out += pick(r, END1);
    else if (stars == 3) out += pick(r, END3);
    else out += " Otherwise okay.";
    return out;
}

}  // namespace pz
