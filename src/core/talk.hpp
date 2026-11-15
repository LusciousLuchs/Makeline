// Makeline - customers: who they are, what they want, how they say it, and what they write about you afterwards.
#pragma once
#include "pizza.hpp"
#include <string>
#include <vector>

namespace pz {
struct Ctx;   // the situation a line is said in (speech.hpp)

struct Person {
    std::string name;
    uint32_t look = 0;    // seeds their clothes and build
    int mood = 0;         // 0 in a hurry, 1 mumbler, 2 can't decide, 3 entitled
    int age = 35, sex = 0;                    // sex 0 a man, 1 a woman
    int skin = 0;                             // 0 lightest .. 5 darkest
    int hair = 0, hairStyle = 1;              // colour: 0 brown, 1 black, 2 blond, 3 red, 4 grey, 5 white; style: 0 bald, 1 short, 2 bun, 3 long, 4 cap
};
Person makePerson(Rng& rng);
Spec randomPizza(Rng& rng, int day);
std::vector<Spec> randomOrder(Rng& rng, int day);

// the lines a customer says to place their order, in the order they say them (changes of mind and all)
std::vector<std::string> orderScript(Rng& rng, const Person& who, const std::vector<Spec>& want, int service, bool phone, int day, int hour = 17);
// said again, annoyed, with no detours
std::string recap(Rng& rng, const std::vector<Spec>& want, int service, bool phone, const Ctx* ctx = nullptr);
// their answer to having the entered order read back; `ok` when it's what they asked for
std::string readBack(Rng& rng, const std::vector<Spec>& want, const std::vector<Spec>& entered, int wantService, int enteredService, bool phone, bool& ok, const Ctx* ctx = nullptr);
std::string speakPizza(Rng& rng, const Spec& s, bool inches);
std::string waitingBark(Rng& rng, float patience, const Ctx* ctx = nullptr);
// The customer's temper in their words: below about 0.35 anger a line is left alone, then it picks up some cursing
// (mild first, the real thing past about 0.65). Does nothing while `strongLanguage` is off.
extern bool strongLanguage;
std::string heat(Rng& rng, const std::string& line, float anger);
std::string reviewText(Rng& rng, const Score& score, int stars);
std::string issueLine(const Issue& is);   // the plain reason, for the day's report: "Too little Pepperoni"
std::string streetAddress(Rng& rng);

}  // namespace pz
