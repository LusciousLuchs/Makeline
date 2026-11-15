// Makeline - the shop for one shift: customers arriving, the phone, orders, tickets, the oven, the heat rack,
// drivers, and the reviews that come back. No graphics: the game draws this and calls the actions.
#pragma once
#include "talk.hpp"
#include "roads.hpp"
#include "speech.hpp"
#include <string>
#include <vector>

namespace pz {

constexpr float OVEN_TIME = 30;
constexpr float REPRIMAND_BELOW = 3.0f;
constexpr float SEC_PER_MIN = 12;        // one minute on the shop clock, in real seconds (20 minutes late = 4 real minutes)
constexpr int OPEN_MIN = 9 * 60, CLOSE_MIN = 24 * 60;   // the store: 9 AM to midnight
constexpr int SHIFT_MINS = 8 * 60;                        // every shift the boss gives you is 8 hours
constexpr float SHIFT_LEN = SHIFT_MINS * SEC_PER_MIN;

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
    int asks = 0;                   // times you've made them repeat or read back, so they get shorter with you
    int server = -1;                // the staff member taking their order (-1: you, or nobody yet)
    int addr = -1;                  // where they live on the road map (deliveries)
};

enum TicketState { T_WAIT, T_MAKING, T_OVEN, T_EXIT, T_CUT, T_BOXED, T_VOID };
struct Ticket { int id, order, idx, of; Spec spec; int state = T_WAIT; int pizza = -1; int madeBy = -1; };
struct Order {
    int num = 0, cust = -1;
    int service = COUNTER;          // as it was entered
    std::vector<Spec> entered;
    std::vector<int> tickets;
    double placed = 0;
    float minutes(double now) const { return (float)((now - placed) / SEC_PER_MIN); }   // the order's timer
    float price = 0;
    bool ready = false, out = false, done = false;
    int takenBy = -1;               // the staff member who rang it up (-1: you)
    int driver = -1;                // who took it out
};
// ---- staff
enum Role { CSR, COOK, DRIVER, ROLE_N };
const char* roleName(int role);
struct RunStop { int order; float at; };          // minutes into the run
struct RunPoint { float x, y, at; };              // the route, with the time each point is reached
struct Worker {
    Person who;
    int role = CSR;
    float skill = 0.6f;             // 0 hopeless .. 1 flawless
    float mood = 0.7f;              // under 0.15 at closing and they quit
    float wage = 60;                // a day
    int writeUps = 0;
    int careDays = 0;               // on their best behaviour for this many more days (after a write-up)
    // today
    float tips = 0; int mistakes = 0, yelled = 0, jobs = 0; bool wroteUpToday = false;
    // what they're doing right now
    float busy = 0; int cust = -1, ticket = -1;
    bool out = false; float runT = 0, runLen = 0;
    std::vector<RunStop> stops; std::vector<RunPoint> route;
    float care() const { float c = skill + (careDays > 0 ? 0.12f : 0) - (mood < 0.3f ? 0.1f : 0); return c < 0.05f ? 0.05f : c > 0.98f ? 0.98f : c; }
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

enum EvKind { E_RING, E_DOOR, E_REVIEW, E_OVEN_OUT, E_READY, E_DRIVER, E_STORMED, E_MISSED, E_HANDED, E_TALK, E_PLACED, E_DELIVERED, E_NOTE };
struct Event { int kind; int a; std::string text; };


struct OvenSlot { int pizza; float prog; };

struct Store {
    // ---- kept between days
    int day = 1;
    float money = 0;
    std::vector<Review> reviews;
    bool guides = true;             // portion guides on the makeline (training wheels)
    std::vector<int> reprimands;    // the days corporate wrote you up
    bool fired = false;
    std::vector<Worker> staff, applicants;
    uint64_t hireSeed = 99;
    SpeechMemory speech;            // what customers said lately, so they don't repeat themselves
    bool crewSet = false;           // the store's first driver has been given

    // ---- this shift
    double t = 0;
    bool closed = false, dayOver = false;
    int shiftStart = 17 * 60;       // today's shift, in minutes after midnight (the end is 8 hours on)
    int forceStart = -1;            // (tests, screenshots) a shift start to use instead of the boss's schedule
    uint64_t schedSeed = 1; bool schedSet = false;
    Rng rng{1};
    std::vector<Customer> custs;
    std::vector<Order> orders;
    std::vector<Ticket> tickets;
    std::vector<Pizza> pizzas;
    Talk talk;
    int bench = -1, cutBoard = -1;
    std::vector<OvenSlot> oven;
    std::vector<int> exitQ;
    RoadMap roads;
    std::vector<std::string> dayNotes;   // what happened with the staff today, for the report
    float dayWages = 0;
    double nextArrival = 4;
    std::vector<Event> events;
    std::vector<Review> dayReviews;
    float dayMoney = 0, dayTips = 0;
    int dayOrders = 0, dayLost = 0;

    void startDay(uint64_t seed);
    void update(float dt);
    // the boss's schedule: a random 8-hour shift somewhere between open and close, in half-hour steps
    int shiftFor(int day) const;
    int shiftEnd() const { return shiftStart + SHIFT_MINS; }
    float clockMin() const { return shiftStart + (float)(std::min(t, (double)SHIFT_LEN + 600) / SEC_PER_MIN); }
    bool idle() const;              // nobody here, nothing on the line: the game skips ahead
    static std::string timeText(int minutesAfterMidnight);   // "7:30 PM"
    float rating() const;           // the last 30 reviews; 0 when there are none yet
    // closing the books on today: 0 nothing, 1 a reprimand (the score closed under REPRIMAND_BELOW), 2 fired
    // (three reprimands inside a week). Call once when the day is over.
    int closeDay();
    int reprimandsThisWeek() const;

    // ---- staff
    void newApplicants();           // a fresh stack of applications (each morning)
    bool hire(int applicant);
    void fireWorker(int i);
    void writeUp(int i);            // a telling-off: sharper for a couple of days, and less happy
    int driversIn() const;
    void note(const std::string& s);   // something for the day's report
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
    Ctx voice(const Customer& c) const;
    void place(Customer& c, const std::vector<Spec>& entered, int service, int takenBy);
    void staffStep(float dt);
    void sendDriver(int w);

    int nextCust = 1, nextOrder = 1, nextTicket = 1;
};

// plays the shift by itself (tests and screenshots): `care` 1 = everything right, 0 = a disaster
void botStep(Store& s, float dt, float care, Rng& rng);

}  // namespace pz
