// Makeline - the shop simulation.
#include "store.hpp"
#include "json.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace pz {

Customer* Store::cust(int id) { for (auto& c : custs) if (c.id == id) return &c; return nullptr; }
Order* Store::order(int num) { for (auto& o : orders) if (o.num == num) return &o; return nullptr; }
Ticket* Store::ticket(int id) { for (auto& k : tickets) if (k.id == id) return &k; return nullptr; }
Pizza* Store::pizza(int id) { return id >= 0 && id < (int)pizzas.size() ? &pizzas[id] : nullptr; }
const Order* Store::orderOf(const Pizza& p) const {
    for (auto& k : tickets) if (k.id == p.ticket) for (auto& o : orders) if (o.num == k.order) return &o;
    return nullptr;
}

void Store::startDay(uint64_t seed) {
    rng = Rng(seed + day * 7919);
    t = 0; closed = dayOver = false;
    custs.clear(); orders.clear(); tickets.clear(); pizzas.clear();
    talk = Talk{};
    bench = cutBoard = -1;
    oven.clear(); exitQ.clear();
    driversFree = 2; driversBack.clear();
    nextArrival = 3;
    events.clear(); dayReviews.clear();
    dayMoney = dayTips = 0; dayOrders = dayLost = 0;
    nextCust = nextOrder = nextTicket = 1;
}

float Store::rating() const {
    if (reviews.empty()) return 0;
    size_t n = std::min<size_t>(30, reviews.size());
    float s = 0;
    for (size_t i = reviews.size() - n; i < reviews.size(); i++) s += reviews[i].stars;
    return s / n;
}
int Store::reprimandsThisWeek() const {
    int n = 0;
    for (int d : reprimands) if (d > day - 7 && d <= day) n++;
    return n;
}
int Store::closeDay() {
    if (dayReviews.empty() || rating() >= REPRIMAND_BELOW) return 0;
    if (reprimands.empty() || reprimands.back() != day) reprimands.push_back(day);
    if (reprimandsThisWeek() >= 3) { fired = true; return 2; }
    return 1;
}
std::string Store::clock() const {
    int mins = 16 * 60 + (int)(std::min(t, (double)SHIFT_LEN + 240) / SHIFT_LEN * 360);
    int h = mins / 60, m = mins % 60;
    char b[24];
    snprintf(b, sizeof b, "%d:%02d %s", h > 12 ? h - 12 : h, m, h >= 12 ? "PM" : "AM");
    return b;
}

int Store::ringing() const { for (auto& c : custs) if (c.state == C_RINGING) return c.id; return -1; }
std::vector<int> Store::line() const { std::vector<int> v; for (auto& c : custs) if (c.state == C_INLINE && c.leaving <= 0) v.push_back(c.id); return v; }
int Store::firstInLine() const { auto v = line(); return v.empty() ? -1 : v[0]; }
std::vector<int> Store::lobby() const { std::vector<int> v; for (auto& c : custs) if (c.state == C_WAITING && c.leaving <= 0) v.push_back(c.id); return v; }
std::vector<int> Store::openTickets() const { std::vector<int> v; for (auto& k : tickets) if (k.state == T_WAIT) v.push_back(k.id); return v; }

void Store::spawn() {
    Customer c;
    c.id = nextCust++;
    c.who = makePerson(rng);
    c.want = randomOrder(rng, day);
    bool phoneBusy = ringing() >= 0 || (talk.active() && cust(talk.cust) && cust(talk.cust)->phone);
    bool wantsPhone = rng.chance(0.45f);
    if (wantsPhone && !phoneBusy) {
        c.phone = true;
        c.service = rng.chance(0.55f) ? DELIVERY : CARRYOUT;
        c.state = C_RINGING;
        events.push_back({E_RING, c.id, ""});
    } else {
        if (line().size() >= 4) return;   // sees the line through the window and keeps driving
        c.state = C_INLINE;
        events.push_back({E_DOOR, c.id, ""});
    }
    custs.push_back(c);
}

void Store::say(const std::string& line) { talk.pending.push_back(line); }

