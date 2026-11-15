# Makeline

*Working title.* A first-person 3D game about managing a fast-food pizza place, by **OAB Foundry**.

Take orders from rude, impatient customers, make the pizzas by hand with the right portions, cut, box and hand
them over, hire and manage a crew, and keep the store's review score high enough not to get fired.

See [DESIGN.md](DESIGN.md) for the whole design and [CHANGELOG.md](CHANGELOG.md) for what's in each version.

## Playing

Download the newest zip from the Releases page, unzip it anywhere, run `makeline.exe`. The game updates itself.

| Key | |
|---|---|
| WASD, mouse | Walk and look |
| E or click | Use what you're facing (register, phone, makeline, cut table, a customer whose order is ready) |
| Shift | Hurry |
| M | Manager's office: staff, hiring, the delivery map |
| Tab | Portion chart (at the makeline) |
| Esc | Step back from a station / pause (strong language and the helper HUD switch here) |
| F11 | Fullscreen |

## Building

C++17 and CMake. raylib is included in `third_party`.

- The rules and their tests (any system): `cmake -S . -B build && cmake --build build -j && ./build/makeline_tests`
- Windows game from Linux (MinGW-w64, apt `g++-mingw-w64-x86-64-posix`):
  `cmake -S . -B build-win -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-gcc.cmake && cmake --build build-win -j --target makeline`
- Windows game on Windows: `cmake -S . -B build && cmake --build build --config Release`

## Source layout

- `src/core`: the rules, no graphics. `menu` (ingredients and portions), `pizza` (a made pizza and how it's judged),
  `talk` (customers, what they say, reviews), `roads` (the delivery map), `store` (the shift: customers, orders,
  oven, staff, drivers, reprimands, the save).
- `src/game`: the game. `world` (the 3D shop and walking), `g3d` (3D drawing kit), `front` (register and order
  screen), `makeline`, `ovencut`, `office`, `ui`, `audio`, `updater`, `main`.
- `tests`: tests of the rules, including whole shifts played by a bot and by a hired crew.

Copyright (C) 2026 The One Armed Bandit.
