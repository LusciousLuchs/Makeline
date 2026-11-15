// Makeline - the shop simulation.
#include "store.hpp"
#include "json.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace pz {

Ctx Store::voice(const Customer& c) const {
    Ctx x;
    x.who = &c.who; x.anger = std::clamp(1 - c.patience, 0.f, 1.f); x.service = c.service; x.phone = c.phone; x.asks = c.asks;
    x.hour = ((int)(clockMin() / 60)) % 24;
    for (auto& o : orders) if (o.num == c.order) x.minutes = o.minutes(t);
    return x;
}
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
    Rng mapRng(seed * 31 + day);
    roads.generate(mapRng);
    dayNotes.clear(); dayWages = 0;
    if (!crewSet) {   // the store comes with one driver
        crewSet = true;
        Rng r(hireSeed);
        Worker w; w.who = makePerson(r); w.who.age = 19 + r.below(30); w.role = DRIVER; w.skill = 0.6f; w.wage = 50;
        staff.push_back(w);
    }
    if (applicants.empty()) newApplicants();
    for (auto& w : staff) { w.tips = 0; w.mistakes = w.yelled = w.jobs = 0; w.wroteUpToday = false; w.busy = 0; w.cust = w.ticket = -1; w.out = false; w.stops.clear(); w.route.clear(); }
    nextArrival = 3;
    events.clear(); dayReviews.clear();
    dayMoney = dayTips = 0; dayOrders = dayLost = 0;
    nextCust = nextOrder = nextTicket = 1;
    shiftStart = forceStart >= 0 ? forceStart : shiftFor(day);
    nextArrival = rng.range(6, 25);
    if (shiftStart <= OPEN_MIN) events.push_back({E_NOTE, 0, "You're opening today. The doors unlock at 9."});
    else if (shiftEnd() >= CLOSE_MIN) events.push_back({E_NOTE, 0, "You're closing tonight."});
}
int Store::shiftFor(int d) const {
    Rng r(schedSeed * 2654435761ull + (uint64_t)d * 40503ull);
    return OPEN_MIN + 30 * r.below((CLOSE_MIN - SHIFT_MINS - OPEN_MIN) / 30 + 1);
}
std::string Store::timeText(int m) {
    m %= 1440;
    int h = m / 60, mm = m % 60;
    char b[24];
    snprintf(b, sizeof b, "%d:%02d %s", h % 12 == 0 ? 12 : h % 12, mm, h >= 12 ? "PM" : "AM");
    return b;
}
bool Store::idle() const {
    if (dayOver || closed || talk.active()) return false;
    for (auto& c : custs) if (c.state != C_DONE) return false;
    for (auto& o : orders) if (!o.done) return false;
    for (auto& k : tickets) if (k.state != T_BOXED && k.state != T_VOID) return false;
    return oven.empty() && exitQ.empty() && bench < 0 && cutBoard < 0;
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
const char* roleName(int role) { static const char* N[] = {"Customer service", "Makeline", "Driver"}; return N[role < 0 ? 0 : role > 2 ? 2 : role]; }
void Store::note(const std::string& s) { dayNotes.push_back(s); events.push_back({E_NOTE, 0, s}); }
int Store::driversIn() const { int n = 0; for (auto& w : staff) if (w.role == DRIVER && !w.out) n++; return n; }

void Store::newApplicants() {
    Rng r(hireSeed += 77);
    applicants.clear();
    for (int i = 0; i < 5; i++) {
        Worker w;
        w.who = makePerson(r);
        w.role = i < 3 ? i : r.below(ROLE_N);
        w.who.age = w.role == DRIVER ? 18 + r.below(45) : 16 + (int)(r.f() * r.f() * 50);
        w.skill = r.range(0.3f, 0.92f);
        w.mood = r.range(0.55f, 0.85f);
        float base = w.role == CSR ? 55 : w.role == COOK ? 65 : 45;
        w.wage = (float)(int)(base * (0.7f + 0.7f * w.skill));   // the good ones know what they're worth
        applicants.push_back(w);
    }
}
bool Store::hire(int i) {
    if (i < 0 || i >= (int)applicants.size() || staff.size() >= 8) return false;
    staff.push_back(applicants[i]);
    applicants.erase(applicants.begin() + i);
    return true;
}
void Store::fireWorker(int i) {
    if (i < 0 || i >= (int)staff.size()) return;
    Worker& w = staff[i];
    // whatever they were in the middle of goes back
    if (Customer* c = cust(w.cust)) { if (c->state == C_ORDERING && c->server == i) { c->state = c->phone ? C_RINGING : C_INLINE; c->server = -1; c->timer = 0; } }
    if (Ticket* k = ticket(w.ticket)) { if (k->state == T_MAKING) { k->state = T_WAIT; k->madeBy = -1; } }
    if (w.out) return;   // (they finish the run first; fire them when they're back)
    staff.erase(staff.begin() + i);
    for (auto& c : custs) { if (c.server == i) c.server = -1; else if (c.server > i) c.server--; }
    for (auto& o : orders) { if (o.takenBy == i) o.takenBy = -1; else if (o.takenBy > i) o.takenBy--; if (o.driver == i) o.driver = -1; else if (o.driver > i) o.driver--; }
    for (auto& k : tickets) { if (k.madeBy == i) k.madeBy = -1; else if (k.madeBy > i) k.madeBy--; }
}
void Store::writeUp(int i) {
    if (i < 0 || i >= (int)staff.size() || staff[i].wroteUpToday) return;
    Worker& w = staff[i];
    w.wroteUpToday = true; w.writeUps++;
    w.careDays = 2;
    w.mood -= w.mistakes > 0 ? 0.2f : 0.38f;   // being written up for nothing stings twice as much
    note(w.mistakes > 0 ? "You wrote " + w.who.name + " up." : "You wrote " + w.who.name + " up for nothing. They noticed.");
}

void Store::sendDriver(int wi) {
    Worker& w = staff[wi];
    std::vector<Order*> load;
    for (auto& o : orders) if (!o.done && !o.out && o.ready && o.service == DELIVERY && load.size() < 3) load.push_back(&o);
    if (load.empty()) return;
    float speed = 0.85f + 0.4f * w.care();   // good drivers know the shortcuts
    w.stops.clear(); w.route.clear();
    int at = roads.store();
    float clock = 0;
    w.route.push_back({0, 0, 0});
    auto drive = [&](int to) {
        std::vector<int> path;
        float mins = roads.minutes(at, to, &path) / speed;
        float total = 0;
        for (size_t i = 1; i < path.size(); i++) total += std::hypot(roads.x[path[i]] - roads.x[path[i - 1]], roads.y[path[i]] - roads.y[path[i - 1]]);
        float run = 0;
        for (size_t i = 1; i < path.size(); i++) {
            run += std::hypot(roads.x[path[i]] - roads.x[path[i - 1]], roads.y[path[i]] - roads.y[path[i - 1]]);
            w.route.push_back({roads.x[path[i]], roads.y[path[i]], clock + mins * (total > 0 ? run / total : 1)});
        }
        clock += mins; at = to;
    };
    while (!load.empty()) {   // nearest door first
        size_t best = 0; float bestT = 1e9f;
        for (size_t i = 0; i < load.size(); i++) {
            Customer* c = cust(load[i]->cust);
            int node = c && c->addr >= 0 ? c->addr : roads.store() + 1;
            float m = roads.minutes(at, node);
            if (m < bestT) { bestT = m; best = i; }
        }
        Order* o = load[best];
        Customer* c = cust(o->cust);
        drive(c && c->addr >= 0 ? c->addr : roads.store() + 1);
        clock += 2;   // to the door, the money, the dog
        w.route.push_back({w.route.back().x, w.route.back().y, clock});
        w.stops.push_back({o->num, clock});
        o->out = true; o->driver = wi;
        load.erase(load.begin() + best);
    }
    drive(roads.store());
    w.out = true; w.runT = 0; w.runLen = clock;
    events.push_back({E_DRIVER, w.stops[0].order, w.who.name});
}

void Store::staffStep(float dt) {
    for (int wi = 0; wi < (int)staff.size(); wi++) {
        Worker& w = staff[wi];
        float care = w.care();
        if (w.role == DRIVER) {
            if (!w.out) { sendDriver(wi); continue; }
            w.runT += dt / SEC_PER_MIN;
            for (auto& st : w.stops) {
                Order* o = order(st.order);
                if (o && !o->done && w.runT >= st.at) { resolve(*o); w.jobs++; Customer* c = cust(o->cust); events.push_back({E_DELIVERED, c ? c->leftStars : 0, ""}); }
            }
            if (w.runT >= w.runLen) { w.out = false; w.stops.clear(); w.route.clear(); }
            continue;
        }
        if (w.role == CSR) {
            Customer* c = cust(w.cust);
            if (c && (c->state != C_ORDERING || c->server != wi)) { w.cust = -1; c = nullptr; }
            if (!c) {   // hand over what's ready, then the phone, then the line; never the customer you're already talking to
                w.busy -= dt;
                if (w.busy > 0) continue;
                bool gave = false;
                for (int id : lobby()) { Customer* q = cust(id); Order* o = q ? order(q->order) : nullptr; if (o && o->ready && !o->done && q->timer > 2.5f) { handOut(id); w.busy = 3; gave = true; break; } }
                if (gave) continue;
                int id = ringing() >= 0 ? ringing() : firstInLine();
                Customer* n = cust(id);
                if (!n || closed) continue;
                if (n->state == C_INLINE && n->timer < 2.5f) continue;   // (gives you the chance to take them yourself)
                n->state = C_ORDERING; n->server = wi; n->timer = 0;
                w.cust = id;
                w.busy = (16 + 7 * (float)n->want.size()) * (1.5f - 0.6f * care) * rng.range(0.85f, 1.2f);
                if (n->patience < 0.5f || n->who.mood == 3) { w.yelled++; w.mood -= 0.03f; }   // rude ones wear them down
                continue;
            }
            w.busy -= dt;
            if (w.busy > 0) continue;
            std::vector<Spec> entered = c->want;
            int service = c->phone ? c->service : COUNTER;
            if (!rng.chance(0.25f + 0.75f * care)) {   // they got something wrong
                Spec& sp = entered[rng.below((int)entered.size())];
                int kind = rng.below(4);
                std::vector<int> tops;
                for (auto& it : sp.items) if (it.ing != MOZZ) tops.push_back(it.ing);
                if (kind == 0 || (kind == 1 && tops.empty())) { int z = sp.size; for (int tries = 0; tries < 20 && (z == sp.size || !crustComesIn(sp.crust, z)); tries++) z = rng.below(SIZE_N); if (z != sp.size && crustComesIn(sp.crust, z)) sp.size = z; else sp.items.push_back({PINEAPPLE, WHOLE, NORMAL}); }
                else if (kind == 1) sp.remove(tops[rng.below((int)tops.size())]);
                else if (kind == 2) { int d = ONION + rng.below(ING_N - ONION); if (!sp.find(d)) sp.items.push_back({d, WHOLE, NORMAL}); else sp.sauce = (sp.sauce + 1) % SAUCE_N; }
                else if (c->phone) service = service == DELIVERY ? CARRYOUT : DELIVERY;
                else sp.sauce = (sp.sauce + 1 + rng.below(3)) % SAUCE_N;
            }
            w.cust = -1; w.jobs++;
            place(*c, entered, service, wi);
            continue;
        }
        // ---- the makeline: the cut table comes first (pizzas go cold), then the next ticket
        w.busy -= dt;
        Ticket* k = ticket(w.ticket);
        if (k && k->state == T_MAKING && k->madeBy == wi) {
            if (w.busy > 0 || !ovenHasRoom()) continue;
            Pizza p = perfectPizza(k->spec, rng);
            p.id = (int)pizzas.size(); p.ticket = k->id; p.baked = 0; p.cuts.clear();
            float slop = (1 - care) * 0.9f;
            // portions by eye: each ingredient a bit over or under, and now and then one forgotten
            for (int ing = SAUCE_N; ing < ING_N; ing++) {
                float have = p.amount(ing);
                if (have <= 0) continue;
                float f = 1 + rng.range(-slop, slop);
                if (ing != MOZZ && rng.chance((1 - care) * 0.22f)) f = 0;
                if (f < 1) { int drop = (int)(have * (1 - f)); for (size_t i = p.bits.size(); i-- > 0 && drop > 0;) if (p.bits[i].ing == ing && rng.chance(0.6f)) { p.bits.erase(p.bits.begin() + i); drop--; } }
                else { int add = (int)(have * (f - 1)); for (int i = 0; i < add; i++) { float a = rng.range(0, 6.28f), rr = std::sqrt(rng.f()) * 0.8f; p.addBit(ing, rr * std::cos(a), rr * std::sin(a), rng); } }
            }
            float sf = 1 + rng.range(-slop, slop);
            for (int i = 0; i < SAUCE_N; i++) p.sauceOz[i] *= sf;
            pizzas.push_back(p);
            k->pizza = p.id; k->state = T_OVEN;
            oven.push_back({p.id, 0});
            w.ticket = -1; w.jobs++;
            continue;
        }
        w.ticket = -1;
        if (w.busy > 0) continue;
        if (!exitQ.empty() && pizzas[exitQ[0]].cool > 5) {   // cut and box what's waiting (if you haven't got to it)
            int id = exitQ[0];
            exitQ.erase(exitQ.begin());
            Pizza& p = pizzas[id];
            p.cuts = perfectPizza(Spec{p.size, p.crust}, rng).cuts;
            if (!rng.chance(0.3f + 0.7f * care)) p.cuts.resize(p.cuts.size() / 2);
            p.boxed = true;
            w.busy = 7 * (1.5f - 0.6f * care);
            if (Ticket* q = ticket(p.ticket)) if (q->state == T_EXIT) {
                q->state = T_BOXED;
                Order* o = order(q->order);
                if (o && !o->done) { bool all = true; for (int tid : o->tickets) { Ticket* z = ticket(tid); if (z && z->state != T_BOXED) all = false; } if (all) { o->ready = true; events.push_back({E_READY, o->num, ""}); } }
            }
            continue;
        }
        auto open = openTickets();
        if (!open.empty()) {
            Ticket* q = ticket(open[0]);
            q->state = T_MAKING; q->madeBy = wi;
            w.ticket = q->id;
            w.busy = (15 + 2.5f * (float)q->spec.items.size()) * (1.5f - 0.6f * care) * rng.range(0.9f, 1.15f);
        }
    }
}

int Store::closeDay() {
    // payday and how everyone feels about today
    for (int i = (int)staff.size(); i-- > 0;) {
        Worker& w = staff[i];
        money -= w.wage; dayWages += w.wage;
        if (w.role == DRIVER) {
            if (w.jobs > 0 && w.tips < 2) { w.mood -= 0.22f; dayNotes.push_back(w.who.name + " drove all night for no tips."); }
            else if (w.tips >= 6) w.mood += 0.08f;
        } else if (w.role == CSR) {
            w.mood += 0.07f - 0.035f * w.yelled;
            if (w.yelled >= 3) dayNotes.push_back(w.who.name + " got yelled at " + std::to_string(w.yelled) + " times today.");
        } else w.mood += w.jobs > 14 ? -0.05f : 0.05f;
        if (w.careDays > 0) w.careDays--;
        w.mood = std::clamp(w.mood, 0.f, 1.f);
        if (w.mood < 0.15f) {
            dayNotes.push_back(w.who.name + (w.role == DRIVER ? " threw the car topper on the counter and quit." : w.role == CSR ? " took off the headset, said \"I'm done,\" and quit." : " hung up the apron and quit."));
            w.out = false;
            fireWorker(i);
        }
    }
    newApplicants();
    if (dayReviews.empty() || rating() >= REPRIMAND_BELOW) return 0;
    if (reprimands.empty() || reprimands.back() != day) reprimands.push_back(day);
    if (reprimandsThisWeek() >= 3) { fired = true; return 2; }
    return 1;
}
std::string Store::clock() const {
    return timeText((int)clockMin());
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
    UseSpeechMemory um(speech);
    talk.pending = orderScript(rng, c->who, c->want, c->service, c->phone, day, ((int)(clockMin() / 60)) % 24);
    return true;
}
void Store::askRepeat() {
    Customer* c = cust(talk.cust);
    if (!c) return;
    if (talk.speaking && !talk.log.empty()) talk.shown = (float)talk.log.back().text.size();
    talk.speaking = false; talk.pending.clear();
    talk.log.push_back({true, "Sorry, could you repeat that?"});
    c->patience -= 0.15f;
    UseSpeechMemory um(speech);
    c->asks++;
    Ctx vc = voice(*c);
    say(heat(rng, recap(rng, c->want, c->service, c->phone, &vc), 1 - c->patience));
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
    UseSpeechMemory um(speech);
    Ctx vc = voice(*c);
    std::string answer = readBack(rng, c->want, talk.draft, c->service, talk.draftService, c->phone, ok, &vc);
    if (!ok) c->asks++;
    say(ok ? answer : heat(rng, answer, 1 - c->patience + 0.25f));
    talk.pause = 1.2f;
}
bool Store::finishOrder() {
    Customer* c = cust(talk.cust);
    if (!c || talk.draft.empty()) return false;
    place(*c, talk.draft, c->phone ? talk.draftService : COUNTER, -1);
    talk = Talk{};
    return true;
}
void Store::place(Customer& c, const std::vector<Spec>& entered, int service, int takenBy) {
    Order o;
    o.num = nextOrder++;
    o.cust = c.id;
    o.service = service;
    o.entered = entered;
    o.placed = t;
    o.takenBy = takenBy;
    int n = (int)o.entered.size();
    if (c.phone && c.service == DELIVERY) c.addr = roads.randomAddress(rng);
    for (int i = 0; i < n; i++) {
        Ticket k{nextTicket++, o.num, i + 1, n, o.entered[i]};
        o.tickets.push_back(k.id);
        tickets.push_back(k);
        o.price += price(o.entered[i]);
    }
    c.order = o.num;
    c.timer = 0; c.patience = 1; c.server = -1;
    if (!c.phone) c.state = C_WAITING;
    else if (c.service == CARRYOUT) { c.state = C_COMING; c.arriveIn = rng.range(45, 75); }
    else c.state = C_AWAY;
    orders.push_back(o);
    dayOrders++;
    events.push_back({E_PLACED, o.num, ""});
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
    if (extra.empty()) os = scoreOrder(c->want, made, c->phone ? c->service : COUNTER, o.service, o.minutes(t));
    else { for (auto& is : extra) os.score.add(is.code, is.pts, is.ing); os.stars = starsFor(os.score.penalty); }
    if (os.stars >= 3) {
        float tip = os.stars == 5 ? o.price * (o.service == DELIVERY ? 0.18f : 0.06f) : os.stars == 4 ? o.price * (o.service == DELIVERY ? 0.08f : 0.04f) : 0;
        money += o.price; dayMoney += o.price;
        if (o.driver >= 0 && o.driver < (int)staff.size()) staff[o.driver].tips += tip;   // a driver keeps their tips
        else { money += tip; dayTips += tip; }
    }   // two stars or worse: they got their money back
    c->leftStars = os.stars;
    // whose fault was it? An order rung up wrong is on whoever took it; a pizza made wrong is on whoever made it
    if (os.stars <= 3 && extra.empty()) {
        bool rungWrong = o.entered.size() != c->want.size() || (c->phone && o.service != c->service);
        for (size_t i = 0; !rungWrong && i < o.entered.size(); i++) { bool any = false; for (auto& w : c->want) if (sameSpec(w, o.entered[i])) any = true; if (!any) rungWrong = true; }
        if (rungWrong && o.takenBy >= 0 && o.takenBy < (int)staff.size()) { staff[o.takenBy].mistakes++; note(staff[o.takenBy].who.name + " rang up order #" + std::to_string(o.num) + " wrong."); }
        for (int id : o.tickets) {
            Ticket* k = ticket(id);
            if (!k || k->madeBy < 0 || k->madeBy >= (int)staff.size() || k->pizza < 0) continue;
            Score ps = scorePizza(k->spec, pizzas[k->pizza]);
            float bad = 0; std::string why;
            for (auto& is : ps.issues) if (is.code != LUKEWARM && is.code != COLD && is.pts > bad) { bad = is.pts; why = issueLine(is); }
            if (bad >= 1) { staff[k->madeBy].mistakes++; note(staff[k->madeBy].who.name + " botched a pizza on order #" + std::to_string(o.num) + " (" + why + ")."); }
        }
        if (o.driver >= 0 && o.driver < (int)staff.size()) for (auto& is : os.score.issues) if (is.code == EXTREMELY_LATE) { staff[o.driver].mistakes++; break; }
    }
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
    if (!closed && t < SHIFT_LEN - 20 * SEC_PER_MIN && t >= nextArrival) {
        spawn();
        // orders an hour, by the time of day: dead mornings, a lunch rush, a lull, the big dinner rush, then a late tail
        static const float PER_HOUR[24] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 1.2f, 2, 4.5f, 5.5f, 4.5f, 2.5f, 2, 3, 5.5f, 7, 6.5f, 4.5f, 3, 2.5f, 2};
        float hr = clockMin() / 60, f = hr - std::floor(hr);
        int h0 = (int)hr % 24, h1 = (h0 + 1) % 24;
        float perHour = (PER_HOUR[h0] * (1 - f) + PER_HOUR[h1] * f) * std::min(1.5f, (day <= 1 ? 0.75f : 1.f) + 0.05f * (day - 1));
        float mean = 60.f * SEC_PER_MIN / std::max(0.8f, perHour);
        nextArrival = t + mean * rng.range(0.55f, 1.45f);
    }
    // ---- the conversation
    if (talk.active()) {
        Customer* c = cust(talk.cust);
        float speed = c && c->who.mood == 0 ? 30.f : c && c->who.mood == 1 ? 19.f : 24.f;
        if (talk.speaking) {
            float before = talk.shown;
            talk.shown += speed * dt;
            if ((int)(talk.shown / 3) != (int)(before / 3)) events.push_back({E_TALK, 0, ""});
            if (talk.shown >= (float)talk.log.back().text.size()) { talk.speaking = false; talk.pause = 1.1f; }
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
                c.patience = 1 - c.timer / 45;
                if (c.timer > 45 || closed) lose(c, NO_ANSWER);
                break;
            case C_INLINE:
                c.patience -= dt / 140;
                if (c.patience <= 0) lose(c, RUDE_SERVICE);
                break;
            case C_ORDERING:
                c.patience -= dt / 260;
                if (c.patience <= 0) { talk = Talk{}; lose(c, RUDE_SERVICE); }
                break;
            case C_COMING:
                c.arriveIn -= dt;
                if (c.arriveIn <= 0) { c.state = C_WAITING; c.timer = 0; events.push_back({E_DOOR, c.id, ""}); }
                break;
            default: break;
        }
        if (c.state == C_WAITING || c.state == C_INLINE) {
            if (Order* o = order(c.order)) c.patience = std::clamp(1 - o->minutes(t) / VERY_LATE_MIN, 0.f, 1.f);
            c.barkT -= dt; c.nextBark -= dt;
            if (c.nextBark <= 0) {
                c.nextBark = rng.range(12, 26) * (0.5f + c.patience);
                if (c.patience < 0.75f) { UseSpeechMemory um(speech); Ctx vc = voice(c); c.bark = waitingBark(rng, c.patience, &vc); c.barkT = 5.5f; }
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
    // ---- the staff, and the orders that went wrong on the phone
    staffStep(dt);
    for (auto& o : orders) {
        if (o.done) continue;
        Customer* c = cust(o.cust);
        if (!c) continue;
        if (o.service == DELIVERY) {
            // they said pickup: they turn up at the counter and there's nothing for them
            if (c->service == CARRYOUT && c->state == C_WAITING && c->timer > 6) resolve(o, {{WRONG_SERVICE, -1, 4}});
        } else if (o.service == CARRYOUT && c->service == DELIVERY) {
            // they said delivery: nobody ever comes for it
            if (o.minutes(t) > 40) resolve(o, {{WRONG_SERVICE, -1, 4}});
        }
    }
    // ---- closing time
    if (closed) {
        bool busy = talk.active();
        for (auto& c : custs) if (c.state != C_DONE) busy = true;
        if (!busy || t > SHIFT_LEN + 30 * SEC_PER_MIN) {
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
    j["hireSeed"] = hireSeed;
    j["crewSet"] = crewSet;
    auto st = nlohmann::json::array();
    for (auto& w : staff) st.push_back({{"name", w.who.name}, {"look", w.who.look}, {"age", w.who.age}, {"sex", w.who.sex}, {"skin", w.who.skin}, {"hair", w.who.hair},
        {"hairStyle", w.who.hairStyle}, {"role", w.role}, {"skill", w.skill}, {"mood", w.mood}, {"wage", w.wage}, {"writeUps", w.writeUps}, {"careDays", w.careDays}});
    j["staff"] = st;
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
    hireSeed = j.value("hireSeed", (uint64_t)99);
    crewSet = j.value("crewSet", false);
    staff.clear();
    if (j.contains("staff") && j["staff"].is_array()) for (auto& e : j["staff"]) if (e.is_object()) {
        Worker w;
        w.who.name = e.value("name", "Someone"); w.who.look = e.value("look", 0u); w.who.age = e.value("age", 30); w.who.sex = e.value("sex", 0);
        w.who.skin = e.value("skin", 0); w.who.hair = e.value("hair", 0); w.who.hairStyle = e.value("hairStyle", 1);
        w.role = std::clamp(e.value("role", 0), 0, 2); w.skill = e.value("skill", 0.6f); w.mood = e.value("mood", 0.7f); w.wage = e.value("wage", 50.f);
        w.writeUps = e.value("writeUps", 0); w.careDays = e.value("careDays", 0);
        staff.push_back(w);
    }
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
