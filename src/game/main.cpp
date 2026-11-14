// Makeline - a pizza shop game by OAB Foundry. Startup, the main loop, the title and end-of-day screens.
#include "game.hpp"
#include "updater.hpp"
#include "version.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

using namespace pz;
using ui::INK;

Game G;
extern const unsigned char* logoJpg(int* size);   // ui.cpp holds the embedded files

namespace {
enum Screen { SPLASH, TITLE, PLAY, DAYEND };
int screen = SPLASH;
float splashT = 0, reviewScroll = 0;
bool confirmNew = false, wantQuit = false, shotMode = false;
uint64_t seed = 1;
Texture2D logo{};
Pizza titlePizza;

std::string savePath() { return std::string(GetApplicationDirectory()) + "makeline_save.json"; }
void save() { if (!shotMode) SaveFileText(savePath().c_str(), (char*)G.S.toJson().c_str()); }
void load() {
    if (shotMode || !FileExists(savePath().c_str())) return;
    char* t = LoadFileText(savePath().c_str());
    if (t) { G.S.fromJson(t); UnloadFileText(t); }
}
void startShift() {
    G.S.startDay(seed);
    G.station = ST_FRONT; G.tool = -1; G.pickTicket = -1; G.toasts.clear(); G.paused = G.chart = G.help = false;
    screen = PLAY;
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
            case E_DRIVER: snprintf(b, sizeof b, "A driver left with order #%d.", e.a); toast(b); break;
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
    DrawRectangle(0, 0, 1600, 64, INK);
    int ready = 0;
    for (int id : S.lobby()) { Customer* c = S.cust(id); Order* o = c ? S.order(c->order) : nullptr; if (o && o->ready) ready++; }
    int alerts[3] = {(int)S.line().size() + ready + (S.ringing() >= 0 ? 1 : 0), (int)S.openTickets().size(), (int)S.exitQ.size()};
    static const char* NAMES[] = {"1  FRONT", "2  MAKELINE", "3  OVEN & CUT"};
    for (int i = 0; i < 3; i++) {
        Rectangle r{14 + i * 232.f, 8, 220, 44};
        bool urgent = i == 0 && (S.ringing() >= 0 || ready > 0);
        Color fill = G.station == i ? ui::CREAM : Color{96, 78, 68, 255};
        if (G.station != i && urgent && std::fmod(ui::time, 0.6f) < 0.3f) fill = ui::RED;
        if (ui::button(r, NAMES[i], fill, true, 22)) { G.station = i; sfx(SFX_CLICK); }
        if (alerts[i] > 0) {
            DrawCircleV({r.x + r.width - 8, r.y + 4}, 15, INK);
            DrawCircleV({r.x + r.width - 8, r.y + 4}, 12, urgent ? ui::RED : ui::YELLOW);
            ui::text(std::to_string(alerts[i]), r.x + r.width - 8, r.y - 6, 19, urgent ? ui::PAPER : INK, 1);
        }
    }
    char b[64];
    snprintf(b, sizeof b, "Day %d", S.day);
    ui::text(b, 760, 18, 26, ui::CREAM);
    ui::text(S.closed ? "CLOSED - finish up" : S.clock(), 860, 18, 26, S.closed ? ui::RED : ui::YELLOW);
    float rt = S.rating();
    ui::stars(1150, 18, 28, rt, {110, 92, 82, 255});
    if (rt > 0) snprintf(b, sizeof b, "%.1f", rt); else snprintf(b, sizeof b, "new");
    ui::text(b, 1308, 18, 26, ui::CREAM);
    snprintf(b, sizeof b, "$%.2f", S.money);
    ui::text(b, 1586, 18, 26, {170, 230, 170, 255}, 2);
}

void drawToasts() {
    float y = 76;
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
    ClearBackground(ui::RED);
    for (int i = 0; i < 20; i++) for (int j = 0; j < 12; j++) if ((i + j) & 1) DrawRectangle(i * 80, j * 80, 80, 80, {204, 58, 44, 255});
    // a pizza the size of the screen, slowly turning
    Pizza p = titlePizza;
    float a = ui::time * 0.05f, cs = std::cos(a), sn = std::sin(a);
    for (auto& b : p.bits) { float x = b.x * cs - b.y * sn, y = b.x * sn + b.y * cs; b.x = x; b.y = y; b.rot += a; }
    drawPizza(p, {1180, 470}, 380);
    Rectangle card{90, 110, 620, 680};
    ui::panel(card, ui::CREAM, 18, 5, true);
    ui::text("MAKELINE", card.x + 40, card.y + 30, 96, ui::DKRED);
    ui::text("a pizza shop game", card.x + 46, card.y + 126, 28, INK);
    float rt = S.rating();
    char b[96];
    if (rt > 0) {
        ui::stars(card.x + 46, card.y + 180, 34, rt);
        snprintf(b, sizeof b, "%.1f  (%d reviews)", rt, (int)S.reviews.size());
        ui::text(b, card.x + 240, card.y + 184, 24, INK);
    } else ui::text("No reviews yet. That won't last.", card.x + 46, card.y + 184, 22, ui::GREY);
    snprintf(b, sizeof b, S.day > 1 || !S.reviews.empty() ? "CLOCK IN: DAY %d" : "CLOCK IN", S.day);
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
    ui::text(std::string("Version ") + version::text(), card.x + 46, card.y + 622, 18, ui::GREY);
    ui::text("OAB Foundry", card.x + 574, card.y + 622, 18, ui::GREY, 2);
    if (G.help) helpPanel();
}

void dayEnd() {
    Store& S = G.S;
    ClearBackground({58, 44, 40, 255});
    Rectangle card{100, 60, 560, 780};
    ui::panel(card, ui::CREAM, 16, 5, true);
    char b[128];
    snprintf(b, sizeof b, "DAY %d", S.day);
    ui::text(b, card.x + 36, card.y + 24, 64, ui::DKRED);
    ui::text("The lights are off. Here's how it went.", card.x + 38, card.y + 96, 21, ui::GREY);
    float y = card.y + 150;
    auto stat = [&](const char* k, const std::string& v, Color c = INK) { ui::text(k, card.x + 40, y, 26, INK); ui::text(v, card.x + card.width - 40, y, 26, c, 2); y += 44; };
    stat("Orders taken", std::to_string(S.dayOrders));
    stat("Walked out or hung up", std::to_string(S.dayLost), S.dayLost ? ui::DKRED : INK);
    snprintf(b, sizeof b, "$%.2f", S.dayMoney); stat("Sales (after refunds)", b);
    snprintf(b, sizeof b, "$%.2f", S.dayTips); stat("Tips", b);
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
    const char* verdict = avg == 0 ? "Nobody said anything. Take it." : avg >= 4.5f ? "Corporate would like a word. A nice one, for once." : avg >= 3.5f ? "A decent night. The regulars will be back." :
        avg >= 2.5f ? "Rough. People talk, you know." : "A disaster. The franchise owner is 'concerned'.";
    y += ui::wrap(verdict, card.x + 40, y, card.width - 80, 22, ui::DKRED) + 10;
    if (S.day == 1 && S.guides) ui::wrap("From tomorrow the portion guides come off. Learn the chart. (You can turn them back on from the pause menu.)", card.x + 40, y, card.width - 80, 19, {90, 74, 64, 255});
    if (ui::button({card.x + 40, card.y + card.height - 100, 300, 70}, "NEXT DAY", ui::RED, true, 30)) {
        if (S.day == 1) S.guides = false;
        S.day++; save(); sfx(SFX_DING); startShift();
    }
    if (ui::button({card.x + 356, card.y + card.height - 100, 164, 70}, "Clock out", {214, 204, 186, 255}, true, 22)) {
        if (S.day == 1) S.guides = false;
        S.day++; save(); screen = TITLE;
    }
    // the reviews, newest first
    Rectangle list{700, 60, 800, 780};
    ui::panel(list, {246, 240, 228, 255}, 16, 5, true);
    ui::text("WHAT THEY WROTE", list.x + 30, list.y + 20, 30, INK);
    reviewScroll = std::max(0.f, reviewScroll - GetMouseWheelMove() * 60);
    float ry = list.y + 76 - reviewScroll, bottom = list.y + list.height - 16;
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

void pauseMenu() {
    Store& S = G.S;
    DrawRectangle(0, 0, 1600, 900, {30, 20, 16, 170});
    Rectangle box{560, 220, 480, 470};
    ui::panel(box, ui::CREAM, 16, 5, true);
    ui::text("PAUSED", box.x + box.width / 2, box.y + 26, 48, ui::DKRED, 1);
    if (ui::button({box.x + 50, box.y + 110, 380, 62}, "Back to work", ui::RED, true, 26)) G.paused = false;
    if (ui::button({box.x + 50, box.y + 190, 380, 54}, S.guides ? "Portion guides: ON" : "Portion guides: OFF", S.guides ? ui::GREEN : Color{214, 204, 186, 255}, true, 22)) S.guides = !S.guides;
    if (ui::button({box.x + 50, box.y + 260, 380, 54}, "How to play", ui::YELLOW, true, 22)) G.help = true;
    if (ui::button({box.x + 50, box.y + 330, 380, 54}, "Walk out (lose today)", {214, 204, 186, 255}, true, 22)) { G.paused = false; screen = TITLE; }
    ui::text("F11 fullscreen", box.x + box.width / 2, box.y + 414, 18, ui::GREY, 1);
    if (G.help) helpPanel();
}

void play(float dt) {
    Store& S = G.S;
    if (IsKeyPressed(KEY_ESCAPE)) { if (G.help) G.help = false; else if (G.chart) G.chart = false; else G.paused = !G.paused; }
    if (!G.paused) {
        if (IsKeyPressed(KEY_ONE)) G.station = ST_FRONT;
        if (IsKeyPressed(KEY_TWO)) G.station = ST_MAKE;
        if (IsKeyPressed(KEY_THREE)) G.station = ST_OVEN;
        if (IsKeyPressed(KEY_TAB) && G.station == ST_MAKE && !G.chart) G.chart = true;
        S.update(dt);
        events();
        for (auto& t : G.toasts) t.age += dt;
        G.toasts.erase(std::remove_if(G.toasts.begin(), G.toasts.end(), [](const Toast& t) { return t.age > 7; }), G.toasts.end());
    }
    audioUpdate(!G.paused && S.ringing() >= 0);
    bool block = G.paused || G.chart;
    Vector2 keepM = ui::M; bool kp = ui::pressed, kd = ui::down;
    if (block) { ui::pressed = ui::down = false; ui::M = {-999, -999}; }   // nothing underneath takes clicks
    if (G.station == ST_FRONT) frontStation(); else if (G.station == ST_MAKE) makeStation(); else ovenStation();
    topBar();
    drawToasts();
    ui::M = keepM; ui::pressed = kp; ui::down = kd;
    if (G.chart && !G.paused) portionChart();
    if (G.paused) pauseMenu();
    if (S.dayOver && !G.paused) { reviewScroll = 0; screen = DAYEND; sfx(SFX_DOOR); }
}
}  // namespace

int main(int argc, char** argv) {
    const char* shot = nullptr; int frames = 40, station = -1; float warm = 0, care = 1; bool demo = false;
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
    RenderTexture2D canvas = LoadRenderTexture((int)ui::W, (int)ui::H);
    SetTextureFilter(canvas.texture, TEXTURE_FILTER_BILINEAR);
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
    load();
    update::cleanup();
    if (!shotMode) update::start();
    if (shotMode) {
        screen = TITLE;
        if (show != "title") {
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
            if (station >= 0) G.station = station;
            if (show == "chart") G.chart = true;
            if (show == "pause") G.paused = true;
            if (show == "help") G.help = G.paused = true;
            if (show == "dayend") { while (!G.S.dayOver) { G.S.update(1 / 30.f); botStep(G.S, 1 / 30.f, care, br); } G.S.events.clear(); screen = DAYEND; }
        }
    }

    int frame = 0;
    while (!WindowShouldClose() && !wantQuit) {
        float dt = shotMode ? 1 / 60.f : std::min(GetFrameTime(), 0.05f);
        if (IsKeyPressed(KEY_F11)) ToggleBorderlessWindowed();
        ui::beginFrame();
        BeginTextureMode(canvas);
        ClearBackground(ui::CREAM);
        if (screen == SPLASH) {
            splashT += dt;
            ClearBackground({16, 16, 18, 255});
            float a = std::min({1.f, splashT / 0.5f, (2.6f - splashT) / 0.5f});
            float w = 900, h = w * logo.height / std::max(1, logo.width);
            DrawTexturePro(logo, {0, 0, (float)logo.width, (float)logo.height}, {800 - w / 2, 450 - h / 2, w, h}, {0, 0}, 0, Fade(WHITE, std::max(0.f, a)));
            if (splashT > 2.6f || ui::pressed || GetKeyPressed()) screen = TITLE;
        } else if (screen == TITLE) titleScreen();
        else if (screen == PLAY) play(dt);
        else dayEnd();
        EndTextureMode();
        BeginDrawing();
        ClearBackground(BLACK);
        float sc = std::min(GetScreenWidth() / ui::W, GetScreenHeight() / ui::H);
        DrawTexturePro(canvas.texture, {0, 0, ui::W, -ui::H}, {(GetScreenWidth() - ui::W * sc) / 2, (GetScreenHeight() - ui::H * sc) / 2, ui::W * sc, ui::H * sc}, {0, 0}, 0, WHITE);
        EndDrawing();
        if (shotMode && ++frame >= frames) {
            Image im = LoadImageFromTexture(canvas.texture);
            ImageFlipVertical(&im);
            ExportImage(im, shot);
            UnloadImage(im);
            break;
        }
    }
    if (screen == PLAY || screen == DAYEND) { /* a shift walked out of isn't kept */ }
    CloseWindow();
    return 0;
}