bool Store::beginTalk(int id) {
    Customer* c = cust(id);
    if (!c || talk.active() || (c->state != C_RINGING && c->state != C_INLINE)) return false;
    if (c->state == C_INLINE && id != firstInLine()) return false;
    c->state = C_ORDERING; c->timer = 0;
    c->patience = std::max(c->patience, 0.5f);   // being noticed helps. A little.
    talk = Talk{};
    talk.cust = id;
    talk.draft = {Spec{}};
    talk.draftService = c->phone ? CARRYOUT : COUNTER;
    talk.pending = orderScript(rng, c->who, c->want, c->service, c->phone, day);
    return true;
}
void Store::askRepeat() {
    Customer* c = cust(talk.cust);
    if (!c) return;
    if (talk.speaking && !talk.log.empty()) talk.shown = (float)talk.log.back().text.size();
    talk.speaking = false; talk.pending.clear();
    talk.log.push_back({true, "Sorry, could you repeat that?"});
    c->patience -= 0.15f;
    say(recap(rng, c->want, c->service, c->phone));
    talk.pause = 0.4f;
}
void Store::askReadBack() {
    Customer* c = cust(talk.cust);
    if (!c) return;
    if (talk.speaking && !talk.log.empty()) talk.shown = (float)talk.log.back().text.size();
    talk.speaking = false; talk.pending.clear();
    talk.log.push_back({true, "Let me read that back to you..."});
    c->patience -= 0.06f;
    bool ok;
    say(readBack(rng, c->want, talk.draft, c->service, talk.draftService, c->phone, ok));
    talk.pause = 1.2f;
}
bool Store::finishOrder() {
    Customer* c = cust(talk.cust);
    if (!c || talk.draft.empty()) return false;
    Order o;
    o.num = nextOrder++;
    o.cust = c->id;
    o.service = c->phone ? talk.draftService : COUNTER;
    o.entered = talk.draft;
    o.placed = t;
    int n = (int)o.entered.size(), ahead = 0;
    for (auto& k : tickets) if (k.state != T_BOXED && k.state != T_VOID) ahead++;
    float base = o.service == COUNTER ? 85 : o.service == CARRYOUT ? 115 : 150;
    o.budget = base + 45 * (std::max(1, (int)c->want.size()) - 1) + 12 * ahead;
    for (int i = 0; i < n; i++) {
        Ticket k{nextTicket++, o.num, i + 1, n, o.entered[i]};
        o.tickets.push_back(k.id);
        tickets.push_back(k);
        o.price += price(o.entered[i]);
    }
    c->order = o.num;
    c->timer = 0; c->patience = 1;
    if (!c->phone) c->state = C_WAITING;
    else if (c->service == CARRYOUT) { c->state = C_COMING; c->arriveIn = rng.range(45, 75); }
    else c->state = C_AWAY;
    orders.push_back(o);
    dayOrders++;
    events.push_back({E_PLACED, o.num, ""});
    talk = Talk{};
    return true;
}

void Store::review(const Customer& c, const Score& sc, int stars) {
    float p = stars <= 2 ? 1.0f : stars == 5 ? 0.6f : 0.7f;   // angry people always find the time
    if (!rng.chance(p)) return;
    Review r;
    r.name = c.who.name; r.stars = stars; r.day = day;
    r.text = reviewText(rng, sc, stars);
    for (auto& is : sc.issues) r.reasons.push_back(issueLine(is));
    reviews.push_back(r);
    if (reviews.size() > 80) reviews.erase(reviews.begin());
    dayReviews.push_back(r);
    events.push_back({E_REVIEW, stars, r.name + ": " + r.text});
}
void Store::lose(Customer& c, int code) {
    Score sc; sc.add(code, 4);
    c.state = C_DONE; c.leaving = 0.01f; c.leftStars = 0;
    dayLost++;
    if (rng.chance(0.65f)) review(c, sc, 1);
    events.push_back({code == NO_ANSWER ? E_MISSED : E_STORMED, c.id, ""});
}
void Store::resolve(Order& o, std::vector<Issue> extra) {
    if (o.done) return;
    o.done = true;
    Customer* c = cust(o.cust);
    std::vector<const Pizza*> made;
    for (int id : o.tickets) {
        Ticket* k = ticket(id);
        if (!k) continue;
        if (k->state == T_BOXED && k->pizza >= 0) made.push_back(&pizzas[k->pizza]);
        else if (k->state == T_WAIT) k->state = T_VOID;
    }
    OrderScore os;
    if (!c) return;
    if (extra.empty()) os = scoreOrder(c->want, made, c->phone ? c->service : COUNTER, o.service, (float)((t - o.placed) / o.budget));
    else { for (auto& is : extra) os.score.add(is.code, is.pts, is.ing); os.stars = starsFor(os.score.penalty); }
    if (os.stars >= 3) {
        float tip = os.stars == 5 ? o.price * (o.service == DELIVERY ? 0.18f : 0.06f) : os.stars == 4 ? o.price * 0.04f : 0;
        money += o.price + tip; dayMoney += o.price; dayTips += tip;
    }   // two stars or worse: they got their money back
    c->leftStars = os.stars;
    if (c->state == C_WAITING || c->state == C_COMING) c->leaving = 0.01f;
    c->state = C_DONE;
    review(*c, os.score, os.stars);
}

