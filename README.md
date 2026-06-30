# Tomogichi — Personal Development RPG

A C++17 CLI and Qt6/Kirigami GUI application for self-tracking through 4 character domains with timer-based practice logging, XP/leveling, cross-skill bonuses, entropy/sync mechanics, effect analytics, achievements, and a coin shop.

Designed for GNU/Linux phones — Librem 5 and PinePhone (ARM64). Also runs on desktop Linux.

## Quick Start

### CLI (zero dependencies beyond C++17 stdlib)

```bash
make
./tomogichi
```

```
tomo> riff 30          # start 30-min practice timer for Riff
⏱ Timer: riff drawing (planned 30 min)
tomo> timer stop       # log actual elapsed time → XP + levels
tomo> status           # show all 4 characters + guild level
tomo> stats            # weekly bar chart
tomo> stats fx         # effect analytics (which combos feel best?)
```

### Qt6 / Kirigami GUI

```bash
cd qt
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/tomogichi-qt
```

Headless test (no display needed):
```bash
QT_QPA_PLATFORM=offscreen timeout 4 ./build/tomogichi-qt
# Exit 124 = app running OK
```

## Architecture

```
src/
  core/types.h       All data structures + Effect enum
  core/engine.h      XP/level/title/bonus/mood/stats/achievements API
  core/engine.cpp    Pure C++ logic — no Qt dependencies
  data/storage.h     GameState, load/save API
  data/storage.cpp   JSON persistence (nlohmann/json single header)
  main.cpp           CLI ~1900 lines: commands + timer + display

qt/
  CMakeLists.txt     Qt 6 + KF6Kirigami
  src/main.cpp       QGuiApplication, QML engine
  src/backend.h/cpp  QObject wrapper (~170+850 lines), 35+ Q_PROPERTY
  qml/*.qml          7 files: Guild, Person, Timer, Stats, Today, Settings pages
```

Same C++ engine shared between CLI and GUI. No code duplication.

## Characters & Skills

| Person | Role | Skills (* = main, 50% weight) |
|--------|------|------|
| Riff | Designer | drawing*, sewing, lefthand |
| Reef | Architect | 3d-model*, linux, anatomy |
| Pitch | Athlete | driving, dance*, kendama |
| Rain | Regulator | ferment, massage*, meditation |

**Master** (meta-character): tasks, challenges, calendar, daily todos, mood diary, birthdays, coin shop, achievements.

## Core Mechanics

### XP & Levels
- 60 min = 100 XP. `minutes_to_xp(min) = (min * 100) / 60`
- Skill level: `floor(sqrt(total_xp / 100))`
- Person level: `floor(0.50 * main_skill_lvl + 0.25 * sec1_lvl + 0.25 * sec2_lvl)`
- Guild level: `floor(average of 4 person levels)`
- Titles: Novice(0-4) → Apprentice(5-9) → Artisan(10-14) → Expert(15-19) → Maestro(20-24) → Grandmaster(25+)

### Cross-Bonuses
8 predefined links — skill at level 10 gives +5% XP to linked skill forever:
```
riff/drawing  → reef/3d-model    reef/3d-model  → riff/drawing
pitch/dance   → rain/meditation  rain/meditation → riff/drawing
rain/massage  → reef/anatomy     reef/anatomy    → rain/massage
reef/linux    → pitch/driving    pitch/kendama   → riff/sewing
```

### Entropy & Sync
- Entropy: +5 per day with no practice, +10 if Rain hasn't practiced in 3+ days
- Rain sessions reduce entropy by -5 (meditation, massage, ferment)
- Sync: `(% of characters who practiced today) * 25` — range 0-100
- XP multiplier: `(1.0 - entropy/200) * (1.0 + sync/200)` — range 0.5x to 1.5x

### Effect Tracking
After each practice, record subjective effect: energized / neutral / tired / drained.
`stats fx` reveals correlations: which combos feel best, time-of-day patterns, activity chains.

### Shop (Coin Economy)
Coins earned from challenge completion.

| Item | Cost | Effect |
|------|------|--------|
| Entropy Reset | 50 | entropy → 0 |
| XP Boost | 20 | +50% XP for next practice |
| Skip Day | 30 | -5 entropy, prevent further increase today |
| Custom Title | 100 | Set custom title for any character |
| Coin Doubler | 40 | 2x coins from next challenge |

### Achievements (16 badges)
First Step → One Hour Club → Dedicated → Master in Making (hours)
Momentum → Weekly Warrior → Unstoppable (streaks)
Full Party (all 4 chars in one day)
Apprentice → Artisan → Maestro (skill levels)
Collector → Treasurer (coins 100/1000)
Challenger → Overachiever (challenges 5/20)

### Character Mood
| Days idle | Mood | Emoji |
|-----------|------|-------|
| 0-1 | Happy | 😊 |
| 2-3 | Neutral | 😐 |
| 4-7 | Sad | 😢 |
| 8+ | Lonely | 💔 |

### Warmth (Visual)
Color-coded freshness per skill — green (0-2 days), yellow (3-5), gray (6+).

## CLI Commands

**Practice:** `riff N`, `reef linux N`, `pitch N`, `rain med N`
**Timer:** `timer start/stop/cancel/status`
**Display:** `status`, `history [N]`, `week`, `stats`
**Stats:** `stats fx` (effect analytics), `stats` (weekly chart)
**Tasks:** `tasks`, `task add/done/undo/del`
**Daily:** `daily`, `daily add/done/undo/del`
**Calendar:** `cal`, `plan`, `plan add DAY HH:MM LABEL`, `plan del N`
**Challenges:** `chal`, `chal add DESC PERS SKILL COUNT DAYS REWARD`
**Shop:** `shop`, `buy <item>`
**Mood:** `mood [word]`, `mood list`
**Diary:** `diary [text]`
**Skills:** `skill archive/reactivate/list`
**Characters:** `rename PERSON NEWNAME`, `clist`, `clist add PERSON TEXT`, `clist done N`, `clist undo N`, `clist del N`, `clist repeat N HOURS`
**Achievements:** `ach`
**Export:** `backup`, `export csv`

## Build Requirements

### CLI
- C++17 compiler (GCC or Clang)
- No external dependencies (nlohmann/json bundled as single header)

### Qt GUI
- Qt 6 (Core, Quick, QuickControls2)
- KF6Kirigami
- CMake 3.16+

```bash
# Debian/Ubuntu/PureOS
sudo apt install build-essential cmake qt6-base-dev qt6-declarative-dev qt6-quickcontrols2-dev kf6-kirigami-dev

# Arch
sudo pacman -S qt6-base qt6-declarative qt6-quickcontrols2 kf6-kirigami cmake
```

### GCC 16 Note
GCC 16 cc1plus hangs on `-O2` — the Makefile uses `-O0` + separate `.o` compilation as workaround.

## Downstream

[PinkReader Desktop](https://github.com/MaurerAnton/pinkreader-desktop) uses the same Qt6/Kirigami architecture pattern.

## License

GPL-3.0
