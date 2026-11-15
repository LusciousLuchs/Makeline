// Makeline - player actions as commands. The groundwork for multiplayer.
//
// Every thing a player does to the shop (take an order, start a pizza, drop a topping, hand an order over, write
// someone up...) is one `Command`. A command is plain data: it can be sent over a network, written to a log and
// replayed, and `apply` is the only place it changes the shop. The plan (see DESIGN.md, Multiplayer): one machine
// hosts the shop and runs `Store::update`; every player, the host included, sends commands to it; the host applies
// them in order and sends everyone the results. Because the shop only changes through `update` and `apply`, two
// copies that see the same seed, the same time steps and the same commands stay identical (tested).
#pragma once
#include "store.hpp"
#include <string>
#include <vector>

namespace pz {

enum CmdType {
    CMD_TAKE_ORDER,      // a: customer id               start talking to them (the phone, or the next one in line)
    CMD_ASK_REPEAT,      //                               "could you repeat that?"
    CMD_ASK_READBACK,    //                               read the order back
    CMD_SET_DRAFT,       // a: service, text: pizzas     what's entered on the order screen so far
    CMD_SEND_ORDER,      //                               send the entered order to the makeline
    CMD_HAND_OUT,        // a: customer id
    CMD_START_PIZZA,     // a: ticket, b: size, c: crust  pull a dough/crust for a ticket onto the bench
    CMD_SAUCE,           // a: pizza, b: sauce, x y: spot, z: ounces
    CMD_BIT,             // a: pizza, b: ingredient, x y: spot
    CMD_REMOVE_NEAR,     // a: pizza, x y: spot, z: radius
    CMD_TO_OVEN,
    CMD_TRASH_BENCH,
    CMD_TO_CUT,          // a: pizza                      pull a baked pizza onto the cut table
    CMD_CUT,             // x y: from, z w: to            a stroke of the cutter
    CMD_CLEAR_CUTS,
    CMD_BOX,
    CMD_TRASH_CUT,
    CMD_WRITE_UP,        // a: staff index
    CMD_HIRE,            // a: applicant index
    CMD_FIRE,            // a: staff index
    CMD_N
};

struct Command {
    int type = CMD_N;
    int player = 0;                  // who sent it (0 = the host)
    int a = 0, b = 0, c = 0;
    float x = 0, y = 0, z = 0, w = 0;
    std::string text;                // a payload, as JSON
};

// Does what the command says. False when it can't be done (it's out of date, or against the rules): the shop is
// left as it was. The host decides; a player's screen only ever shows what the host has done.
bool apply(Store& s, const Command& c);

std::string commandToJson(const Command& c);
bool commandFromJson(const std::string& text, Command& out);
std::string specsToJson(const std::vector<Spec>& v);
std::vector<Spec> specsFromJson(const std::string& text);

// A short fingerprint of everything that matters in the shop: two copies that agree give the same text.
std::string digest(const Store& s);

}  // namespace pz