bool Store::handOut(int id) {
    Customer* c = cust(id);
    if (!c || c->state != C_WAITING) return false;
    Order* o = order(c->order);
    if (!o || !o->ready || o->done) return false;
    resolve(*o);
    events.push_back({E_HANDED, c->leftStars, ""});
    return true;
}

bool Store::ovenHasRoom() const { for (auto& s : oven) if (s.prog < 0.14f) return false; return true; }
bool Store::startPizza(int tid, int size, int crust) {
    Ticket* k = ticket(tid);
    if (!k || k->state != T_WAIT || bench >= 0) return false;
    Pizza p;
    p.id = (int)pizzas.size(); p.ticket = tid; p.size = size; p.crust = crust;
    pizzas.push_back(p);
    bench = p.id; k->state = T_MAKING; k->pizza = p.id;
    return true;
}
bool Store::benchToOven() {
    Pizza* p = pizza(bench);
    if (!p || !ovenHasRoom()) return false;
    oven.push_back({p->id, 0});
    if (Ticket* k = ticket(p->ticket)) k->state = T_OVEN;
    bench = -1;
    return true;
}
void Store::trashBench() {
    Pizza* p = pizza(bench);
    if (!p) return;
    if (Ticket* k = ticket(p->ticket)) { if (k->state == T_MAKING) { k->state = T_WAIT; k->pizza = -1; } }
    bench = -1;
}
bool Store::toCutBoard(int id) {
    if (cutBoard >= 0) return false;
    auto it = std::find(exitQ.begin(), exitQ.end(), id);
    if (it == exitQ.end()) return false;
    exitQ.erase(it);
    cutBoard = id;
    if (Ticket* k = ticket(pizzas[id].ticket)) if (k->state == T_EXIT) k->state = T_CUT;
    return true;
}
bool Store::boxCut() {
    Pizza* p = pizza(cutBoard);
    if (!p) return false;
    p->boxed = true;
    cutBoard = -1;
    Ticket* k = ticket(p->ticket);
    if (!k || k->state != T_CUT) return true;
    k->state = T_BOXED;
    Order* o = order(k->order);
    if (o && !o->done) {
        bool all = true;
        for (int id : o->tickets) { Ticket* q = ticket(id); if (q && q->state != T_BOXED) all = false; }
        if (all) { o->ready = true; events.push_back({E_READY, o->num, ""}); }
    }
    return true;
}
void Store::trashCut() {
    Pizza* p = pizza(cutBoard);
    if (!p) return;
    if (Ticket* k = ticket(p->ticket)) { if (k->state == T_CUT) { k->state = T_WAIT; k->pizza = -1; } }
    cutBoard = -1;
}

