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
        CHECK(scoreOrder({a, b}, {&pb, &pa}, COUNTER, COUNTER, 0.5f).stars == 5);
        CHECK(scoreOrder({a, b}, {&pa}, COUNTER, COUNTER, 0.5f).stars <= 3);
        CHECK(scoreOrder({a}, {&pa}, DELIVERY, CARRYOUT, 0.5f).stars <= 2);
        CHECK(scoreOrder({a}, {&pa}, COUNTER, COUNTER, 1.2f).stars == 4);
    }
    // whole shifts: a careful bot keeps the shop well reviewed, a careless one sinks it
    for (int pass = 0; pass < 2; pass++) {
        float care = pass == 0 ? 1.0f : 0.3f;
        float sum = 0; int n = 0, orders = 0, lost = 0;
        for (int day = 1; day <= 4; day++) {
            Store s; s.day = day; s.startDay(100 + day);
            Rng br(day);
            for (int i = 0; i < 60 * 800 && !s.dayOver; i++) { s.update(1 / 60.f); botStep(s, 1 / 60.f, care, br); }
            CHECK(s.dayOver);
            for (auto& r : s.dayReviews) { sum += r.stars; n++; if (pass == 0 && getenv("MAKELINE_TESTDBG")) { printf("  day %d %d*:", day, r.stars); for (auto& x : r.reasons) printf(" [%s]", x.c_str()); printf("\n"); } }
            orders += s.dayOrders; lost += s.dayLost;
            // saving and loading keeps the reviews
            Store t2; CHECK(t2.fromJson(s.toJson())); CHECK(t2.reviews.size() == s.reviews.size());
        }
        float avg = n ? sum / n : 0;
        printf("bot care %.1f: %d orders, %d walked out, %d reviews, average %.2f stars\n", care, orders, lost, n, avg);
        CHECK(orders >= 20);
        if (pass == 0) CHECK(avg > 4.0f); else CHECK(avg < 3.0f);
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
