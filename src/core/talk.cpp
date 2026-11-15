// Makeline - customers and everything they say.
#include "talk.hpp"
#include "speech.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>

namespace pz {
bool strongLanguage = true;
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

std::string heat(Rng& r, const std::string& line, float anger) {
    if (!strongLanguage || anger < 0.35f || !r.chance(0.35f + 0.6f * std::min(1.f, anger))) return line;
    static const char* const MILD_BEFORE[] = {"Damn it. ", "Jesus Christ. ", "Oh, hell no. ", "Are you kidding me? ", "Seriously, what the hell? "};
    static const char* const MILD_AFTER[] = {" This is crap.", " Unbelievable.", " Damn.", " What a joke.", " Are you serious right now?"};
    static const char* const HARD_BEFORE[] = {"Oh, for fuck's sake. ", "Are you fucking kidding me? ", "Jesus fucking Christ. ", "What the fuck? ", "Goddamn it. ", "You've gotta be shitting me. "};
    static const char* const HARD_AFTER[] = {" This is bullshit.", " Fucking unbelievable.", " What the hell is wrong with you people?", " I'm so damn tired of this shit.", " Are you fucking stupid?", " Un-fucking-believable.", " Pathetic."};
    bool hard = anger > 0.65f && r.chance(0.7f);
    bool before = r.chance(0.5f);
    std::string out = line;
    if (before) out = std::string(hard ? pick(r, HARD_BEFORE) : pick(r, MILD_BEFORE)) + line;
    else out += hard ? pick(r, HARD_AFTER) : pick(r, MILD_AFTER);
    return out;
}

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
    if (s.crust == PAN && s.size == XLARGE) s.size = LARGE;
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
    std::string sz = saySize(r, s.size, inches), cr, sauceC, cheeseC, topsC;
    if (s.crust == THIN) cr = r.chance(0.5f) ? "thin crust" : "thin";
    else if (s.crust == PAN) cr = r.chance(0.5f) ? "pan pizza" : "pan";
    else if (r.chance(0.3f)) cr = "hand tossed";
    if (s.sauce != MARINARA) {
        std::string n = sayIng(r, s.sauce);
        if (s.sauceLevel == LIGHT) sauceC = "light on the " + n;
        else if (s.sauceLevel == EXTRA) sauceC = "extra " + n;
        else sauceC = r.chance(0.5f) ? "with " + n : n + " instead of the red";
    } else if (s.sauceLevel == LIGHT) sauceC = r.chance(0.5f) ? "light sauce" : "easy on the sauce";
    else if (s.sauceLevel == EXTRA) sauceC = r.chance(0.5f) ? "extra sauce" : "lots of sauce";
    const Item* mz = s.find(MOZZ);
    if (!mz) cheeseC = r.chance(0.5f) ? "no cheese" : "hold the cheese";
    else if (mz->level == LIGHT) cheeseC = r.chance(0.5f) ? "light cheese" : "easy on the cheese";
    else if (mz->level == EXTRA) cheeseC = r.chance(0.5f) ? "extra cheese" : "double up on the cheese";
    std::vector<std::string> W, L, R;
    for (auto& i : s.items) {
        if (i.ing == MOZZ) continue;
        (i.part == WHOLE ? W : i.part == LEFT ? L : R).push_back(sayItem(r, i));
    }
    if (W.empty() && L.empty() && R.empty()) {
        if (mz && mz->level == NORMAL && s.sauce == MARINARA && s.sauceLevel == NORMAL) topsC = r.chance(0.5f) ? "just cheese" : "plain cheese";
        else topsC = "nothing else on it";
    } else if (!L.empty() && !R.empty()) {
        topsC = "half " + join(L) + " and half " + join(R);
        if (!W.empty()) topsC += ", and " + join(W) + " on the whole thing";
    } else if (!L.empty() || !R.empty()) {
        auto& H = L.empty() ? R : L;
        if (!W.empty()) topsC = "with " + join(W) + ", and " + join(H) + " on just half";
        else topsC = "with " + join(H) + " on only half of it";
    } else topsC = r.chance(0.5f) ? "with " + join(W) : join(W) + " on it";
    auto list = [](std::vector<std::string> v) { std::string o; for (auto& x : v) if (!x.empty()) o += (o.empty() ? "" : ", ") + x; return o; };
    std::string head = sz + (cr.empty() ? "" : " " + cr);
    int pattern = r.below(3);
    bool lead = topsC.compare(0, 4, "with") == 0 || topsC.compare(0, 4, "half") == 0 || topsC.compare(0, 4, "just") == 0 || topsC.compare(0, 5, "plain") == 0 || topsC.compare(0, 7, "nothing") == 0;
    std::string withT = lead ? topsC : "with " + topsC;
    if (pattern == 0) return list({head, sauceC, cheeseC, topsC});
    if (pattern == 1) return sz + " " + withT + (cr.empty() ? "" : ", " + cr) + (sauceC.empty() && cheeseC.empty() ? "" : ", " + list({sauceC, cheeseC}));
    return list({sz, cr, topsC, sauceC, cheeseC});
}

