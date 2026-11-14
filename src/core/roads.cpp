// Makeline - the delivery area's roads.
#include "roads.hpp"
#include <algorithm>
#include <cmath>

namespace pz {

void RoadMap::generate(Rng& r) {
    const int C = N / 2;
    unsigned char rowSpeed[N], colSpeed[N];
    static const unsigned char RES[] = {25, 25, 30, 35};
    for (int i = 0; i < N; i++) {
        rowSpeed[i] = (i - C) % 5 == 0 ? 45 : RES[r.below(4)];
        colSpeed[i] = (i - C) % 5 == 0 ? 45 : RES[r.below(4)];
    }
    for (int j = 0; j < N; j++) for (int i = 0; i < N; i++) {
        int k = j * N + i;
        bool main = rowSpeed[j] == 45 || colSpeed[i] == 45;
        float jit = main ? 0.08f : 0.24f;
        x[k] = (i - C) + r.range(-jit, jit);
        y[k] = (j - C) + r.range(-jit, jit);
        right[k] = i + 1 < N ? rowSpeed[j] : 0;
        down[k] = j + 1 < N ? colSpeed[i] : 0;
        // some residential streets don't go through
        if (right[k] && right[k] != 45 && r.chance(0.2f)) right[k] = 0;
        if (down[k] && down[k] != 45 && r.chance(0.2f)) down[k] = 0;
    }
    x[store()] = y[store()] = 0;
    // nobody gets cut off: any crossing that can't be reached gets its streets back
    for (int pass = 0; pass < 6; pass++) {
        bool all = true;
        for (int k = 0; k < N * N; k++) {
            if (minutes(store(), k) < 1e8f) continue;
            all = false;
            int i = k % N, j = k / N;
            if (i + 1 < N) right[k] = rowSpeed[j];
            if (j + 1 < N) down[k] = colSpeed[i];
            if (i > 0) right[k - 1] = rowSpeed[j];
            if (j > 0) down[k - N] = colSpeed[i];
        }
        if (all) break;
    }
}

float RoadMap::minutes(int a, int b, std::vector<int>* path) const {
    const int V = N * N;
    std::vector<float> dist(V, 1e9f);
    std::vector<int> prev(V, -1);
    std::vector<char> done(V, 0);
    dist[a] = 0;
    for (int it = 0; it < V; it++) {
        int u = -1;
        for (int k = 0; k < V; k++) if (!done[k] && (u < 0 || dist[k] < dist[u])) u = k;
        if (u < 0 || dist[u] >= 1e9f) break;
        done[u] = 1;
        if (u == b) break;
        auto relax = [&](int v, unsigned char mph) {
            if (!mph) return;
            float len = std::sqrt((x[u] - x[v]) * (x[u] - x[v]) + (y[u] - y[v]) * (y[u] - y[v]));
            float t = dist[u] + len / mph * 60 + 0.15f;   // (a little for every crossing: stop signs, lights)
            if (t < dist[v]) { dist[v] = t; prev[v] = u; }
        };
        int i = u % N, j = u / N;
        if (i + 1 < N) relax(u + 1, right[u]);
        if (i > 0) relax(u - 1, right[u - 1]);
        if (j + 1 < N) relax(u + N, down[u]);
        if (j > 0) relax(u - N, down[u - N]);
    }
    if (path) {
        path->clear();
        if (dist[b] < 1e9f) { for (int k = b; k >= 0; k = prev[k]) path->push_back(k); std::reverse(path->begin(), path->end()); }
    }
    return dist[b];
}

float RoadMap::miles(int k) const { return std::sqrt(x[k] * x[k] + y[k] * y[k]); }
int RoadMap::randomAddress(Rng& r) const {
    for (int t = 0; t < 400; t++) {
        int k = r.below(N * N);
        float d = miles(k);
        if (k == store() || d > 10 || d < 0.7f) continue;
        float w = 1.05f - d / 11;
        if (r.f() < w * w) return k;   // most people live close; a few are right out at the edge
    }
    return store() + 1;
}

}  // namespace pz
