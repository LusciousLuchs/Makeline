// Makeline - customers: who they are, what they want, how they say it, and what they write about you afterwards.
#pragma once
#include "pizza.hpp"
#include <string>
#include <vector>

namespace pz {

struct Person {
    std::string name;
    uint32_t look = 0;    // seeds their face and clothes
    int mood = 0;         // 0 in a hurry, 1 mumbler, 2 can't decide, 3 entitled
};
Person makePerson(Rng& rng);
Spec randomPizza(Rng& rng, int day);
std::vector<Spec> randomOrder(Rng& rng, int day);

// the lines a customer says to place their order, in the order they say them (changes of mind and all)
std::vector<std::string> orderScript(Rng& rng, const Person& who, const std::vector<Spec>& want, int service, bool phone, int day);
// said again, annoyed, with no detours
std::string recap(Rng& rng, const std::vector<Spec>& want, int service, bool phone);
// their answer to having the entered order read back; `ok` when it's what they asked for
std::string readBack(Rng& rng, const std::vector<Spec>& want, const std::vector<Spec>& entered, int wantService, int enteredService, bool phone, bool& ok);
std::string speakPizza(Rng& rng, const Spec& s, bool inches);
std::string waitingBark(Rng& rng, float patience);
std::string reviewText(Rng& rng, const Score& score, int stars);
std::string issueLine(const Issue& is);   // the plain reason, for the day's report: "Too little Pepperoni"
std::string streetAddress(Rng& rng);

}  // namespace pz
