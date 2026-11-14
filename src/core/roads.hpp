// Makeline - the delivery area: a road map made fresh each shift. The store sits in the middle; addresses are up to
// 10 miles out. Main roads are fast, residential streets slow, and some streets don't go through.
#pragma once
#include "pizza.hpp"
#include <vector>

namespace pz {

struct RoadMap {
    static const int N = 21;                   // crossings each way, about a mile apart
    float x[N * N], y[N * N];                  // miles from the store (east, south)
    unsigned char right[N * N], down[N * N];   // the road to the next crossing east / south: its speed limit, 0 = no road
    int store() const { return (N / 2) * N + N / 2; }
    void generate(Rng& rng);
    // the quickest drive between two crossings, in minutes at the limit; the crossings passed, if asked for
    float minutes(int a, int b, std::vector<int>* path = nullptr) const;
    int randomAddress(Rng& rng) const;         // somewhere a customer lives: mostly close, sometimes right at the edge
    float miles(int node) const;               // straight-line, from the store
};

}  // namespace pz