std::string streetAddress(Rng& r) {
    static const char* const ST[] = {"Maple", "Dogwood", "Cedar Ridge", "Old Mill", "Pinecrest", "Larkspur", "County Line", "Sawgrass", "Bellview", "Tanager"};
    static const char* const KIND[] = {"Street", "Lane", "Road", "Court", "Drive"};
    return std::to_string(100 + r.below(8900)) + " " + pick(r, ST) + " " + pick(r, KIND);
}

std::vector<std::string> orderScript(Rng& r, const Person& who, const std::vector<Spec>& want, int service, bool phone, int day, int hour) {
    std::vector<std::string> L;
    Ctx c;
    c.who = &who; c.phone = phone; c.service = service; c.hour = hour;
    c.anger = who.mood == 3 ? 0.55f : who.mood == 0 ? 0.3f : 0.1f;
    c.v["svc"] = service == DELIVERY ? "delivery" : "pickup";
    bool chatty = (who.look % 3) == 0;
    L.push_back(heat(r, gen(r, phone ? "greet_phone" : "greet_counter", c), c.anger));
    if (r.chance(chatty ? 0.55f : 0.12f)) { std::string a = gen(r, "aside", c); if (!a.empty()) L.push_back(a); }

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
                static const char* const F0[] = {"Actually, no %s. Make that %s.", "Wait. Not %s. %s.", "Sorry, scratch the %s, I meant %s.", "Hold on, I said %s? No, %s.", "No no, not the %s. The %s."};
                char b[200]; std::string a = info(decoy).say, rl = sayItem(r, real);
                snprintf(b, sizeof b, pick(r, F0), a.c_str(), rl.c_str());
                fix = b;
            } else if (twistKind == 1 && decoy >= 0) {   // adds one, then takes it back
                said.items.push_back({decoy, WHOLE, NORMAL});
                static const char* const F1[] = {"You know what, forget the %s.", "No, scratch the %s.", "Actually, leave off the %s.", "Wait, no %s. Never mind that.", "Take the %s off, I changed my mind."};
                char b[160]; snprintf(b, sizeof b, pick(r, F1), info(decoy).say);
                fix = b;
            } else {   // the wrong size first
                int other = truth.size;
                for (int t = 0; t < 20 && (other == truth.size || !crustComesIn(truth.crust, other)); t++) other = r.below(SIZE_N);
                if (other != truth.size && crustComesIn(truth.crust, other)) {
                    said.size = other;
                    static const char* const F2[] = {"Actually make that %s %s. Same stuff.", "No wait. %s %s.", "Sorry, I want %s %s, not that one.", "Hang on, make it %s %s.", "Let's bump that to %s %s instead."};
                    std::string sz = saySize(r, truth.size, false);
                    char b[160]; snprintf(b, sizeof b, pick(r, F2), article(sz).c_str(), sz.c_str());
                    fix = b;
                }
            }
        }
        c.v["nth"] = std::to_string(k + 1);
        std::string verb = gen(r, k == 0 ? "ask_first" : "ask_next", c);
        std::string desc = speakPizza(r, said, r.chance(0.15f));
        std::string line = verb + genRaw(r, "tic_", c) + " " + article(desc) + " " + desc + ".";
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
            L.push_back(line.substr(cut + 2));   // (carries on mid-sentence, so it starts in lower case)
        } else L.push_back(line);
        if (!fix.empty()) L.push_back(tidyText(fix));
    }
    L.push_back(heat(r, gen(r, "closing", c), c.anger));
    if (phone && service == DELIVERY) { c.v["addr"] = streetAddress(r); L.push_back(gen(r, "addr", c)); }
    std::string after = gen(r, "after", c);
    if (!after.empty()) L.push_back(heat(r, after, who.mood == 3 ? 0.85f : who.mood == 0 ? 0.5f : 0.1f));
    return L;
}

