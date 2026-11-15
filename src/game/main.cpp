// Makeline - a pizza shop game by OAB Foundry. Startup, the main loop, the title and end-of-day screens.
#include "game.hpp"
#include "updater.hpp"
#include "version.hpp"
#include "json.hpp"
#include "talk.hpp"
#include "eula.inc"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

using namespace pz;
using ui::INK;

Game G;
bool gShotMode = false;
extern const unsigned char* logoJpg(int* size);   // ui.cpp holds the embedded files

namespace {
enum Screen { SPLASH, EULA, TITLE, PLAY, DAYEND };
std::string eulaAgreed;      // a fingerprint of the agreement the player accepted
bool eulaFromTitle = false;  // reading it again from the title (nothing to agree to)
float eulaScroll = 0;
int screen = SPLASH;
float splashT = 0, reviewScroll = 0;
bool confirmNew = false, wantQuit = false;
bool& shotMode = gShotMode;
int verdict = 0;   // corporate, at closing: 0 fine, 1 a reprimand, 2 fired
uint64_t seed = 1;
int shiftArg = -1;   // (screenshots) --shift hours after midnight
Texture2D logo{};
Pizza titlePizza;

std::string savePath() { return std::string(GetApplicationDirectory()) + "makeline_save.json"; }
void save() { if (!shotMode) SaveFileText(savePath().c_str(), (char*)G.S.toJson().c_str()); }
std::string eulaFingerprint() {
    uint64_t h = 1469598103934665603ull;
    for (const char* p = EULA_TEXT; *p; p++) { h ^= (unsigned char)*p; h *= 1099511628211ull; }
    char b[24]; snprintf(b, sizeof b, "%016llx", (unsigned long long)h);
    return b;
}
std::string settingsPath() { return std::string(GetApplicationDirectory()) + "makeline_settings.json"; }
void saveSettings() {
    if (shotMode) return;
    nlohmann::json j;
    j["eulaAgreed"] = eulaAgreed; j["strongLanguage"] = pz::strongLanguage; j["hud"] = G.hud;
    std::string t = j.dump(1);
    SaveFileText(settingsPath().c_str(), (char*)t.c_str());
}
void loadSettings() {
    if (shotMode || !FileExists(settingsPath().c_str())) return;
    char* t = LoadFileText(settingsPath().c_str());
    if (!t) return;
    nlohmann::json j = nlohmann::json::parse(t, nullptr, false);
    UnloadFileText(t);
    if (!j.is_object()) return;
    eulaAgreed = j.value("eulaAgreed", std::string());
    pz::strongLanguage = j.value("strongLanguage", true);
    G.hud = j.value("hud", false);
}
void load() {
    if (shotMode || !FileExists(savePath().c_str())) return;
    char* t = LoadFileText(savePath().c_str());
    if (t) { G.S.fromJson(t); UnloadFileText(t); }
    if (G.S.fired) G.S = Store{};
}
void startShift() {
    if (!G.S.schedSet) { G.S.schedSeed = seed; G.S.schedSet = true; }
    if (shiftArg >= 0) G.S.forceStart = shiftArg;
    G.S.startDay(seed);
    G.tool = -1; G.pickTicket = -1; G.toasts.clear(); G.paused = G.chart = G.help = false;
    screen = PLAY;
    G.playing = true; G.office = false;
    worldReset();
}

void toast(const std::string& text, int stars = 0) {
    G.toasts.push_back({text, stars, 0});
    if (G.toasts.size() > 4) G.toasts.erase(G.toasts.begin());
}
void events() {
    Store& S = G.S;
    char b[96];
    for (auto& e : S.events) {
        switch (e.kind) {
            case E_DOOR: sfx(SFX_DOOR); break;
            case E_REVIEW: toast(e.text, e.a); sfx(e.a >= 4 ? SFX_GOOD : SFX_BAD); break;
            case E_OVEN_OUT: sfx(SFX_OVEN); break;
            case E_READY: snprintf(b, sizeof b, "Order #%d is boxed and ready.", e.a); toast(b); sfx(SFX_DING); break;
            case E_DRIVER: snprintf(b, sizeof b, "%s left on a delivery run.", e.text.c_str()); toast(b); break;
            case E_NOTE: toast(e.text); break;
            case E_STORMED: toast("A customer gave up and walked out."); sfx(SFX_BAD); break;
            case E_MISSED: toast("Missed a call. They hung up."); sfx(SFX_BAD); break;
            case E_TALK: sfx(SFX_TALK); break;
            default: break;
        }
    }
    S.events.clear();
}

void topBar() {
    Store& S = G.S;
    if (!G.hud) return;   // maximum immersion: the clock and rating are on the wall sign, the rest is in the office
    DrawRectangle(-2000, 0, 5600, 64, INK);
    char b[64];
    snprintf(b, sizeof b, "Day %d", S.day);
    ui::text(b, 20, 18, 26, ui::CREAM);
    ui::text(S.closed ? (S.shiftEnd() >= CLOSE_MIN ? "CLOSING - finish up" : "SHIFT OVER - finish up") : S.clock() + "   (until " + Store::timeText(S.shiftEnd()) + ")", 120, 18, 26, S.closed ? ui::RED : ui::YELLOW);
    float rt = S.rating();
    ui::stars(1150, 18, 28, rt, {110, 92, 82, 255});
    if (rt > 0) snprintf(b, sizeof b, "%.1f", rt); else snprintf(b, sizeof b, "new");
    ui::text(b, 1308, 18, 26, ui::CREAM);
    snprintf(b, sizeof b, "$%.2f", S.money);
    ui::text(b, 1586, 18, 26, {170, 230, 170, 255}, 2);
    int rp = S.reprimandsThisWeek();
    if (rp) { snprintf(b, sizeof b, "Reprimands: %d of 3", rp); ui::text(b, 1120, 20, 22, ui::RED, 2); }
}

void drawToasts() {
    if (!G.hud) return;   // reviews show up on the wall sign instead
    float y = G.mode == M_WALK ? 16 : 76;
    for (auto& t : G.toasts) {
        float in = ui::ease(t.age / 0.3f), out = ui::ease((t.age - 6.5f) / 0.5f);
        float w = 440, h = ui::wrap(t.text, 0, 0, w - 28, 18, INK, false) + (t.stars ? 46 : 20);
        float x = 1600 - (w + 16) * in + (w + 40) * out;
        Rectangle r{x, y, w, h};
        ui::panel(r, t.stars == 0 ? ui::CREAM : t.stars >= 4 ? Color{222, 240, 214, 255} : t.stars == 3 ? ui::PAPER : Color{250, 214, 204, 255}, 10, 3, true);
        float ty = r.y + 10;
        if (t.stars) { ui::stars(r.x + 12, ty, 22, (float)t.stars); ui::text("NEW REVIEW", r.x + w - 14, ty + 2, 16, ui::GREY, 2); ty += 28; }
        ui::wrap(t.text, r.x + 14, ty, w - 28, 18, INK);
        y += h + 12;
    }
}

void helpPanel() {
    DrawRectangle(0, 0, 1600, 900, {30, 20, 16, 160});
    Rectangle box{240, 90, 1120, 720};
    ui::panel(box, ui::PAPER, 14, 4, true);
    ui::text("HOW TO RUN THE SHOP", box.x + 34, box.y + 22, 36, ui::DKRED);
    static const char* STEPS[][2] = {
        {"1. Take the order", "Customers walk in or call. Click them (or the ringing phone), listen, and punch what they say into the order screen. They change their minds, mumble, and don't like repeating themselves. What YOU enter is what gets made."},
        {"2. Make it", "On the makeline, pick the ticket, grab the right dough, then sauce, cheese and top it by hand. Hold to ladle and sprinkle, click to place pepperoni. Portions matter: too little or too much and you'll hear about it."},
        {"3. Bake, cut, box", "Send it through the oven. When it comes out, drag the cutter across it (pie cut, or squares for thin crust) and box it before it goes cold."},
        {"4. Hand it over", "Counter and carryout customers are in the lobby: click them when their order's ready. Drivers take deliveries by themselves."},
        {"Reviews", "Every order can turn into a review. Wrong pizzas, bad portions and long waits sink your rating. Get it right, fast, and it climbs."}};
    float y = box.y + 84;
    for (auto& s : STEPS) {
        ui::text(s[0], box.x + 36, y, 24, INK);
        y += 30;
        y += ui::wrap(s[1], box.x + 36, y, box.width - 72, 20, {90, 74, 64, 255}) + 16;
    }
    if (ui::button({box.x + box.width - 170, box.y + 20, 140, 46}, "Got it", ui::YELLOW, true, 22)) G.help = false;
}

void titleScreen() {
    Store& S = G.S;
    EndMode2D();
    ClearBackground({196, 58, 44, 255});
    {   // a pizza the size of the screen, slowly turning
        Camera3D c{};
        c.position = {-0.16f, 0.62f, 0.2f}; c.target = {-0.16f, 0, 0.02f}; c.up = {0, 0, -1}; c.fovy = 45; c.projection = CAMERA_PERSPECTIVE;
        g3::begin(c);
        for (int i = -8; i < 8; i++) for (int j = -5; j < 5; j++) g3::quadUp({i * 0.12f, -0.001f, j * 0.12f}, 0.12f, 0.12f, (i + j) & 1 ? Color{214, 66, 50, 255} : Color{226, 82, 62, 255});
        g3::cyl({0, 0, 0}, 0.245f, 0.006f, {206, 170, 120, 255}, 32);
        g3::Frame f{{0, 0.006f, 0}, ui::time * 4};
        g3::pizza(titlePizza, f, 0, 0);
        g3::end();
    }
    BeginMode2D(Camera2D{ui::offset, {0, 0}, 0, ui::scale});
    Rectangle card{90, 110, 620, 680};
    ui::panel(card, ui::CREAM, 18, 5, true);
    ui::text("MAKELINE", card.x + 40, card.y + 30, 96, ui::DKRED);
    ui::text("you're the manager. try to stay that way.", card.x + 46, card.y + 126, 26, INK);
    float rt = S.rating();
    char b[96];
    if (rt > 0) {
        ui::stars(card.x + 46, card.y + 180, 34, rt);
        snprintf(b, sizeof b, "%.1f  (%d reviews)", rt, (int)S.reviews.size());
        ui::text(b, card.x + 240, card.y + 184, 24, INK);
    } else ui::text("No reviews yet. That won't last.", card.x + 46, card.y + 184, 22, ui::GREY);
    snprintf(b, sizeof b, S.day > 1 || !S.reviews.empty() ? "CLOCK IN: DAY %d" : "CLOCK IN", S.day);
    if (!S.schedSet) { S.schedSeed = seed; S.schedSet = true; }
    {   // the boss's schedule
        int st = S.shiftFor(S.day);
        ui::text("Your shift: " + Store::timeText(st) + " to " + Store::timeText(st + SHIFT_MINS), card.x + 46, card.y + 212, 24, ui::DKRED);
    }
    float y = card.y + 250;
    if (!confirmNew) {
        if (ui::button({card.x + 46, y, 528, 84}, b, ui::RED, true, 36)) { sfx(SFX_DING); startShift(); }
        if (ui::button({card.x + 46, y + 110, 256, 58}, "How to play", ui::YELLOW, true, 24)) G.help = true;
        if (ui::button({card.x + 318, y + 110, 256, 58}, "Start over", {214, 204, 186, 255}, S.day > 1 || !S.reviews.empty(), 24)) confirmNew = true;
        if (ui::button({card.x + 46, y + 190, 528, 58}, "Quit", {214, 204, 186, 255}, true, 24)) wantQuit = true;
    } else {
        ui::wrap("Start over from day 1? Your reviews and money go in the bin.", card.x + 46, y, 528, 26, INK);
        if (ui::button({card.x + 46, y + 110, 256, 58}, "Yes, start over", ui::RED, true, 24)) { G.S = Store{}; save(); confirmNew = false; }
        if (ui::button({card.x + 318, y + 110, 256, 58}, "Never mind", ui::YELLOW, true, 24)) confirmNew = false;
    }
    // the updater
    update::Status u = update::status();
    std::string us;
    switch (u.state) {
        case update::State::Checking: us = "Checking for updates..."; break;
        case update::State::UpToDate: us = "You have the newest version."; break;
        case update::State::Downloading: us = "Downloading version " + u.latest + "..."; break;
        case update::State::Installing: us = "Installing the update..."; break;
        case update::State::Installed: us = "Version " + u.latest + " is installed."; break;
        case update::State::Failed: us = "Update check: " + u.note; break;
        default: break;
    }
    ui::wrap(us, card.x + 46, card.y + 560, 380, 18, ui::GREY);
    if (u.state == update::State::Installed) { if (ui::button({card.x + 420, card.y + 552, 154, 44}, "Restart now", ui::GREEN, true, 20)) { if (update::restart()) wantQuit = true; } }
    else if (u.state == update::State::UpToDate || u.state == update::State::Failed) { if (ui::button({card.x + 420, card.y + 552, 154, 44}, "Check again", {214, 204, 186, 255}, true, 18)) update::checkNow(); }
    if (ui::button({card.x + 46, card.y + 596, 220, 38}, pz::strongLanguage ? "Strong language: ON" : "Strong language: OFF", pz::strongLanguage ? ui::GREEN : Color{214, 204, 186, 255}, true, 17)) { pz::strongLanguage = !pz::strongLanguage; saveSettings(); }
    if (ui::button({card.x + 280, card.y + 596, 150, 38}, "Agreement", {214, 204, 186, 255}, true, 17)) { eulaFromTitle = true; eulaScroll = 0; screen = EULA; }
    ui::text(std::string("Version ") + version::text() + "   -   a work of satire", card.x + 46, card.y + 648, 18, ui::GREY);
    ui::text("OAB Foundry", card.x + 574, card.y + 648, 18, ui::GREY, 2);
    if (G.help) helpPanel();
}

void dayEnd() {
    Store& S = G.S;
    DrawRectangle(-2000, -2000, 5600, 4900, {58, 44, 40, 255});
    Rectangle card{100, 60, 560, 780};
    ui::panel(card, ui::CREAM, 16, 5, true);
    char b[128];
    snprintf(b, sizeof b, "DAY %d", S.day);
    ui::text(b, card.x + 36, card.y + 24, 64, ui::DKRED);
    ui::text("The lights are off. Here's how it went.", card.x + 38, card.y + 96, 21, ui::GREY);
    float y = card.y + 150;
    auto stat = [&](const char* k, const std::string& v, Color c = INK) { ui::text(k, card.x + 40, y, 24, INK); ui::text(v, card.x + card.width - 40, y, 24, c, 2); y += 36; };
    stat("Orders taken", std::to_string(S.dayOrders));
    stat("Walked out or hung up", std::to_string(S.dayLost), S.dayLost ? ui::DKRED : INK);
    snprintf(b, sizeof b, "$%.2f", S.dayMoney); stat("Sales (after refunds)", b);
    snprintf(b, sizeof b, "$%.2f", S.dayTips); stat("Tips", b);
    if (S.dayWages > 0) { snprintf(b, sizeof b, "-$%.2f", S.dayWages); stat("Wages", b, ui::DKRED); }
    float sum = 0; for (auto& r : S.dayReviews) sum += r.stars;
    float avg = S.dayReviews.empty() ? 0 : sum / S.dayReviews.size();
    y += 10;
    ui::text("Today's reviews", card.x + 40, y, 26, INK);
    if (avg > 0) { ui::stars(card.x + 300, y, 30, avg); snprintf(b, sizeof b, "%.1f", avg); ui::text(b, card.x + card.width - 40, y, 26, INK, 2); }
    else ui::text("none", card.x + card.width - 40, y, 26, ui::GREY, 2);
    y += 48;
    ui::text("Store rating", card.x + 40, y, 26, INK);
    ui::stars(card.x + 300, y, 30, S.rating()); snprintf(b, sizeof b, "%.1f", S.rating()); ui::text(S.rating() > 0 ? b : "new", card.x + card.width - 40, y, 26, INK, 2);
    y += 62;
    if (verdict) {   // the letter from corporate
        Rectangle letter{card.x + 30, y - 8, card.width - 60, 196};
        DrawRectangleRec({letter.x + 4, letter.y + 5, letter.width, letter.height}, ui::SHADOW);
        DrawRectangleRec(letter, ui::PAPER);
        DrawRectangleLinesEx(letter, 3, ui::DKRED);
        ui::text(verdict == 2 ? "NOTICE OF TERMINATION" : "WRITTEN REPRIMAND", letter.x + 16, letter.y + 12, 26, ui::DKRED);
        if (verdict == 2) snprintf(b, sizeof b, "Three reprimands in one week. The store closed today at %.1f stars. Hand in your keys. You're fired.", S.rating());
        else snprintf(b, sizeof b, "The store closed today at %.1f stars. Under %.1f is not acceptable. Reprimand %d of 3 this week.", S.rating(), REPRIMAND_BELOW, S.reprimandsThisWeek());
        ui::wrap(b, letter.x + 16, letter.y + 50, letter.width - 32, 20, INK);
        ui::text("- Regional Office", letter.x + letter.width - 16, letter.y + letter.height - 30, 18, ui::GREY, 2);
    } else {
        const char* note = avg == 0 ? "Nobody said anything. Take it." : avg >= 4.5f ? "Corporate would like a word. A nice one, for once." : avg >= 3.5f ? "A decent night. The regulars will be back." :
            "Rough night, but the store's score held. This time.";
        y += ui::wrap(note, card.x + 40, y, card.width - 80, 22, ui::DKRED) + 10;
        if (S.day == 1 && S.guides) ui::wrap("From tomorrow the portion guides come off. Learn the chart. (You can turn them back on from the pause menu.)", card.x + 40, y, card.width - 80, 19, {90, 74, 64, 255});
    }
    if (verdict != 2) {
        int st = S.shiftFor(S.day + 1);
        ui::text("The boss has you in tomorrow: " + Store::timeText(st) + " to " + Store::timeText(st + SHIFT_MINS), card.x + 40, card.y + card.height - 134, 19, ui::DKRED);
    }
    if (verdict == 2) {
        if (ui::button({card.x + 40, card.y + card.height - 100, 480, 70}, "START OVER AT A NEW STORE", ui::RED, true, 26)) { G.S = Store{}; save(); verdict = 0; screen = TITLE; }
    } else {
        if (ui::button({card.x + 40, card.y + card.height - 100, 300, 70}, "NEXT DAY", ui::RED, true, 30)) {
            if (S.day == 1) S.guides = false;
            S.day++; save(); sfx(SFX_DING); startShift();
        }
        if (ui::button({card.x + 356, card.y + card.height - 100, 164, 70}, "Clock out", {214, 204, 186, 255}, true, 22)) {
            if (S.day == 1) S.guides = false;
            S.day++; save(); screen = TITLE;
        }
    }
    if (ui::button({card.x + 40, card.y + card.height - 164, 480, 50}, "Staff and hiring", ui::YELLOW, verdict != 2, 22)) { G.office = true; G.officeTab = 0; }
    // the reviews, newest first
    Rectangle list{700, 60, 800, 780};
    ui::panel(list, {246, 240, 228, 255}, 16, 5, true);
    ui::text("WHAT THEY WROTE", list.x + 30, list.y + 20, 30, INK);
    reviewScroll = std::max(0.f, reviewScroll - GetMouseWheelMove() * 60);
    float ry = list.y + 76 - reviewScroll, bottom = list.y + list.height - 16;
    for (auto& n : S.dayNotes) {   // what the crew got up to
        if (ry >= list.y + 66 && ry + 30 <= bottom) { DrawCircleV({list.x + 36, ry + 12}, 5, ui::BLUE); ui::text(n, list.x + 52, ry, 19, INK); }
        ry += 30;
    }
    if (!S.dayNotes.empty()) ry += 10;
    if (S.dayReviews.empty()) ui::text("No reviews today.", list.x + 30, ry, 22, ui::GREY);
    for (size_t i = S.dayReviews.size(); i-- > 0;) {
        auto& r = S.dayReviews[i];
        std::string why;
        for (size_t k = 0; k < r.reasons.size() && k < 4; k++) why += (k ? ",  " : "") + r.reasons[k];
        float h = 34 + ui::wrap(r.text, 0, 0, list.width - 80, 19, INK, false) + (why.empty() ? 0 : 24) + 14;
        if (ry >= list.y + 66 && ry + h <= bottom) {
            DrawRectangleRounded({list.x + 20, ry, list.width - 40, h - 8}, 0.12f, 6, r.stars >= 4 ? Color{226, 240, 218, 255} : r.stars == 3 ? ui::PAPER : Color{250, 220, 210, 255});
            ui::stars(list.x + 32, ry + 8, 22, (float)r.stars);
            ui::text(r.name, list.x + list.width - 36, ry + 8, 19, ui::GREY, 2);
            float th = ui::wrap(r.text, list.x + 34, ry + 34, list.width - 80, 19, INK);
            if (!why.empty()) ui::text("What went wrong: " + why, list.x + 34, ry + 36 + th, 16, ui::DKRED);
        }
        ry += h;
    }
    if (ry + reviewScroll > bottom) ui::text("scroll for more", list.x + list.width - 30, list.y + 28, 16, ui::GREY, 2);
    else if (ry < bottom - 200 && reviewScroll > 0) reviewScroll = std::max(0.f, reviewScroll - 30);
}

void eulaScreen() {
    DrawRectangle(-2000, -2000, 5600, 4900, {40, 32, 30, 255});
    Rectangle card{180, 50, 1240, 800};
    ui::panel(card, ui::CREAM, 16, 5, true);
    Rectangle ban{card.x + 30, card.y + 22, card.width - 60, 92};
    DrawRectangleRounded(ban, 0.15f, 6, ui::DKRED);
    ui::text("A WORK OF SATIRE", ban.x + ban.width / 2, ban.y + 6, 44, ui::PAPER, 1);
    ui::text("Everyone in this game is made up, and they are awful on purpose.", ban.x + ban.width / 2, ban.y + 58, 24, {250, 226, 214, 255}, 1);
    Rectangle box{card.x + 30, card.y + 130, card.width - 60, card.height - 130 - 100};
    float full = ui::wrap(EULA_TEXT, 0, 0, box.width - 40, 21, ui::INK, false);
    float maxScroll = std::max(0.f, full - box.height + 20);
    if (ui::hover(box)) eulaScroll = std::clamp(eulaScroll - GetMouseWheelMove() * 60, 0.f, maxScroll);
    eulaScroll = std::clamp(eulaScroll, 0.f, maxScroll);
    DrawRectangleRec(box, ui::PAPER);
    Vector2 a = ui::toScreen({box.x, box.y});
    BeginScissorMode((int)a.x, (int)a.y, (int)(box.width * ui::scale), (int)(box.height * ui::scale));
    ui::wrap(EULA_TEXT, box.x + 20, box.y + 10 - eulaScroll, box.width - 40, 21, ui::INK);
    EndScissorMode();
    DrawRectangleLinesEx(box, 2, ui::INK);
    if (maxScroll > 0) ui::text("scroll for the rest", box.x + box.width - 10, box.y + box.height + 4, 16, ui::GREY, 2);
    if (eulaFromTitle) {
        if (ui::button({card.x + card.width / 2 - 100, card.y + card.height - 74, 200, 56}, "Back", ui::YELLOW, true, 26) || IsKeyPressed(KEY_ESCAPE)) screen = TITLE;
    } else {
        if (ui::button({card.x + 30, card.y + card.height - 74, 420, 56}, "I agree. It's satire.", ui::GREEN, true, 26)) { eulaAgreed = eulaFingerprint(); saveSettings(); screen = TITLE; }
        if (ui::button({card.x + card.width - 250, card.y + card.height - 74, 220, 56}, "Quit", {214, 204, 186, 255}, true, 26)) wantQuit = true;
    }
}

void pauseMenu() {
    Store& S = G.S;
    DrawRectangle(0, 0, 1600, 900, {30, 20, 16, 170});
    Rectangle box{560, 160, 480, 600};
    ui::panel(box, ui::CREAM, 16, 5, true);
    ui::text("PAUSED", box.x + box.width / 2, box.y + 26, 48, ui::DKRED, 1);
    if (ui::button({box.x + 50, box.y + 110, 380, 62}, "Back to work", ui::RED, true, 26)) { G.paused = false; setMode(G.mode); }
    if (ui::button({box.x + 50, box.y + 190, 380, 54}, S.guides ? "Portion guides: ON" : "Portion guides: OFF", S.guides ? ui::GREEN : Color{214, 204, 186, 255}, true, 22)) S.guides = !S.guides;
    if (ui::button({box.x + 50, box.y + 260, 380, 54}, pz::strongLanguage ? "Strong language: ON" : "Strong language: OFF", pz::strongLanguage ? ui::GREEN : Color{214, 204, 186, 255}, true, 22)) { pz::strongLanguage = !pz::strongLanguage; saveSettings(); }
    if (ui::button({box.x + 50, box.y + 330, 380, 54}, G.hud ? "Helper HUD: ON" : "Helper HUD: OFF (immersive)", G.hud ? ui::GREEN : Color{214, 204, 186, 255}, true, 22)) { G.hud = !G.hud; saveSettings(); }
    if (ui::button({box.x + 50, box.y + 400, 380, 54}, "How to play", ui::YELLOW, true, 22)) G.help = true;
    if (ui::button({box.x + 50, box.y + 470, 380, 54}, "Walk out (lose today)", {214, 204, 186, 255}, true, 22)) { G.paused = false; G.mode = M_WALK; screen = TITLE; }
    ui::text("F11 fullscreen", box.x + box.width / 2, box.y + 548, 18, ui::GREY, 1);
    if (G.help) helpPanel();
}

void play(float dt) {
    Store& S = G.S;
    if (IsKeyPressed(KEY_ESCAPE)) {
        if (G.help) G.help = false;
        else if (G.chart) G.chart = false;
        else if (G.office) { G.office = false; setMode(G.mode); }
        else if (G.paused) { G.paused = false; setMode(G.mode); }
        else if (G.mode != M_WALK) setMode(M_WALK);
        else { G.paused = true; if (!shotMode) EnableCursor(); }
    }
    if (!G.paused) {
        if (IsKeyPressed(KEY_TAB) && G.mode == M_MAKE) G.chart = !G.chart;
        if (IsKeyPressed(KEY_M)) { G.office = !G.office; if (G.office) { if (!shotMode) EnableCursor(); } else setMode(G.mode); }
        // a dead shop skips ahead on its own, so a quiet morning isn't twenty minutes of staring at an empty counter
        bool skipping = S.idle();
        float step = skipping ? dt * 14 : dt;
        for (float left = step; left > 0; left -= 0.2f) S.update(std::min(left, 0.2f));
        G.skipping = skipping;
        events();
        for (auto& t : G.toasts) t.age += dt;
        G.toasts.erase(std::remove_if(G.toasts.begin(), G.toasts.end(), [](const Toast& t) { return t.age > 7; }), G.toasts.end());
    }
    audioUpdate(!G.paused && S.ringing() >= 0);
    worldUpdate(dt);
    EndMode2D();
    posRender();
    signRender();
    ClearBackground({30, 24, 22, 255});
    worldDraw();
    BeginMode2D(Camera2D{ui::offset, {0, 0}, 0, ui::scale});
    bool block = G.paused || G.chart || G.office;
    Vector2 keepM = ui::M; bool kp = ui::pressed, kd = ui::down;
    if (block) { ui::pressed = ui::down = false; ui::M = {-999, -999}; }   // nothing underneath takes clicks
    worldHud();
    if (G.mode == M_REGISTER) registerOverlay(); else if (G.mode == M_MAKE) makeOverlay(); else if (G.mode == M_CUT) cutOverlay();
    topBar();
    if (G.skipping && !G.paused && G.hud) { ui::panel({600, 86, 400, 44}, ui::CREAM, 12, 3, true); ui::text("Quiet... skipping ahead", 800, 94, 24, INK, 1); }
    drawToasts();
    ui::M = keepM; ui::pressed = kp; ui::down = kd;
    if (G.chart && !G.paused) portionChart();
    if (G.office && !G.paused) officeOverlay();
    if (G.paused) pauseMenu();
    if (S.dayOver && !G.paused) { reviewScroll = 0; G.playing = false; G.office = false; verdict = S.closeDay(); save(); screen = DAYEND; sfx(verdict ? SFX_BAD : SFX_DOOR); if (!shotMode) EnableCursor(); }
}
}  // namespace

