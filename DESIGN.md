# Makeline: design document

*Working title. An OAB Foundry game. First-person 3D, raylib / OpenGL, Windows.*

## The pitch

You are the **manager of a fast-food pizza place**. You take the orders, make the pizzas, cut them, box them and
hand them over, while impatient, rude customers pile up at the counter and on the phone. Every order can turn into
a review. Keep the store's review score up or corporate reprimands you. Get reprimanded too many times in a row
and you're fired.

It's built from five years on a real makeline: the point is that it feels right to anyone who has worked one.

## Pillars

1. **Mistakes are yours to make.** Nothing is "press F to do the job". You can mishear an order, grab the wrong
   dough, over-sauce, forget a topping, put the pepperoni on the wrong half, or cut a thin crust like a pie.
2. **Portions matter.** Every ingredient has a set portion per size. Too little or too much and the customer notices.
3. **Customers are impatient and rude.** They mumble, change their minds, hate repeating themselves and write
   reviews when they're angry.
4. **The review score is the game.** Everything you do right or wrong ends up there.

## The job

### Being fired (the lose condition)

- You get reprimanded two ways: for the store's score, and for how you treat your staff (see Staff: HR complaints).
- At closing each day, corporate looks at the **store's review score** (the average of the last 30 reviews).
- Below the threshold (**3.0 stars**, to be tuned), you get a **reprimand**.
- **Three reprimands inside any 7 days and you're fired.** Game over: the save is closed out and you start again
  at a new store.
- Reprimands older than a week stop counting, so a bad patch can be recovered from.
- The end-of-day screen shows the reprimand letter and how many you're on (1 of 3, 2 of 3).
- No reprimand on a day with no reviews at all.

### A shift

One day is the dinner rush, 5:00 to 6:00 PM, about 12 minutes of play. After
closing you finish what's still in the store, then get the day's report: orders, walk-outs, sales, tips, every
review with what went wrong, and corporate's verdict.

Days get busier and orders get harder: day 1 is one-topping pizzas with portion guides on; later days bring
half-and-half, light and extra, several pizzas an order, customers who change their mind, and bad phone lines.

### The store (3D, first person)

You walk around behind the counter. Each station is a place in the store:

| Station | Where | What you do |
|---|---|---|
| Register | The counter | Take walk-in orders, hand finished orders over |
| Phone | The counter | Answer before they hang up; carryout or delivery |
| Makeline | Left wall | Dough, sauce, cheese, toppings |
| Oven | Back wall | Conveyor oven: in one end, out the other |
| Cut table | End of the oven | Cut and box |
| Heat rack | Right wall | Boxed orders wait here for the customer or a driver |

### Taking orders

- Customers order **at the counter** or **by phone**, in text dialogue, at their own speed.
- You enter what they say on the order screen: size, crust, sauce and how much, cheese, each topping (whole, left
  or right half; light, normal or extra), how many pizzas, and carryout or delivery for phone orders.
- **What you enter is what gets made.** The customer's real order is never shown to you.
- They say sizes in inches, change their mind mid-order, and the phone cuts out.
- You can ask them to **repeat** it or **read the order back**. Both cost patience, and they let you know.
- Phones left ringing and customers left standing in line give up, and may review you for it.

### Making pizza

- Pick the ticket, grab the right dough (size and crust), then build it by hand:
  ladle the sauce on and spread it, sprinkle cheese, sprinkle or place toppings. Toppings can be picked back off.
- **Sizes:** Small 10", Medium 12", Large 14", X-Large 16". **Crusts:** hand tossed, thin (M, L), pan (M).
- **Sauces:** marinara, alfredo, garlic parm, barbecue.
- **Cheeses:** shredded mozzarella, mozzarella balls, provolone, cheddar, parmesan, feta.
- **Vegetables:** onions, green peppers, pineapple, tomatoes, mushrooms, black olives.
- **Meats:** Italian sausage, pepperoni, Canadian bacon, bacon, beef, chicken.
- **Portions:** a chart per size. Light is half, extra is one and a half, a topping on one half gets half.
  About a fifth either way is acceptable. It has to be spread evenly.
- Portion guides are shown on day 1 (training). After that you're expected to know the chart.

### Oven, cutting, boxing

- The conveyor oven takes a fixed time. Pizzas that sit after coming out go cold.
- Cut with the rocker knife: pie cut (Small 6 slices, the rest 8), **thin crust in squares**.
- Box it. When every pizza on an order is boxed, the order is ready.

### Handing over

- Counter and carryout customers wait in the lobby: hand them their order.
- Deliveries leave with a driver (see Delivery).
- Entering carryout when they said delivery (or the reverse) means they never get their pizza.

## The order timer

Every order has a timer from the moment it's rung up, shown on its ticket, on the order screen and over the
customer's head. The store runs on the same times a real one does:

| Timer when the customer gets it | Counts as |
|---|---|
| Over 20 minutes | Late |
| Over 30 minutes | Very late |
| Over 45 minutes | Extreme: one star, whatever the pizza was like |

A shift is the dinner rush, 5:00 to 6:00 on the shop clock (one clock minute is twelve seconds of play).