std::string recap(Rng& r, const std::vector<Spec>& want, int service, bool phone, const Ctx* ctx) {
    Ctx c;
    if (ctx) c = *ctx;
    c.phone = phone; c.service = service;
    if (c.asks < 1) c.asks = 1;
    std::string s = gen(r, "recap_pre", c);
    if (!s.empty()) s += " ";
    if (phone) s += service == DELIVERY ? "It's for delivery. " : "It's for pickup. ";
    for (size_t k = 0; k < want.size(); k++) {
        if (want.size() > 1) s += (k == 0 ? "First one: " : k == 1 ? "Second one: " : "Third one: ");
        std::string p = speakPizza(r, want[k], false);
        p[0] = (char)std::toupper((unsigned char)p[0]);
        s += p + ". ";
    }
    std::string post = gen(r, "recap_post", c);
    if (post.empty()) s.pop_back();
    else s += post;
    return s;
}

std::string readBack(Rng& r, const std::vector<Spec>& want, const std::vector<Spec>& entered, int wantService, int enteredService, bool phone, bool& ok, const Ctx* ctx) {
    ok = false;
    Ctx c;
    if (ctx) c = *ctx;
    c.phone = phone; c.service = wantService;
    std::string no = gen(r, "no_open", c);
    auto say = [&](const char* key) { return no + " " + gen(r, key, c); };
    if (phone && wantService != enteredService) return say(wantService == DELIVERY ? "wrong_svc_d" : "wrong_svc_c");
    c.v["n"] = std::to_string(want.size()); c.v["m"] = std::to_string(entered.size());
    if (entered.size() < want.size()) return say("too_few");
    if (entered.size() > want.size()) return say("too_many");
    for (size_t k = 0; k < want.size(); k++) {
        const Spec& w = want[k];
        Spec e = entered[k];
        auto diff = [&](const Spec& x) { int d = 0; for (auto& i : w.items) { const Item* j = x.find(i.ing); if (!j || j->part != i.part) d++; } return d; };
        if (diff(mirrored(e)) < diff(e)) e = mirrored(e);
        c.v["which"] = want.size() > 1 ? (k == 0 ? " on the first one" : k == 1 ? " on the second one" : " on the third one") : "";
        if (e.size != w.size) { c.v["size"] = upper(sizeName(w.size)); return say("wrong_size"); }
        if (e.crust != w.crust) { c.v["crust"] = upper(crustName(w.crust)); return say("wrong_crust"); }
        if (e.sauce != w.sauce) { c.v["sauce"] = upper(info(w.sauce).say); return say("wrong_sauce"); }
        if (e.sauceLevel != w.sauceLevel) { c.v["level"] = upper(levelName(w.sauceLevel)); return say("wrong_slevel"); }
        for (auto& i : w.items) {
            const Item* j = e.find(i.ing);
            c.v["top"] = info(i.ing).say;
            if (!j) return say("missing_top");
            if (j->level != i.level) { c.v["level"] = upper(levelName(i.level)); return say("wrong_level"); }
            if (j->part != i.part) { c.v["where"] = i.part == WHOLE ? "on the WHOLE pizza" : "on only HALF of it"; return say("wrong_part"); }
        }
        for (auto& j : e.items) if (!w.find(j.ing)) {
            if (j.ing == MOZZ) return say("extra_cheese");
            c.v["top"] = info(j.ing).say;
            return say("extra_top");
        }
    }
    ok = true;
    return gen(r, "ok_back", c);
}

std::string waitingBark(Rng& r, float patience, const Ctx* ctx) {
    Ctx c;
    if (ctx) c = *ctx;
    c.anger = 1 - patience;
    return heat(r, gen(r, "bark", c), 1 - patience);
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
        Ctx c; static const char* const TOD[] = {"morning", "afternoon", "evening", "night"}; c.v["tod"] = TOD[r.below(4)];
        return gen(r, "review_good", c);
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
    if (stars <= 2) out = heat(r, out, 0.9f) + pick(r, END1);
    else if (stars == 3) out += pick(r, END3);
    else out += " Otherwise okay.";
    return out;
}

}  // namespace pz
