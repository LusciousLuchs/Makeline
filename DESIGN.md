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

- At closing each day, corporate looks at the **store's review score** (the average of the last 30 reviews).
- Below the threshold (**3.0 stars**, to be tuned), you get a **reprimand**.
- **Three reprimands inside any 7 days and you're fired.** Game over: the save is closed out and you start again
  at a new store.
- Reprimands older than a week stop counting, so a bad patch can be recovered from.
- The end-of-day screen shows the reprimand letter and how many you're on (1 of 3, 2 of 3).
- No reprimand on a day with no reviews at all.

### A shift

One day is one evening shift, 4 PM to 10 PM, about 7 minutes of play, with a dinner rush in the middle. After
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
- Deliveries leave with a driver (two drivers; they take time to get back).
- Entering carryout when they said delivery (or the reverse) means they never get their pizza.

## Reviews

Every finished or failed order is judged against what the customer **actually** asked for.

| What went wrong | How bad |
|---|---|
| Never served, nobody answered, pickup/delivery mixed up | Worst |
| A whole pizza missing; wrong size; wrong sauce; a topping missing; a topping they didn't order | Serious |
| Wrong crust; very slow | Serious |
| Too little or too much of something; not cut; cold; slow; topping over the wrong half | Moderate |
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
- One save: day, money, reviews, reprimands.

## Later

- Staff to hire and schedule (you are the manager): insiders, makeline, drivers, each with skill and flaws.
- Sides and drinks, coupons, "the customer says their last order was wrong".
- Stock and prep: running out of cheese mid-rush, dough proofing.
- Inspections, a store you can upgrade, more stores.
- Carrying pizzas by hand between stations; delivery runs you drive yourself.

## Open questions for the owner

1. The reprimand threshold: 3.0 stars?
   And the count: built as "3 reprimands within 7 days". If you meant three days in a row only, say so.
2. Fired means the save starts over at a new store. Or should it be a hard game over?
3. The name. "Makeline" is a placeholder.