## Staff

You're the manager: you can do every job yourself, or hire people to do them.

- **Customer service** take orders at the counter and on the phone, and hand finished orders over.
- **Makeline** make the pizzas, and cut and box what comes out of the oven.
- **Drivers** deliver. The store starts with one.
- New applications land on your desk every morning. Each person has an age, a look, a wage they want, and a
  skill you only get a hint of at the interview. Good people cost more.
- **Staff make mistakes**, by skill: orders rung up wrong, portions off, a topping forgotten, a sloppy cut. Their
  mistakes land on the store's score like yours do, and the day's report names who did what.
- **Talking to them and writing them up.** Two things you can do when someone messes up (or when they haven't):
  - **Talk to them** about what they're doing: the light option. A small nudge, a small cost.
  - **Write them up:** the heavy option. It goes in their file.
  - Neither is a sure thing. It can **improve their behaviour** (more careful for a few days), or they can
    **do worse**: resentful, sloppier, slower. Which way it goes depends on the person, how happy they already
    are, whether they actually did something wrong, and how often you've been on them lately.
  - Deserved and rare works best. Undeserved, or the third time this week, mostly backfires.
- **Being hounded out.** Talks and write-ups back to back wear a person down faster than anything else. Push
  someone far enough and they quit on the spot.
- **HR complaints.** Someone who quits after you've been on their case may file a **complaint about you** with HR.
  The more you piled on, and the less of it was deserved, the more likely the complaint.
- **Upper management.** An HR complaint lands on the regional office's desk, and they can **reprimand you** for it.
  It's the same kind of reprimand as a bad review score: it counts toward the three that get you fired.
- **Happiness:** unhappy staff do worse work, and anyone miserable at closing quits.
  - Drivers who make no tips get unhappy. Tips come from on-time, correct deliveries.
  - Customer service hate being yelled at: every rude or fed-up customer they serve wears them down.
  - Makeline get worn down by being slammed all night.
- Wages come out of the till every night.
- Customers and staff vary in age, sex, ethnic background, names and looks.

## Delivery

- A **road map of the delivery area is generated fresh every shift**: the store in the middle, addresses up to
  10 miles out. Main roads are 45 mph; residential streets are 25 to 35 and some don't go through.
- When a delivery is boxed, a driver who's in the store takes it, with up to two more that are ready.
- The run goes nearest door first. Drive time comes from the real route and its speed limits, each door adds a
  couple of minutes, and the driver has to drive back before they can take another run.
- Better drivers are quicker. The order timer keeps running until the pizza is at the door.
- The manager's office shows the map live: every waiting door, every driver on the road.

## Reviews

Every finished or failed order is judged against what the customer **actually** asked for.

| What went wrong | How bad |
|---|---|
| Never served, nobody answered, pickup/delivery mixed up, over 45 minutes | Worst |
| A whole pizza missing; wrong size; wrong sauce; a topping missing; a topping they didn't order | Serious |
| Wrong crust; very late (over 30 minutes) | Serious |
| Too little or too much of something; not cut; cold; late (over 20 minutes); topping over the wrong half | Moderate |
| Sauce or toppings not spread; sloppy or wrong kind of cut; lukewarm | Minor |

- One minor slip still gets five stars. Problems add up to 4, 3, 2 or 1 star.
- Angry customers nearly always review. Happy ones only sometimes.
- Two stars or less means a refund. Five stars brings a tip.
- The review text says what they hated, in their own rude words. The day's report also lists the plain reasons.

## Look and sound

- Indie low-poly 3D with flat colours and warm light. A red-and-cream fast-food lobby, steel kitchen.
- The order screen, tickets and reports are clean 2D screens over the 3D view.
- Placeholder sounds are synthesised (phone, door chime, oven beep). Real recordings later.

## Technical

- C++17, raylib 5.5 (OpenGL 3.3), CMake. Windows build; the rules also build and are tested on Linux.
- `src/core`: the rules with no graphics (menu, portions, judging, customers and dialogue, the shift). Tested.
- `src/game`: the 3D store, stations, screens, sound, updater.
- **Its own GitHub repository**, with releases published automatically when the version goes up.
- **Self-updating:** the game checks the repository's newest release at start, downloads and installs it.
- One save: day, money, reviews, reprimands, staff.

## Later

- Scheduling staff by day, raises, praise, training.
- Sides and drinks, coupons, "the customer says their last order was wrong".
- Stock and prep: running out of cheese mid-rush, dough proofing.
- Inspections, a store you can upgrade, more stores.
- Carrying pizzas by hand between stations; delivery runs you drive yourself.

## Open questions for the owner

1. The reprimand threshold: 3.0 stars?
   And the count: built as "3 reprimands within 7 days". If you meant three days in a row only, say so.
2. Fired means the save starts over at a new store. Or should it be a hard game over?
3. The name. "Makeline" is a placeholder.
4. HR reprimands: written here as counting toward the same three-in-seven-days that gets you fired. Or should
   they be a separate count?
5. Should a complaint always mean a reprimand, or should upper management sometimes side with you when the
   write-ups were deserved?
