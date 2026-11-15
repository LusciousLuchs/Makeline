// Makeline - what customers say, built on the spot (no recordings, no language model).
//
// A small grammar: each thing a customer can say (a greeting, an aside, a complaint about the wait, a reaction to
// being asked to repeat themselves...) has many templates. A template can be limited to when it makes sense (a tag
// such as <angry>, <old>, <slow>, <phone>), can pull in other templates ({tic}, {mins}, {aside}) and can contain
// quick choices ([this|that]). Each customer has a voice of their own (polite or rude, in a hurry, mumbling,
// unsure, a verbal tic) from who they are, and the shop remembers what was said lately so the same line doesn't
// come round twice in a row.
#pragma once
#include "talk.hpp"
#include <deque>
#include <map>
#include <string>

namespace pz {

struct SpeechMemory { std::deque<uint32_t> recent; };
extern SpeechMemory* speechMem;     // the shop in charge right now (so two copies of a shop don't share a memory)
struct UseSpeechMemory {
    SpeechMemory* prev;
    explicit UseSpeechMemory(SpeechMemory& m) : prev(speechMem) { speechMem = &m; }
    ~UseSpeechMemory() { speechMem = prev; }
};

struct Ctx {
    const Person* who = nullptr;
    float anger = 0;          // 0 calm .. 1 furious
    float minutes = 0;        // how long their order has been running (the order timer)
    int service = COUNTER;
    bool phone = false;
    int hour = 17;            // the shop clock
    int asks = 0;             // times they've been made to repeat or correct themselves
    std::map<std::string, std::string> v;   // named values for {v:name}
};

std::string gen(Rng& r, const std::string& symbol, const Ctx& c);
std::string genRaw(Rng& r, const std::string& symbol, const Ctx& c);   // like gen, but not tidied (for pieces of a sentence)
std::string tidyText(const std::string& s);                           // capital letters and spacing
std::string article(const std::string& nextWord);          // "a" or "an"
std::string minutesText(float minutes);                    // "about ten minutes", "twenty-odd minutes"

}  // namespace pz
