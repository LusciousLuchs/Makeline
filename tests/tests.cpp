// Makeline - tests of the rules: portions, judging, what customers say, and whole shifts played by a bot.
#include "store.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
using namespace pz;
static int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static bool has(const Score& s, int code, int ing = -1) { for (auto& i : s.issues) if (i.code == code && (ing < 0 || i.ing == ing)) return true; return false; }

int main() {
    Rng rng(42);
    // every kind of order, made exactly right, is a clean pizza
    for (int day = 1; day <= 5; day++) for (int k = 0; k < 400; k++) {
        Spec s = randomPizza(rng, day);
        CHECK(crustComesIn(s.crust, s.size));
        Pizza p = perfectPizza(s, rng);
        Score sc = scorePizza(s, p);
        if (sc.penalty > 0) { printf("  perfect pizza judged: %s (%s)\n", issueLine(sc.issues[0]).c_str(), speakPizza(rng, s, false).c_str()); }
        CHECK(sc.penalty == 0);
        CHECK(sameSpec(s, mirrored(s)));
        // said back correctly, the customer agrees; with something changed, they don't
        bool ok = false;
        readBack(rng, {s}, {mirrored(s)}, CARRYOUT, CARRYOUT, true, ok);
        CHECK(ok);
        Spec w = s; w.size = (w.size + 1) % SIZE_N;
        readBack(rng, {s}, {w}, CARRYOUT, CARRYOUT, true, ok);
        CHECK(!ok);
        readBack(rng, {s}, {s}, DELIVERY, CARRYOUT, true, ok);
        CHECK(!ok);
    }
    {   // the ways to get it wrong
        Spec s; s.items.push_back({PEPPERONI, WHOLE, NORMAL});
        Pizza p = perfectPizza(s, rng);
        Pizza q = p; q.size = MEDIUM;
        CHECK(has(scorePizza(s, q), WRONG_SIZE));
        q = p; q.bits.erase(std::remove_if(q.bits.begin(), q.bits.end(), [](const Bit& b) { return b.ing == PEPPERONI; }), q.bits.end());
        CHECK(has(scorePizza(s, q), MISSING, PEPPERONI));
        q = p; for (int i = 0; i < 20; i++) q.addBit(PINEAPPLE, rng.range(-0.5f, 0.5f), rng.range(-0.5f, 0.5f), rng);
        CHECK(has(scorePizza(s, q), UNWANTED, PINEAPPLE));
        q = p; for (int i = 0; i < 40; i++) q.addBit(PEPPERONI, rng.range(-0.5f, 0.5f), rng.range(-0.5f, 0.5f), rng);
        CHECK(has(scorePizza(s, q), TOO_MUCH, PEPPERONI));
        q = p; q.bits.erase(std::remove_if(q.bits.begin(), q.bits.end(), [](const Bit& b) { return b.ing == MOZZ && b.x > 0; }), q.bits.end());
        CHECK(has(scorePizza(s, q), TOO_LITTLE, MOZZ));
        q = p; q.cuts.clear();
        CHECK(has(scorePizza(s, q), UNCUT));
        q = p; q.cuts = perfectPizza(Spec{LARGE, THIN}, rng).cuts;
        CHECK(has(scorePizza(s, q), WRONG_CUT));
        q = p; q.sauceOz[MARINARA] = 0; q.sauceOz[BBQ] = 5;
        CHECK(has(scorePizza(s, q), WRONG_SAUCE));
        q = p; q.sauceOz[MARINARA] *= 2;
        CHECK(has(scorePizza(s, q), SAUCE_HIGH));
        q = p; q.stamps.resize(q.stamps.size() / 3);
        CHECK(has(scorePizza(s, q), SAUCE_PATCHY));
        q = p; q.cool = 200;
        CHECK(has(scorePizza(s, q), COLD));
        // half pepperoni made on the whole pizza
        Spec h; h.items.push_back({PEPPERONI, LEFT, NORMAL});
        CHECK(has(scorePizza(h, p), WRONG_HALF, PEPPERONI));
        CHECK(scorePizza(h, perfectPizza(mirrored(h), rng)).penalty == 0);
        // an order: the right pizzas in any order, one missing, service mixed up
        Spec a, b; b.size = SMALL; b.sauce = BBQ;
        Pizza pa = perfectPizza(a, rng), pb = perfectPizza(b, rng);
        CHECK(scoreOrder({a, b}, {&pb, &pa}, COUNTER, COUNTER, 12).stars == 5);
        CHECK(scoreOrder({a, b}, {&pa}, COUNTER, COUNTER, 12).stars <= 3);
        CHECK(scoreOrder({a}, {&pa}, DELIVERY, CARRYOUT, 12).stars <= 2);
        CHECK(scoreOrder({a}, {&pa}, COUNTER, COUNTER, 19).stars == 5);   // the order timer: 20 late, 30 very late, 45 extreme
        CHECK(scoreOrder({a}, {&pa}, COUNTER, COUNTER, 21).stars == 4);
        CHECK(scoreOrder({a}, {&pa}, COUNTER, COUNTER, 31).stars == 3);
        CHECK(scoreOrder({a}, {&pa}, COUNTER, COUNTER, 46).stars == 1);
    }
    // whole shifts: a careful bot keeps the shop well reviewed, a careless one sinks it
    for (int pass = 0; pass < 2; pass++) {
        float care = pass == 0 ? 1.0f : 0.3f;
        float sum = 0; int n = 0, orders = 0, lost = 0;
        for (int day = 1; day <= 4; day++) {
            Store s; s.day = day; s.startDay(100 + day);
            Rng br(day);
            for (int i = 0; i < 60 * 1600 && !s.dayOver; i++) { s.update(1 / 60.f); botStep(s, 1 / 60.f, care, br); }
            CHECK(s.dayOver);
            for (auto& r : s.dayReviews) { sum += r.stars; n++; if (pass == 0 && getenv("MAKELINE_TESTDBG")) { printf("  day %d %d*:", day, r.stars); for (auto& x : r.reasons) printf(" [%s]", x.c_str()); printf("\n"); } }
            orders += s.dayOrders; lost += s.dayLost;
            // saving and loading keeps the reviews
            Store t2; CHECK(t2.fromJson(s.toJson())); CHECK(t2.reviews.size() == s.reviews.size());
        }
        float avg = n ? sum / n : 0;
        printf("bot care %.1f: %d orders, %d walked out, %d reviews, average %.2f stars\n", care, orders, lost, n, avg);
        CHECK(orders >= 20);
        if (pass == 0) CHECK(avg > 3.5f); else CHECK(avg < 2.5f);   // (alone, with one driver: late orders are part of life)
    }
    {   // reprimands: a bad score at closing is one; three inside a week and you're fired; old ones stop counting
        Store s;
        auto close = [&](int day, int stars) { s.day = day; s.dayReviews = {Review{"x", "y", stars, day, {}}}; s.reviews.assign(10, Review{"x", "y", stars, day, {}}); return s.closeDay(); };
        CHECK(close(1, 5) == 0);
        CHECK(close(2, 1) == 1);
        CHECK(close(3, 4) == 0);
        CHECK(close(4, 2) == 1);
        CHECK(close(9, 1) == 1);          // day 2's is over a week old by now
        CHECK(!s.fired);
        CHECK(close(10, 1) == 2);         // days 4, 9 and 10
        CHECK(s.fired);
        Store t2; CHECK(t2.fromJson(s.toJson())); CHECK(t2.fired && t2.reprimands.size() == s.reprimands.size());
        Store q; q.day = 3; CHECK(q.closeDay() == 0);   // nobody reviewed: nothing to judge
    }
    {   // the roads: every crossing can be reached, further is slower, and no two shifts get the same map
        Rng r1(3), r2(4);
        RoadMap a, b; a.generate(r1); b.generate(r2);
        int diff = 0;
        for (int k = 0; k < RoadMap::N * RoadMap::N; k++) { CHECK(a.minutes(a.store(), k) < 60); if (a.right[k] != b.right[k]) diff++; }
        CHECK(diff > 20);
        CHECK(a.minutes(a.store(), 0) > a.minutes(a.store(), a.store() + 2));
        for (int i = 0; i < 200; i++) { int n = a.randomAddress(r1); CHECK(a.miles(n) <= 10 && n != a.store()); }
    }
    {   // a crew runs the shop without you: a good one keeps the score up, a bad one doesn't, and its mistakes are noted
        float avg[2];
        for (int pass = 0; pass < 2; pass++) {
            Store s; s.startDay(77);
            float skill = pass == 0 ? 0.97f : 0.25f;
            for (int role : {CSR, CSR, COOK, COOK, DRIVER}) { Worker w; Rng r(role + 5); w.who = makePerson(r); w.role = role; w.skill = skill; s.staff.push_back(w); }
            for (int i = 0; i < 60 * 1600 && !s.dayOver; i++) s.update(1 / 60.f);
            if (getenv("MAKELINE_TESTDBG")) for (auto& r : s.dayReviews) { printf("  crew %d*:", r.stars); for (auto& x : r.reasons) printf(" [%s]", x.c_str()); printf("\n"); }
            CHECK(s.dayOver);
            float sum = 0; for (auto& r : s.dayReviews) sum += r.stars;
            avg[pass] = s.dayReviews.empty() ? 0 : sum / s.dayReviews.size();
            int mistakes = 0; for (auto& w : s.staff) mistakes += w.mistakes;
            printf("crew skill %.2f: %d orders, %d reviews, average %.2f stars, %d staff mistakes\n", skill, s.dayOrders, (int)s.dayReviews.size(), avg[pass], mistakes);
            if (pass == 1) CHECK(mistakes > 0);
            // a write-up for nothing costs more goodwill than one that was earned; miserable staff quit at closing
            Store q; q.startDay(1);
            Worker w; Rng r(9); w.who = makePerson(r); w.role = CSR; w.mood = 0.5f; q.staff.push_back(w); q.staff.push_back(w);
            q.staff[1].mistakes = 1; q.staff[2].mistakes = 0;
            q.writeUp(1); q.writeUp(2);
            CHECK(q.staff[2].mood < q.staff[1].mood);
            q.staff[2].mood = 0.05f;
            size_t before = q.staff.size();
            q.closeDay();
            CHECK(q.staff.size() == before - 1);
            Store l; CHECK(l.fromJson(q.toJson())); CHECK(l.staff.size() == q.staff.size());
        }
        CHECK(avg[0] > avg[1] + 1);
        CHECK(avg[0] > 3.5f);
    }
    {   // a script always names every topping somewhere (or loses one to static)
        for (int k = 0; k < 300; k++) {
            Person who = makePerson(rng);
            auto want = randomOrder(rng, 4);
            auto lines = orderScript(rng, who, want, DELIVERY, true, 4);
            CHECK(lines.size() >= 3);
            std::string rc = recap(rng, want, DELIVERY, true);
            for (auto& sp : want) for (auto& it : sp.items) if (it.ing != MOZZ)
                CHECK(rc.find(info(it.ing).say) != std::string::npos || rc.find(info(it.ing).say2) != std::string::npos);
        }
    }
    printf(fails ? "%d FAILED\n" : "all tests passed\n", fails);
    return fails ? 1 : 0;
}