void Store::update(float dt) {
    if (dayOver) return;
    t += dt;
    if (t >= SHIFT_LEN) closed = true;
    // ---- people arriving: slow at open, a dinner rush in the middle
    if (!closed && t < SHIFT_LEN - 25 && t >= nextArrival) {
        spawn();
        float mean = std::max(27.f, 48.f - 4.f * (day - 1));
        float u = (float)(t / SHIFT_LEN), rush = 1.0f - 0.4f * std::exp(-(u - 0.55f) * (u - 0.55f) / 0.04f);
        nextArrival = t + mean * rush * rng.range(0.55f, 1.45f);
    }
    // ---- the conversation
    if (talk.active()) {
        Customer* c = cust(talk.cust);
        float speed = c && c->who.mood == 0 ? 40.f : c && c->who.mood == 1 ? 26.f : 32.f;
        if (talk.speaking) {
            float before = talk.shown;
            talk.shown += speed * dt;
            if ((int)(talk.shown / 3) != (int)(before / 3)) events.push_back({E_TALK, 0, ""});
            if (talk.shown >= (float)talk.log.back().text.size()) { talk.speaking = false; talk.pause = 0.65f; }
        } else if (!talk.pending.empty()) {
            talk.pause -= dt;
            if (talk.pause <= 0) {
                talk.log.push_back({false, talk.pending.front()});
                talk.pending.erase(talk.pending.begin());
                talk.shown = 0; talk.speaking = true;
            }
        }
    }
    // ---- everyone's patience
    for (auto& c : custs) {
        c.timer += dt;
        if (c.leaving > 0) { c.leaving += dt; continue; }
        switch (c.state) {
            case C_RINGING:
                c.patience = 1 - c.timer / 20;
                if (c.timer > 20 || closed) lose(c, NO_ANSWER);
                break;
            case C_INLINE:
                c.patience -= dt / 60;
                if (c.patience <= 0) lose(c, RUDE_SERVICE);
                break;
            case C_ORDERING:
                c.patience -= dt / 130;
                if (c.patience <= 0) { talk = Talk{}; lose(c, RUDE_SERVICE); }
                break;
            case C_COMING:
                c.arriveIn -= dt;
                if (c.arriveIn <= 0) { c.state = C_WAITING; c.timer = 0; events.push_back({E_DOOR, c.id, ""}); }
                break;
            default: break;
        }
        if (c.state == C_WAITING || c.state == C_INLINE) {
            if (Order* o = order(c.order)) c.patience = std::clamp(1 - (float)((t - o->placed) / o->budget), 0.f, 1.f);
            c.barkT -= dt; c.nextBark -= dt;
            if (c.nextBark <= 0) {
                c.nextBark = rng.range(7, 16) * (0.5f + c.patience);
                if (c.patience < 0.75f) { c.bark = waitingBark(rng, c.patience); c.barkT = 3.5f; }
            }
        }
    }
    // ---- the oven
    for (auto& s : oven) s.prog += dt / OVEN_TIME;
    while (!oven.empty() && oven.front().prog >= 1) {
        int id = oven.front().pizza;
        oven.erase(oven.begin());
        pizzas[id].baked = 1;
        exitQ.push_back(id);
        if (Ticket* k = ticket(pizzas[id].ticket)) if (k->state == T_OVEN) k->state = T_EXIT;
        events.push_back({E_OVEN_OUT, id, ""});
    }
    for (auto& p : pizzas) {
        if (p.baked < 1) continue;
        const Order* o = orderOf(p);
        if (o && o->done) continue;
        p.cool += dt * (p.boxed ? 0.3f : 1.0f);
    }
    // ---- drivers and the orders that went wrong on the phone
    for (auto& b : driversBack) b -= dt;
    for (size_t i = driversBack.size(); i-- > 0;) if (driversBack[i] <= 0) { driversBack.erase(driversBack.begin() + i); driversFree++; }
    for (auto& o : orders) {
        if (o.done) continue;
        Customer* c = cust(o.cust);
        if (!c) continue;
        if (o.service == DELIVERY) {
            if (o.ready && !o.out && driversFree > 0) { o.out = true; o.tripLeft = 22; driversFree--; driversBack.push_back(44); events.push_back({E_DRIVER, o.num, ""}); }
            if (o.out) {
                o.tripLeft -= dt;
                if (o.tripLeft <= 0) { resolve(o); events.push_back({E_DELIVERED, c->leftStars, ""}); }
            }
            // they said pickup: they turn up at the counter and there's nothing for them
            if (!o.done && c->service == CARRYOUT && c->state == C_WAITING && c->timer > 6) resolve(o, {{WRONG_SERVICE, -1, 4}});
        } else if (o.service == CARRYOUT && c->service == DELIVERY) {
            // they said delivery: nobody ever comes for it
            if (t - o.placed > o.budget * 1.3f) resolve(o, {{WRONG_SERVICE, -1, 4}});
        }
    }
    // ---- closing time
    if (closed) {
        bool busy = talk.active();
        for (auto& c : custs) if (c.state != C_DONE) busy = true;
        if (!busy || t > SHIFT_LEN + 240) {
            for (auto& o : orders) if (!o.done) resolve(o, {{NEVER_SERVED, -1, 4}});
            for (auto& c : custs) if (c.state != C_DONE) { c.state = C_DONE; }
            talk = Talk{};
            dayOver = true;
        }
    }
}

