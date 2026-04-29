# Texas Hold'em Odds Calculator

A C++17 Texas Hold'em odds calculator with a dependency-light poker engine, an
interactive console app, a core test suite, and an optional Raylib 3D UI.

The calculator estimates win and tie percentages with Monte Carlo simulation.
It supports 2-6 players, known or unknown hole cards, and 0-5 known community
cards.

## Features

- Texas Hold'em odds simulation for 2-6 players
- Known and unknown hole-card support
- Partial or complete board support
- Duplicate-card validation
- Seven-card poker hand evaluator
- Reproducible simulations through a seeded API
- Console interface with simple card input
- Optional Raylib UI with card assets, table rendering, animated dealing, chip
  stacks, async simulation, and orbit camera controls

## Requirements

- C++17 compiler, such as `g++` or `clang++`
- `make`
- Raylib and `pkg-config`, only for the optional UI target

Raylib install examples:

```sh
# macOS
brew install raylib pkg-config

# Ubuntu/Debian
sudo apt install libraylib-dev pkg-config

# Windows/MSYS2 UCRT64
pacman -S mingw-w64-ucrt-x86_64-raylib pkgconf
```

The UI build first uses Raylib's `pkg-config` metadata when available. If that
is not present, the Makefile falls back to common macOS, Linux, and generic
link flags.

## Quick Start

Build and run the console calculator:

```sh
make engine
./poker_engine 10000
```

The optional argument is the number of Monte Carlo simulations. If it is
omitted, the console app uses `10000`.

Run the core tests:

```sh
make test
```

Build and run the Raylib UI:

```sh
make ui
./poker_ui
```

## Build Targets

```sh
make engine        # build ./poker_engine
make test          # build and run ./poker_tests
make ui            # build ./poker_ui after checking Raylib headers
make clean         # remove generated binaries
```

Running `make` with no target builds the console engine.

## Console Usage

Run the calculator with an optional simulation count:

```sh
./poker_engine 50000
```

The program prompts for:

- player count, clamped to 2-6
- two hole cards per player
- 0-5 known board cards

It then prints each player's estimated win and tie percentages.

Example card input:

```text
Players (2-6): 2

Player 1
  Card 1: AS
  Card 2: AH

Player 2
  Card 1: KS
  Card 2: KH

Known board cards (0-5): 0
```

## Card Notation

Cards use two-character notation:

- `AS`: ace of spades
- `TD`: ten of diamonds
- `7H`: seven of hearts
- `??`: unknown card

Ranks are `2`-`9`, `T`, `J`, `Q`, `K`, `A`.
Suits are `C`, `D`, `H`, `S`.

Duplicate known cards are rejected during validation.

## Raylib UI Controls

- Left click: select card slots, buttons, and picker cards
- Right drag over the table: orbit the camera
- Mouse wheel over the table: zoom
- Shift + right drag, or middle drag: pan the camera target
- `W/A/S/D`: keyboard camera orbit
- `R`: reset camera

## Core API

The main public API is declared in `src/core/simulation.h`:

```cpp
bool validateGameState(const GameState &state, char error[128]);
OddsResult calculateOdds(GameState state, int simulations);
OddsResult calculateOdds(GameState state, int simulations, unsigned int seed);
```

Use the seeded overload for deterministic tests and reproducible Monte Carlo
results.

Important data types are declared in `src/core/game_state.h`:

```cpp
const int MaxPlayers = 6;
const int BoardCards = 5;
const int HoleCards = 2;

struct GameState {
    PlayerHand players[MaxPlayers];
    Card board[BoardCards];
    int playerCount;
    int boardCount;
};
```

The core engine does not depend on Raylib.

## Project Layout

```text
Makefile                     Build targets for console, tests, and UI
src/main_console.cpp         Interactive console calculator
src/main_raylib.cpp          Raylib app loop
src/core/                    Card model, deck, evaluator, game state, simulation
src/ui/                      Raylib UI helpers and rendering
tests/core_tests.cpp         Core unit-style tests
assets/cards/                Bundled playing card PNGs
assets/fonts/                Bundled Inter font
assets/attribution/          Asset licenses and attribution
```

## Current Scope

Implemented:

- 2-6 active players
- known or unknown hole cards
- 0-5 known board cards
- duplicate-card validation
- Monte Carlo win and tie percentages
- seven-card hand evaluation
- console calculator
- Raylib 3D UI

Not included:

- betting
- AI opponents
- save files
- networking
- real-money or gameplay mechanics

## Assets

Playing card PNGs are bundled in `assets/cards/`.
The Inter font is bundled in `assets/fonts/Inter.ttf`.
Attribution and license details are in `assets/attribution/`.
