// Makeline - the shop for one shift: customers arriving, the phone, orders, tickets, the oven, the heat rack,
// drivers, and the reviews that come back. No graphics: the game draws this and calls the actions.
#pragma once
#include "talk.hpp"
#include <string>
#include <vector>

namespace pz {

enum CustState { C_RINGING, C_INLINE, C_ORDERING, C_COMING, C_WAITING, C_AWAY, C_DONE };
struct Customer {
    int id = 0;
    Person who;
    bool phone = false;
    int service = COUNTER;          // what they want
    std::vector<Spec> want;
    int state = C_INLINE;
    float patience = 1;             // 1 calm, 0 done with you
    float timer = 0;                // time in this state
    float arriveIn = 0;             // a pickup on its way
    int order = -1;
    std::string bark; float barkT = 0, nextBark = 6;   // what they're muttering in the lobby
    float leaving = 0;              // > 0: walking out (for the drawing)
    int leftStars = 0;              // how they left: 0 angry and unserved
};

enum TicketState { T_WAIT, T_MAKING, T_OVEN, T_EXIT, T_CUT, T_BOXED, T_VOID };
struct Ticket { int id, order, idx, of; Spec spec; int state = T_WAIT; int pizza = -1; };
struct Order {
    int num = 0, cust = -1;
    int service = COUNTER;          // as it was entered
    std::vector<Spec> entered;
    std::vector<int> tickets;
    double placed = 0;
    float budget = 0;               // seconds they think is reasonable
    float price = 0;
    bool ready = false, out = false, done = false;
    float tripLeft = 0;             // a delivery on the road
};
struct Review { std::string name, text; int stars = 5; int day = 1; std::vector<std::string> reasons; };

struct TalkLine { bool you; std::string text; };
struct Talk {
    int cust = -1;
    std::vector<TalkLine> log;              // everything said so far (the last customer line may still be coming out)
    std::vector<std::string> pending;       // what they're about to say
    float shown = 0;                        // letters of the last line out so far
    bool speaking = false;
    float pause = 0;
    std::vector<Spec> draft;                // the order as you're entering it
    int draftService = CARRYOUT;
    int cur = 0;                            // which pizza of the draft is on screen
    bool active() const { return cust >= 0; }
};

enum EvKind { E_RING, E_DOOR, E_REVIEW, E_OVEN_OUT, E_READY, E_DRIVER, E_STORMED, E_MISSED, E_HANDED, E_TALK, E_PLACED, E_DELIVERED };
struct Event { int kind; int a; std::string text; };

constexpr float OVEN_TIME = 24;
constexpr float SHIFT_LEN = 420;

struct OvenSlot { int pizza; float prog; };

struct Store {
    // ---- kept between days
    int day = 1;
    float money = 0;
    std::vector<Review> reviews;
    bool guides = true;             // portion guides on the makeline (training wheels)

    // ---- this shift
    double t = 0;
    bool closed = false, dayOver = false;
    Rng rng{1};
    std::vector<Customer> custs;
    std::vector<Order> orders;
    std::vector<Ticket> tickets;
    std::vector<Pizza> pizzas;
    Talk talk;
    int bench = -1, cutBoard = -1;
    std::vector<OvenSlot> oven;
    std::vector<int> exitQ;
    int driversFree = 2;
    std::vector<float> driversBack;
    double nextArrival = 4;
    std::vector<Event> events;
    std::vector<Review> dayReviews;
    float dayMoney = 0, dayTips = 0;
    int dayOrders = 0, dayLost = 0;

    void startDay(uint64_t seed);
    void update(float dt);
    float rating() const;           // the last 30 reviews; 0 when there are none yet
    std::string clock() const;      // "6:42 PM"

    // ---- the front
    int ringing() const;            // the customer on the ringing phone, or -1
    int firstInLine() const;
    std::vector<int> line() const;  // customers in line, first first
    std::vector<int> lobby() const; // customers waiting for their food
    bool beginTalk(int cust);       // answer the phone / take the next customer
    void askRepeat();
    void askReadBack();
    bool finishOrder();             // sends the draft to the makeline
    bool handOut(int cust);         // their order is ready and they're here

    // ---- the makeline, oven and cut table
    std::vector<int> openTickets() const;      // waiting to be made, oldest first
    bool startPizza(int ticket, int size, int crust);
    bool benchToOven();
    void trashBench();
    bool ovenHasRoom() const;
    bool toCutBoard(int pizza);
    bool boxCut();
    void trashCut();

    Customer* cust(int id);
    Order* order(int num);
    Ticket* ticket(int id);
    Pizza* pizza(int id);
    const Order* orderOf(const Pizza& p) const;

    std::string toJson() const;
    bool fromJson(const std::string& text);

private:
    void spawn();
    void resolve(Order& o, std::vector<Issue> extra = {});
    void lose(Customer& c, int code);
    void review(const Customer& c, const Score& sc, int stars);
    void say(const std::string& line);
    int nextCust = 1, nextOrder = 1, nextTicket = 1;
};

// plays the shift by itself (tests and screenshots): `care` 1 = everything right, 0 = a disaster
void botStep(Store& s, float dt, float care, Rng& rng);

}  // namespace pz