int main(int argc, char** argv) {
    const char* shot = nullptr, *at = nullptr; int frames = 40, station = -1, hireN = 0; float warm = 0, care = 1; bool demo = false;
    std::string show;
    seed = (uint64_t)time(nullptr);
    for (int i = 1; i < argc; i++) {
        auto arg = [&](const char* n) { return !strcmp(argv[i], n) && i + 1 < argc; };
        if (arg("--shot")) shot = argv[++i];
        else if (arg("--frames")) frames = atoi(argv[++i]);
        else if (arg("--station")) station = atoi(argv[++i]);
        else if (arg("--warm")) warm = (float)atof(argv[++i]);
        else if (arg("--care")) care = (float)atof(argv[++i]);
        else if (arg("--day")) G.S.day = atoi(argv[++i]);
        else if (arg("--seed")) seed = (uint64_t)atoll(argv[++i]);
        else if (arg("--screen")) show = argv[++i];
        else if (arg("--at")) at = argv[++i];
        else if (arg("--shift")) shiftArg = (int)(atof(argv[++i]) * 60);
        else if (arg("--hire")) hireN = atoi(argv[++i]);
        else if (arg("--script")) ui::script(argv[++i]);
        else if (!strcmp(argv[i], "--demo")) demo = true;
        else if (!strcmp(argv[i], "--noguides")) G.S.guides = false;
    }
    shotMode = shot != nullptr;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(shotMode ? 1600 : 1280, shotMode ? 900 : 720, "Makeline");
    SetWindowMinSize(800, 450);
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
    if (!shotMode) { MaximizeWindow(); audioInit(); }
    ui::loadFonts();
    g3::init();
    {
        int n = 0; const unsigned char* d = logoJpg(&n);
        Image im = LoadImageFromMemory(".jpg", d, n);
        logo = LoadTextureFromImage(im); UnloadImage(im);
        SetTextureFilter(logo, TEXTURE_FILTER_BILINEAR);
    }
    {
        Rng r(5);
        Spec s; s.size = XLARGE;
        s.items.push_back({PEPPERONI, WHOLE, NORMAL}); s.items.push_back({MUSHROOM, WHOLE, NORMAL}); s.items.push_back({PEPPER, WHOLE, NORMAL}); s.items.push_back({OLIVE, WHOLE, NORMAL});
        titlePizza = perfectPizza(s, r);
    }
    loadSettings();
    load();
    update::cleanup();
    if (!shotMode) update::start();
    if (shotMode) {
        screen = TITLE;
        if (show == "eula") { screen = EULA; eulaFromTitle = false; }
        else if (show != "title") {
            startShift();
            Rng br(3);
            for (float t = 0; t < warm; t += 1 / 30.f) { G.S.update(1 / 30.f); botStep(G.S, 1 / 30.f, care, br); }
            G.S.events.clear();
            if (demo) {   // something on every station to look at
                Store& S = G.S;
                auto open = S.openTickets();
                if (S.bench < 0 && !open.empty()) {
                    Ticket* k = S.ticket(open[0]);
                    S.startPizza(k->id, k->spec.size, k->spec.crust);
                    Pizza made = perfectPizza(k->spec, br), *p = S.pizza(S.bench);
                    p->stamps = made.stamps; p->bits = made.bits;
                    for (int i = 0; i < SAUCE_N; i++) p->sauceOz[i] = made.sauceOz[i];
                    G.tool = PEPPERONI;
                }
                if (S.cutBoard < 0 && !S.exitQ.empty()) { S.toCutBoard(S.exitQ[0]); S.pizza(S.cutBoard)->cuts = {{0, 0}, {1.5708f, 0.02f}}; }
                if (!S.talk.active()) { if (S.firstInLine() >= 0) S.beginTalk(S.firstInLine()); else if (S.ringing() >= 0) S.beginTalk(S.ringing()); }
                for (int i = 0; i < 240; i++) S.update(1 / 30.f);
                S.events.clear();
            }
            if (station >= 0) setMode(station);
            if (at) sscanf(at, "%f,%f,%f,%f", &G.px, &G.pz, &G.yaw, &G.pitch);
            if (show == "chart") G.chart = true;
            if (show == "hud") G.hud = true;
            if (show.rfind("office", 0) == 0) { G.office = true; G.officeTab = show.size() > 6 ? show[6] - '0' : 0; }
            if (hireN) { Store& S = G.S; S.applicants.clear(); S.newApplicants(); for (int i = 0; i < hireN && !S.applicants.empty(); i++) S.hire(0); }
            if (show == "pause") G.paused = true;
            if (show == "help") G.help = G.paused = true;
            if (show == "dayend") { while (!G.S.dayOver) { G.S.update(1 / 30.f); botStep(G.S, 1 / 30.f, care, br); } G.S.events.clear(); verdict = G.S.closeDay(); screen = DAYEND; }
        }
    }

    int frame = 0;
    while (!WindowShouldClose() && !wantQuit) {
        float dt = shotMode ? 1 / 60.f : std::min(GetFrameTime(), 0.05f);
        if (IsKeyPressed(KEY_F11)) ToggleBorderlessWindowed();
        ui::beginFrame();
        if (G.office) {   // pictures of the crew and the applicants, made before anything else is drawn
            for (auto& w : G.S.staff) g3::portraitPrepare(w.who, true);
            for (auto& w : G.S.applicants) g3::portraitPrepare(w.who, false);
        }
        BeginDrawing();
        ClearBackground({16, 16, 18, 255});
        BeginMode2D(Camera2D{ui::offset, {0, 0}, 0, ui::scale});
        if (screen == SPLASH) {
            splashT += dt;
            float a = std::min({1.f, splashT / 0.5f, (2.6f - splashT) / 0.5f});
            float w = 900, h = w * logo.height / std::max(1, logo.width);
            DrawTexturePro(logo, {0, 0, (float)logo.width, (float)logo.height}, {800 - w / 2, 450 - h / 2, w, h}, {0, 0}, 0, Fade(WHITE, std::max(0.f, a)));
            if (splashT > 2.6f || ui::pressed || GetKeyPressed()) { screen = (eulaAgreed == eulaFingerprint()) ? TITLE : EULA; eulaFromTitle = false; eulaScroll = 0; }
        } else if (screen == EULA) eulaScreen();
        else if (screen == TITLE) titleScreen();
        else if (screen == PLAY) play(dt);
        else { dayEnd(); if (G.office) { if (IsKeyPressed(KEY_ESCAPE)) G.office = false; else officeOverlay(); } }
        EndMode2D();
        EndDrawing();
        if (shotMode && ++frame >= frames) {
            Image im = LoadImageFromScreen();
            ExportImage(im, shot);
            UnloadImage(im);
            break;
        }
    }
    if (screen == PLAY || screen == DAYEND) { /* a shift walked out of isn't kept */ }
    CloseWindow();
    return 0;
}
