// Makeline - making and judging a pizza.
#include "pizza.hpp"
#include <algorithm>
#include <cmath>

namespace pz {
namespace {
const float PI = 3.14159265f;
}

bool Pizza::addSauce(int sauce, float x, float y, float oz) {
    if (sauce < 0 || sauce >= SAUCE_N) return false;
    float r = std::sqrt(x * x + y * y);
    if (r > 1.0f) return false;
    if (r > SAUCE_EDGE - STAMP_R * 0.6f) {   // keep the dab off the crust
        float k = (SAUCE_EDGE - STAMP_R * 0.6f) / r;
        x *= k; y *= k;
    }
    sauceOz[sauce] += oz;
    // a new dab only when the ladle has moved (a held ladle pools sauce without spreading it)
    bool far = true;
    for (size_t i = stamps.size(); i-- > 0 && stamps.size() - i < 40;) {
        float dx = stamps[i].x - x, dy = stamps[i].y - y;
        if (stamps[i].sauce == sauce && dx * dx + dy * dy < 0.045f * 0.045f) { far = false; break; }
    }
    if (far && stamps.size() < 1500) stamps.push_back({(uint8_t)sauce, x, y});
    return true;
}
bool Pizza::addBit(int ing, float x, float y, Rng& rng) {
    if (x * x + y * y > TOP_EDGE * TOP_EDGE) return false;
    if (bits.size() >= 1200) return false;
    bits.push_back({(uint8_t)ing, (uint8_t)rng.below(4), x, y, rng.range(0, 2 * PI)});
    return true;
}
int Pizza::removeNear(float x, float y, float r) {
    size_t before = bits.size();
    bits.erase(std::remove_if(bits.begin(), bits.end(), [&](const Bit& b) { float dx = b.x - x, dy = b.y - y; return dx * dx + dy * dy < r * r; }), bits.end());
    return (int)(before - bits.size());
}
void Pizza::addCut(float x1, float y1, float x2, float y2) {
    float dx = x2 - x1, dy = y2 - y1, len = std::sqrt(dx * dx + dy * dy);
    if (len < 0.5f || cuts.size() >= 12) return;   // a nick, not a cut
    float ang = std::atan2(dy, dx);
    if (ang < 0) ang += PI;
    if (ang >= PI) ang -= PI;
    float nx = -std::sin(ang), ny = std::cos(ang);
    cuts.push_back({ang, x1 * nx + y1 * ny});
}
float Pizza::amount(int ing) const {
    if (ing < SAUCE_N) return sauceOz[ing];
    float n = 0;
    for (auto& b : bits) if (b.ing == ing) n++;
    return n;
}
float Pizza::amountSide(int ing, bool right) const {
    float n = 0;
    for (auto& b : bits) if (b.ing == ing && (b.x >= 0) == right) n++;
    return n;
}
float Pizza::totalSauce() const { return sauceOz[0] + sauceOz[1] + sauceOz[2] + sauceOz[3]; }
int Pizza::mainSauce() const {
    int best = -1;
    for (int i = 0; i < SAUCE_N; i++) if (sauceOz[i] > 0.05f && (best < 0 || sauceOz[i] > sauceOz[best])) best = i;
    return best;
}
float Pizza::sauceCoverage() const {
    const int N = 26;
    int in = 0, hit = 0;
    for (int j = 0; j < N; j++) for (int i = 0; i < N; i++) {
        float x = ((i + 0.5f) / N * 2 - 1) * SAUCE_EDGE, y = ((j + 0.5f) / N * 2 - 1) * SAUCE_EDGE;
        if (x * x + y * y > SAUCE_EDGE * SAUCE_EDGE) continue;
        in++;
        for (auto& s : stamps) { float dx = s.x - x, dy = s.y - y; if (dx * dx + dy * dy < STAMP_R * STAMP_R) { hit++; break; } }
    }
    return in ? (float)hit / in : 0;
}

int cutStyle(const Pizza& p) {
    int n = (int)p.cuts.size();
    if (n == 0) return 0;
    auto deg = [](float a) { return a * 180 / PI; };
    {   // pie: half as many cuts as slices, all through the middle, evenly turned
        int need = slicesFor(p.size) / 2;
        bool ok = n == need;
        std::vector<float> a;
        for (auto& c : p.cuts) { if (std::fabs(c.off) > 0.13f) ok = false; a.push_back(deg(c.ang)); }
        if (ok) {
            std::sort(a.begin(), a.end());
            for (int i = 0; i < n; i++) {
                float gap = (i + 1 < n ? a[i + 1] : a[0] + 180) - a[i];
                if (std::fabs(gap - 180.f / need) > 16) ok = false;
            }
        }
        if (ok) return 1;
    }
    {   // squares: two sets of cuts at right angles, at least two each way, spaced apart
        std::vector<float> A, B;
        float a0 = deg(p.cuts[0].ang);
        bool ok = true;
        for (auto& c : p.cuts) {
            float d = std::fabs(deg(c.ang) - a0);
            if (d > 90) d = 180 - d;
            if (d < 13) A.push_back(c.off * (std::fabs(deg(c.ang) - a0) > 90 ? -1 : 1));
            else if (d > 77) B.push_back(c.off);
            else ok = false;
        }
        auto spaced = [](std::vector<float>& v) {
            if (v.size() < 2) return false;
            std::sort(v.begin(), v.end());
            for (size_t i = 0; i + 1 < v.size(); i++) if (v[i + 1] - v[i] < 0.18f) return false;
            return true;
        };
        if (ok && spaced(A) && spaced(B)) return 2;
    }
    return 3;
}

namespace {
void judgeIng(Score& sc, const Spec& want, const Pizza& p, int ing) {
    const Item* it = want.find(ing);
    float normal = info(ing).normal[p.size];
    float expL = 0, expR = 0;
    if (it) {
        float t = portion(ing, want.size, it->level) * 0.5f;
        if (it->part != RIGHT) expL = t;
        if (it->part != LEFT) expR = t;
    }
    float gotL = p.amountSide(ing, false), gotR = p.amountSide(ing, true);
    float stray = std::max(2.f, 0.25f * normal * 0.5f);
    if (expL + expR <= 0) {
        if (gotL + gotR > stray) sc.add(UNWANTED, 2.0f, ing);
        return;
    }
    float r = (gotL + gotR) / (expL + expR);
    if (r < 0.3f) { sc.add(MISSING, 2.0f, ing); return; }
    if ((expL <= 0 && gotL > stray) || (expR <= 0 && gotR > stray)) {
        // it's there, but over the line: only what's on the right half counts
        sc.add(WRONG_HALF, 1.0f, ing);
        return;
    }
    if (r < 0.6f) sc.add(TOO_LITTLE, 1.0f, ing);
    else if (r < 0.8f) sc.add(TOO_LITTLE, 0.5f, ing);
    else if (r > 1.7f) sc.add(TOO_MUCH, 1.0f, ing);
    else if (r > 1.3f) sc.add(TOO_MUCH, 0.5f, ing);
    // spread: each eighth of the pizza should get its share, and not all of it in the middle
    if (expL + expR >= 8) {
        float cnt[8] = {0}, sumR = 0, n = 0;
        for (auto& b : p.bits) if (b.ing == ing) {
            float a = std::atan2(b.y, b.x) + PI / 2;
            if (a < 0) a += 2 * PI;
            cnt[std::min(7, (int)(a / (PI / 4)))]++;
            sumR += std::sqrt(b.x * b.x + b.y * b.y); n++;
        }
        float even = 0; int secs = 0;
        for (int k = 0; k < 8; k++) {
            float e = (k < 4 ? expR : expL) / 4;
            if (e <= 0) continue;
            even += std::min(1.f, cnt[k] / e); secs++;
        }
        even = secs ? even / secs : 1;
        float meanR = n ? sumR / n : 0.6f;
        if (even < 0.55f * std::min(1.f, r) || meanR < 0.4f || meanR > 0.78f) sc.add(UNEVEN, 0.5f, ing);
    }
}
Score scoreOneWay(const Spec& want, const Pizza& p) {
    Score sc;
    if (p.size != want.size) sc.add(WRONG_SIZE, 2.0f);
    if (p.crust != want.crust) sc.add(WRONG_CRUST, 1.5f);
    // sauce
    int ms = p.mainSauce();
    float total = p.totalSauce(), target = portion(want.sauce, want.size, want.sauceLevel);
    if (ms < 0) sc.add(NO_SAUCE, 2.0f, want.sauce);
    else if (ms != want.sauce || p.sauceOz[want.sauce] < 0.7f * total) sc.add(WRONG_SAUCE, 2.0f, ms != want.sauce ? ms : want.sauce);
    if (ms >= 0) {
        float r = total / target;
        if (r < 0.55f) sc.add(SAUCE_LOW, 1.0f);
        else if (r < 0.78f) sc.add(SAUCE_LOW, 0.5f);
        else if (r > 1.8f) sc.add(SAUCE_HIGH, 1.0f);
        else if (r > 1.32f) sc.add(SAUCE_HIGH, 0.5f);
        if (p.sauceCoverage() < 0.72f) sc.add(SAUCE_PATCHY, 0.5f);
    }
    for (int ing = SAUCE_N; ing < ING_N; ing++) judgeIng(sc, want, p, ing);
    // the cut: thin crust is cut in squares, everything else like a pie
    int style = cutStyle(p), right = p.crust == THIN ? 2 : 1;
    if (style == 0) sc.add(UNCUT, 1.0f);
    else if (style == 3) sc.add(BAD_CUT, 0.5f);
    else if (style != right) sc.add(WRONG_CUT, 0.5f);
    if (p.cool > 170) sc.add(COLD, 1.0f);
    else if (p.cool > 100) sc.add(LUKEWARM, 0.5f);
    return sc;
}
}  // namespace

Score scorePizza(const Spec& want, const Pizza& made) {
    Score a = scoreOneWay(want, made);
    bool halves = false;
    for (auto& i : want.items) if (i.part != WHOLE) halves = true;
    if (!halves) return a;
    Score b = scoreOneWay(mirrored(want), made);
    return b.penalty < a.penalty ? b : a;
}

int starsFor(float penalty) {
    if (penalty <= 0.5f) return 5;
    if (penalty <= 1.25f) return 4;
    if (penalty <= 2.25f) return 3;
    if (penalty <= 3.25f) return 2;
    return 1;
}

OrderScore scoreOrder(const std::vector<Spec>& want, const std::vector<const Pizza*>& made, int wantService, int gotService, float waitRatio) {
    OrderScore out;
    int nw = (int)want.size(), nm = (int)made.size(), n = std::max(nw, nm);
    std::vector<int> perm(n);
    for (int i = 0; i < n; i++) perm[i] = i;
    Score best; float bestPen = 1e9f;
    do {   // wanted pizza i gets made pizza perm[i] (an index past the end = none)
        Score s; float mx = 0, sum = 0;
        for (int i = 0; i < n; i++) {
            if (i >= nw) { if (perm[i] < nm) { s.issues.push_back({EXTRA_PIZZA, -1, 1.0f}); sum += 1; mx = std::max(mx, 1.f); } continue; }
            if (perm[i] >= nm) { s.issues.push_back({MISSING_PIZZA, -1, 2.5f}); sum += 2.5f; mx = std::max(mx, 2.5f); continue; }
            Score one = scorePizza(want[i], *made[perm[i]]);
            for (auto& is : one.issues) s.issues.push_back(is);
            sum += one.penalty; mx = std::max(mx, one.penalty);
        }
        s.penalty = mx + 0.5f * (sum - mx);   // the worst pizza counts in full, the others half
        if (s.penalty < bestPen) { bestPen = s.penalty; best = s; }
    } while (std::next_permutation(perm.begin(), perm.end()));
    out.score = best;
    if (wantService != gotService && !(wantService == COUNTER || gotService == COUNTER)) out.score.add(WRONG_SERVICE, 3.0f);
    if (waitRatio > 1.6f) out.score.add(VERY_LATE, 2.0f);
    else if (waitRatio > 1.0f) out.score.add(LATE, 1.0f);
    out.stars = starsFor(out.score.penalty);
    return out;
}

Pizza perfectPizza(const Spec& s, Rng& rng) {
    Pizza p;
    p.size = s.size; p.crust = s.crust;
    // sauce in a spiral
    float oz = portion(s.sauce, s.size, s.sauceLevel);
    const int N = 150;
    for (int i = 0; i < N; i++) {
        float t = (i + 0.5f) / N, r = std::sqrt(t) * (SAUCE_EDGE - 0.05f), a = i * 2.39996f;
        p.addSauce(s.sauce, r * std::cos(a), r * std::sin(a), oz / N);
    }
    for (auto& it : s.items) {
        float full = portion(it.ing, s.size, it.level);
        int n = (int)std::lround(it.part == WHOLE ? full : full * 0.5f);
        float a0 = rng.range(0, 6.28f);
        for (int i = 0; i < n; i++) {
            float t = (i + 0.5f) / n, r = std::sqrt(t) * (TOP_EDGE - 0.06f), a = a0 + i * 2.39996f;
            float x = r * std::cos(a), y = r * std::sin(a);
            if (it.part != WHOLE) {   // the same spiral folded onto one half
                x = std::fabs(x) + 0.03f;
                if (it.part == LEFT) x = -x;
                if (x * x + y * y > 0.84f * 0.84f) { float k = 0.84f / std::sqrt(x * x + y * y); x *= k; y *= k; }
            }
            p.addBit(it.ing, x, y, rng);
        }
    }
    p.baked = 1;
    if (s.crust == THIN) { for (float o : {-0.4f, 0.f, 0.4f}) { p.cuts.push_back({0, o}); p.cuts.push_back({PI / 2, o}); } }
    else { int n = slicesFor(s.size) / 2; for (int i = 0; i < n; i++) p.cuts.push_back({PI * i / n, 0}); }
    return p;
}

}  // namespace pz
