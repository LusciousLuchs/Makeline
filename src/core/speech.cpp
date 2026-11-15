// Makeline - the dialogue grammar.
#include "speech.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>

namespace pz {
SpeechMemory* speechMem = nullptr;

namespace {
struct Rule { std::string tags, text; };
using Table = std::map<std::string, std::vector<Rule>>;

Rule R(const char* s) {
    std::string t = s, tags;
    if (!t.empty() && t[0] == '<') { size_t e = t.find('>'); tags = t.substr(1, e - 1); t = t.substr(e + 1); }
    return {tags, t};
}
void add(Table& T, const char* key, std::initializer_list<const char*> list) { for (auto s : list) T[key].push_back(R(s)); }

const Table& table() {
    static Table T = [] {
        Table T;
        add(T, "greet_counter", {
            "<polite,morning>Good morning.", "<polite,evening>Good evening.", "<polite,lunch>Hi there. Quick one for lunch.", "<polite>Hi, how's it going?",
            "<polite>Hey there, how are you?", "<polite,old>Well, hello there. I hope I'm not keeping you.", "<polite,old>Hello, dear. Take your time, I'll wait for you.",
            "<calm,hurry>Hey. Quick order.", "<calm,hurry>Hi. I'm in a bit of a hurry.", "<calm,mumble>Uh... hi.", "<calm,unsure>Hi, um, give me a second, I haven't decided.",
            "<calm,night>Hey. You're still open, right?", "<calm,young>Yo. What's good?", "<rude>Finally. Is anybody working here?", "<rude>Took you long enough.",
            "<rude,entitled>Hi. I'm in here all the time, so let's make this smooth.", "<angry>I've been standing here forever.", "<angry>Oh, now you see me.",
            "<calm>Hey.", "<calm>Hi.", "<calm>Yeah, hi.", "<calm>Hey, how's it going?", "<calm,old>Afternoon."});
        add(T, "greet_phone", {
            "<polite>Hi, I'd like to place an order, please.", "<polite>Hello, is this the pizza place? Great.", "<polite,old>Hello? Can you hear me all right? I'd like to order a pizza.",
            "<calm,hurry>Yeah, hi. Order for {v:svc}. Go.", "<calm>Hey, I'd like to order for {v:svc}.", "<calm>Hi. I need to put in an order for {v:svc}.",
            "<calm,mumble>Uh, yeah, hi... can I, uh, order something?", "<calm,unsure>Hi, um, I'm looking at the menu... okay, I think I'm ready.",
            "<rude>That took ten rings. Order for {v:svc}.", "<rude>Do you people ever pick up the phone?", "<angry>Finally! I've called three times.",
            "<calm,young>Hey, can I get a pizza for {v:svc}?", "<calm,night>Hi, I know it's late, but I'd like an order for {v:svc}."});
        add(T, "ask_first", {
            "<polite>Could I please get", "<polite>I'd like", "<polite>Can I get", "<calm>Can I get", "<calm>Let me get", "<calm>I'll have", "<calm>I want", "<calm>I'll take",
            "<calm>Give me", "<calm>Let's do", "<calm>Put me down for", "<calm>I'm gonna get", "<hurry>I need", "<hurry>Gimme", "<rude>Gimme", "<rude>I need", "<young>Lemme get",
            "<old>I think I'll have", "<unsure>I guess I'll do", "<unsure>Maybe... let's go with", "<mumble>Uh, I'll have", "<polite,old>I would like"});
        add(T, "ask_next", {
            "Also", "And", "And then", "Plus", "Then I need", "Throw in", "Add on", "And I also want", "On top of that, give me",
            "<unsure>Oh, and maybe", "<hurry>And", "<polite>And could I also get", "<v2>Second one's", "<v3>Third one's"});
        add(T, "aside", {
            "<evening>I've got the whole family over and nobody wants to cook.", "<evening>The game's on in twenty minutes.", "<evening,phone>We're about to sit down for a movie.",
            "<lunch>I only get half an hour for lunch.", "<lunch>It's for the office. Everybody's starving.", "<morning>Don't ask. It's a pizza-for-breakfast kind of day.",
            "<morning>We've got a meeting at noon and nobody brought food.", "<night>I just got off a double shift.", "<night>Can't sleep. Might as well eat.",
            "<old>My grandkids are coming over and they live on this stuff.", "<young>My roommates won't shut up until there's food.", "<phone,delivery>I am not leaving the house tonight.",
            "It's my kid's birthday party, so it'd better be right.", "We've got people coming over.", "It's been one of those days.", "I've been thinking about this all day.",
            "<angry>And I'm already in a bad mood, so.", "<hurry>I've got about ten minutes before I have to be somewhere.", "My wife's making me order, she says I'll burn water."});
        add(T, "closing", {
            "That's it.", "That's everything.", "I think that's all.", "And that's the order.", "That should do it.", "<polite>That'll do it, thank you.", "<polite>That's all for me. Thanks!",
            "<rude>That's it. Try not to mess it up.", "<rude>That's it. Write it down this time.", "<hurry>That's it. How long?", "<unsure>I think that's... yeah. Yeah, that's it. Maybe.",
            "<mumble>Mm-hm. That's it.", "<old,polite>That's all, dear. Thank you."});
        add(T, "after", {
            "", "", "", "<hurry>And hurry it up, I've got places to be.", "<hurry>How long? No, don't tell me. Just hurry.", "<mumble>...you get all that?", "<mumble>Mhm. Yeah. That.",
            "<unsure>Was that right? You wrote it down, right?", "<entitled>And it better be right this time.", "<entitled>Last time it was wrong. I WILL be checking.",
            "<polite>Thanks, I appreciate it.", "<calm>Okay, thanks.", "<calm>Cool. How long, roughly?", "<rude>Don't screw it up.", "<angry>I'm not saying it again."});
        add(T, "addr", {"It's {v:addr}.", "That's {v:addr}.", "Deliver it to {v:addr}.", "I'm at {v:addr}.", "<hurry>{v:addr}. Go.", "<unsure>It's... {v:addr}. Yeah.", "<rude>{v:addr}. Try to find it."});
        add(T, "no_open", {
            "<polite>Hmm, not quite.", "<polite>Close, but no.", "<polite>Sorry, I think something's off.", "<polite,old>Oh, dear. No, that's not what I asked for.", "<calm>No, that's not right.",
            "<calm>Nope.", "<calm>Wait, no.", "<rude>No. No, no, no.", "<rude>That's not what I said.", "<angry>Are you even listening?", "<angry>Are you kidding me?",
            "<again>Again? It's still wrong.", "<again2>I've told you twice now.", "<hurry>No. Wrong. Fix it.", "<unsure>Um, I don't think that's right?"});
        add(T, "wrong_size", {"I said {v:size}{v:which}.", "It's a {v:size}{v:which}, not that.", "{v:size}. I want a {v:size}{v:which}.", "<polite>I need a {v:size}{v:which}, sorry.", "<rude>Size! {v:size}{v:which}!"});
        add(T, "wrong_crust", {"I said {v:crust}{v:which}.", "It's {v:crust} crust{v:which}.", "<polite>It should be {v:crust}{v:which}, please.", "<rude>{v:crust}. That's the crust.{v:which}"});
        add(T, "wrong_sauce", {"The sauce is {v:sauce}{v:which}.", "It's {v:sauce}{v:which}, I said that.", "<polite>I asked for {v:sauce}{v:which}.", "<rude>{v:sauce}. Not whatever that is.{v:which}"});
        add(T, "wrong_slevel", {"I said {v:level} sauce{v:which}.", "<polite>The sauce should be {v:level}{v:which}.", "{v:level} on the sauce{v:which}. Come on."});
        add(T, "missing_top", {"Where's the {v:top}{v:which}?", "You left off the {v:top}{v:which}.", "<polite>I think the {v:top} is missing{v:which}.", "<rude>Where's my {v:top}{v:which}? I said {v:top}."});
        add(T, "wrong_level", {"I said {v:level} {v:top}{v:which}.", "<polite>The {v:top} should be {v:level}{v:which}.", "{v:level} {v:top}{v:which}. That's what I asked for."});
        add(T, "wrong_part", {"The {v:top} goes {v:where}{v:which}.", "<polite>I wanted the {v:top} {v:where}{v:which}.", "{v:top}: {v:where}. Not that.{v:which}"});
        add(T, "extra_top", {"I never said {v:top}{v:which}.", "Why is there {v:top} on that{v:which}?", "<polite>I didn't ask for {v:top}{v:which}, I don't think.", "<rude>I'm not paying for {v:top} I didn't order{v:which}."});
        add(T, "extra_cheese", {"I said NO cheese{v:which}.", "<polite>I don't want any cheese{v:which}, remember?", "Cheese? I told you no cheese{v:which}."});
        add(T, "too_few", {"I ordered {v:n}. You're missing one.", "That's only {v:m}. I wanted {v:n}.", "<polite>Sorry, I think I asked for {v:n} pizzas?"});
        add(T, "too_many", {"I only ordered {v:n}.", "That's more than I asked for. I wanted {v:n}.", "<rude>I'm not paying for extra pizzas. {v:n}. I said {v:n}."});
        add(T, "wrong_svc_d", {"I said DELIVERY. I'm not coming down there.", "<polite>It's for delivery, actually.", "Delivery. I'm not leaving the house."});
        add(T, "wrong_svc_c", {"I said pickup. I'm coming to you.", "<polite>It's pickup, actually. I'll come get it.", "Pickup! Not delivery."});
        add(T, "ok_back", {"Yes, that's it.", "Yeah, that's right.", "<polite>Perfect, thank you.", "<polite,old>Yes, that's lovely, dear.", "<rude>Yes. Finally.", "<hurry>Yup. Go.",
            "<unsure>I think so? Yeah. Okay.", "<mumble>Mm. Yeah.", "That's what I said.", "<calm>Sounds right.", "<angry>YES. That's what I've been saying."});
        add(T, "recap_pre", {
            "<again,polite,!again2>Sure, no problem. Let me say it again.", "<again,polite,!again2>Of course.", "<again,calm,!again2>Okay, once more.", "<again,old,!again2>Oh, certainly, dear.",
            "<again2,!again3>That's the second time. Pay attention.", "<again2,!again3>Second time now. Listen.", "<again3>Third time. Are you even listening?", "<again3>This is the third time I'm saying this.",
            "<rude,!again2>I just told you.", "<angry,!again3>Oh my GOD. Listen this time.", "<hurry,!again3>Seriously? Okay, one more time, fast.", "<unsure,!again2>Sorry, I... okay. Let me start over.",
            "<calm,!again>Sure."});
        add(T, "recap_post", {"", "", "", "<polite>Thanks.", "<calm>Got it?", "<rude>Write it DOWN.", "<angry>Was that slow enough for you?", "<hurry>Okay?", "<unsure>Is that... okay?", "<again2>Got it this time?"});
        add(T, "bark", {
            "<calm,fresh>*checks phone*", "<calm>*sighs quietly*", "<calm>Busy tonight.", "<calm,hurry>How much longer is this going to take?", "<calm,old>Take your time, dear. I'm not going anywhere. Much.",
            "<calm,unsure>Is that mine? ...no?", "<calm,mumble>*mutters something about the wait*", "<slow>It's been {mins} now.", "<slow>{mins}. That's a long time for a pizza.",
            "<slow,calm>I don't mean to rush, but it's been {mins}.", "<veryslow>It's been {mins}. Is it in the oven or did you forget it?", "<veryslow,rude>{mins}! You're kidding me.",
            "<extreme>{mins}. I'm done. This is ridiculous.", "<extreme,angry>{mins}! I want my money back.", "<rude>Is anyone even working back there?", "<rude>I could have made it myself by now.",
            "<angry>Who's in charge here? You? Of course it is.", "<angry>I'm never coming back here.", "<rude,hurry>I'm on my lunch break. LUNCH. Break.", "<fresh,rude>Are you guys short-staffed or just slow?"});
        add(T, "tic_", {"{tic}"});
        add(T, "review_open", {"", "", "Stopped in this {v:tod}.", "Ordered this {v:tod}.", "Went by this {v:tod}.", "Not my first time here."});
        add(T, "review_good", {"It was fine. Pizza was actually right for once.", "Fast, hot, and what I ordered. Shocked.", "No complaints. Which is rare for me.", "Good pizza. Kid at the counter looked tired though.",
            "They got it right. Five stars, I guess.", "Hot and correct. That's all I ask.", "Came out perfect. Somebody back there knows what they're doing.", "Honestly the best in town. Don't tell them I said that.",
            "Quick service, friendly enough, pizza was exactly what I asked for.", "Ordered twice this week. Both times right. That's a miracle."});
        return T;
    }();
    return T;
}

bool has(const std::string& tags, const std::string& tag) {
    size_t at = 0;
    while (at <= tags.size()) {
        size_t e = tags.find(',', at);
        if (e == std::string::npos) e = tags.size();
        if (tags.compare(at, e - at, tag) == 0 && e - at == tag.size()) return true;
        at = e + 1;
    }
    return false;
}
bool holds(const std::string& tag, const Ctx& c) {
    bool neg = !tag.empty() && tag[0] == '!';
    std::string t = neg ? tag.substr(1) : tag;
    const Person* w = c.who;
    bool r = true;
    int mood = w ? w->mood : -1, age = w ? w->age : 35;
    if (t == "polite") r = c.anger < 0.4f && mood != 3;
    else if (t == "calm") r = c.anger < 0.4f;
    else if (t == "rude") r = c.anger >= 0.55f || mood == 3;
    else if (t == "angry") r = c.anger >= 0.7f;
    else if (t == "phone") r = c.phone;
    else if (t == "counter") r = !c.phone;
    else if (t == "delivery") r = c.service == DELIVERY;
    else if (t == "carryout") r = c.service == CARRYOUT;
    else if (t == "morning") r = c.hour < 11;
    else if (t == "lunch") r = c.hour >= 11 && c.hour < 15;
    else if (t == "evening") r = c.hour >= 15 && c.hour < 21;
    else if (t == "night") r = c.hour >= 21 || c.hour < 5;
    else if (t == "old") r = age >= 60;
    else if (t == "young") r = age <= 24;
    else if (t == "hurry") r = mood == 0;
    else if (t == "mumble") r = mood == 1;
    else if (t == "unsure") r = mood == 2;
    else if (t == "entitled") r = mood == 3;
    else if (t == "fresh") r = c.minutes < 8;
    else if (t == "slow") r = c.minutes >= LATE_MIN;
    else if (t == "veryslow") r = c.minutes >= VERY_LATE_MIN;
    else if (t == "extreme") r = c.minutes >= EXTREME_MIN;
    else if (t == "again") r = c.asks >= 1;
    else if (t == "again2") r = c.asks >= 2;
    else if (t == "again3") r = c.asks >= 3;
    else if (t == "v2") r = c.v.count("nth") && c.v.at("nth") == "2";
    else if (t == "v3") r = c.v.count("nth") && c.v.at("nth") == "3";
    return neg ? !r : r;
}
bool eligible(const Rule& rule, const Ctx& c) {
    if (rule.tags.empty()) return true;
    size_t at = 0;
    while (at <= rule.tags.size()) {
        size_t e = rule.tags.find(',', at);
        if (e == std::string::npos) e = rule.tags.size();
        if (e > at && !holds(rule.tags.substr(at, e - at), c)) return false;
        at = e + 1;
    }
    return true;
}
uint32_t hashOf(const std::string& s, size_t i) {
    uint32_t h = 2166136261u;
    for (char ch : s) { h ^= (unsigned char)ch; h *= 16777619u; }
    return h ^ (uint32_t)(i * 2654435761u);
}

std::string expand(Rng& r, const std::string& tpl, const Ctx& c, int depth);

std::string pickRule(Rng& r, const std::string& key, const Ctx& c, int depth) {
    auto it = table().find(key);
    if (it == table().end() || depth > 6) return "";
    const auto& rules = it->second;
    std::vector<float> w(rules.size(), 0);
    float sum = 0;
    for (size_t i = 0; i < rules.size(); i++) {
        if (!eligible(rules[i], c)) continue;
        float x = 1;
        if (!rules[i].tags.empty()) x += 0.6f;                       // a line made for this moment beats a stock one
        if (speechMem) for (uint32_t h : speechMem->recent) if (h == hashOf(key, i)) { x *= 0.08f; break; }   // said lately: much less likely
        w[i] = x; sum += x;
    }
    if (sum <= 0) return "";
    float t = r.f() * sum;
    size_t pick = 0;
    for (size_t i = 0; i < rules.size(); i++) { if (w[i] <= 0) continue; pick = i; if ((t -= w[i]) <= 0) break; }
    if (speechMem) { speechMem->recent.push_back(hashOf(key, pick)); if (speechMem->recent.size() > 48) speechMem->recent.pop_front(); }
    return expand(r, rules[pick].text, c, depth + 1);
}

std::string expand(Rng& r, const std::string& tpl, const Ctx& c, int depth) {
    static const char* const TICS[] = {"you know", "honestly", "man", "I swear", "okay", "like", "right"};
    std::string out;
    for (size_t i = 0; i < tpl.size(); i++) {
        char ch = tpl[i];
        if (ch == '{') {
            size_t e = tpl.find('}', i);
            if (e == std::string::npos) { out += ch; continue; }
            std::string name = tpl.substr(i + 1, e - i - 1);
            i = e;
            if (name.compare(0, 2, "v:") == 0) { auto f = c.v.find(name.substr(2)); if (f != c.v.end()) out += f->second; }
            else if (name == "tic") { if (c.who && r.chance(0.35f)) out += std::string(", ") + TICS[(c.who->look >> 7) % 7] + ","; }
            else if (name == "mins") out += minutesText(c.minutes);
            else out += pickRule(r, name, c, depth);
        } else if (ch == '[') {
            size_t e = tpl.find(']', i);
            if (e == std::string::npos) { out += ch; continue; }
            std::vector<std::string> opts;
            size_t at = i + 1;
            while (at <= e) { size_t b = tpl.find('|', at); if (b == std::string::npos || b > e) b = e; opts.push_back(tpl.substr(at, b - at)); at = b + 1; }
            out += opts[r.below((int)opts.size())];
            i = e;
        } else out += ch;
    }
    return out;
}

std::string tidy(std::string s) {
    std::string o;
    bool cap = true;
    for (size_t i = 0; i < s.size(); i++) {
        char ch = s[i];
        if (ch == ' ' && (o.empty() || o.back() == ' ')) continue;
        if ((ch == '.' || ch == ',' || ch == '!' || ch == '?') && !o.empty() && o.back() == ' ') o.pop_back();
        if (cap && std::isalpha((unsigned char)ch)) { ch = (char)std::toupper((unsigned char)ch); cap = false; }
        o += ch;
        if (ch == '.' || ch == '!' || ch == '?') cap = true;
    }
    while (!o.empty() && o.back() == ' ') o.pop_back();
    return o;
}
}  // namespace

std::string gen(Rng& r, const std::string& symbol, const Ctx& c) { return tidy(pickRule(r, symbol, c, 0)); }
std::string genRaw(Rng& r, const std::string& symbol, const Ctx& c) { return pickRule(r, symbol, c, 0); }
std::string tidyText(const std::string& s) { return tidy(s); }

std::string article(const std::string& w) {
    if (w.empty()) return "a";
    char c = (char)std::tolower((unsigned char)w[0]);
    if (w.compare(0, 2, "XL") == 0 || w.compare(0, 2, "X-") == 0) return "an";
    return std::string(1, c).find_first_of("aeiou") != std::string::npos ? "an" : "a";
}

std::string minutesText(float m) {
    int n = (int)std::lround(m);
    if (n <= 1) return "a minute";
    if (n < 6) return "a few minutes";
    static const char* const W[] = {"", "", "", "", "", "five", "six", "seven", "eight", "nine", "ten", "eleven", "twelve", "thirteen", "fourteen", "fifteen", "sixteen", "seventeen", "eighteen", "nineteen", "twenty"};
    if (n <= 20) return std::string(W[n]) + " minutes";
    if (n < 25) return "over twenty minutes";
    if (n < 30) return "almost half an hour";
    if (n < 35) return "over half an hour";
    if (n < 45) return "nearly forty-five minutes";
    return "forty-five minutes. Forty-five";
}

}  // namespace pz