std::string Store::toJson() const {
    nlohmann::json j;
    j["makelineSave"] = 1;
    j["day"] = day; j["money"] = money; j["guides"] = guides;
    auto arr = nlohmann::json::array();
    for (auto& r : reviews) arr.push_back({{"name", r.name}, {"text", r.text}, {"stars", r.stars}, {"day", r.day}});
    j["reviews"] = arr;
    j["reprimands"] = reprimands;
    j["fired"] = fired;
    return j.dump(1);
}
bool Store::fromJson(const std::string& text) {
    nlohmann::json j = nlohmann::json::parse(text, nullptr, false);
    if (!j.is_object() || !j.contains("makelineSave")) return false;
    day = std::max(1, j.value("day", 1));
    money = j.value("money", 0.f);
    guides = j.value("guides", true);
    fired = j.value("fired", false);
    reprimands.clear();
    if (j.contains("reprimands") && j["reprimands"].is_array()) for (auto& d : j["reprimands"]) if (d.is_number_integer()) reprimands.push_back(d.get<int>());
    reviews.clear();
    if (j.contains("reviews") && j["reviews"].is_array())
        for (auto& r : j["reviews"]) if (r.is_object()) {
            Review v; v.name = r.value("name", ""); v.text = r.value("text", ""); v.stars = std::clamp(r.value("stars", 5), 1, 5); v.day = r.value("day", 1);
            reviews.push_back(v);
        }
    return true;
}

void botStep(Store& s, float dt, float care, Rng& rng) {
    static float busy = 0;
    (void)dt;
    if (s.t < 0.1) busy = 0;
    if (s.t < busy) return;
    for (int id : s.lobby()) if (s.handOut(id)) { busy = (float)s.t + 2; return; }
    if (s.talk.active()) {
        Customer* c = s.cust(s.talk.cust);
        if (!s.talk.pending.empty() || s.talk.speaking || !c) return;
        s.talk.draft = c->want;
        s.talk.draftService = c->service;
        if (!rng.chance(care)) s.talk.draft[0].size = (s.talk.draft[0].size + 1) % SIZE_N;
        s.finishOrder();
        busy = (float)s.t + 2;
        return;
    }
    if (s.ringing() >= 0) { s.beginTalk(s.ringing()); return; }
    if (s.firstInLine() >= 0 && (s.openTickets().empty() || s.cust(s.firstInLine())->patience < 0.45f)) { s.beginTalk(s.firstInLine()); return; }
    if (s.cutBoard < 0 && !s.exitQ.empty()) {
        s.toCutBoard(s.exitQ[0]);
        Pizza* p = s.pizza(s.cutBoard);
        if (rng.chance(care)) p->cuts = perfectPizza(Spec{p->size, p->crust}, rng).cuts;
        s.boxCut();
        busy = (float)s.t + 6;
        return;
    }
    auto open = s.openTickets();
    if (s.bench < 0 && !open.empty() && s.ovenHasRoom()) {
        Ticket* k = s.ticket(open[0]);
        s.startPizza(k->id, k->spec.size, k->spec.crust);
        Pizza* p = s.pizza(s.bench);
        Pizza made = perfectPizza(k->spec, rng);
        p->stamps = made.stamps; p->bits = made.bits;
        for (int i = 0; i < SAUCE_N; i++) p->sauceOz[i] = made.sauceOz[i];
        if (!rng.chance(care)) p->bits.resize(p->bits.size() / 3);   // rushed it
        s.benchToOven();
        busy = (float)s.t + 14;
        return;
    }
    if (s.firstInLine() >= 0) s.beginTalk(s.firstInLine());
}

}  // namespace pz
