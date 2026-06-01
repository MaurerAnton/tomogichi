# Changelog

## 2026-06-01 — v0.4.3

### Practice Notes
- **Optional text notes** per practice session — attach a note after logging effect
- **CLI**: "Note? (Enter to skip)" prompt after effect selection
- **Qt GUI**: TextField in effect picker overlay for note input
- **History**: 📝 indicator on entries with notes in both CLI and StatsPage
- New `logEffectNote(effect, note)` slot in Backend

## 2026-05-31 — v0.4.2

### UX Polish
- **Timer chip** — floating ⏱ indicator in top-right when timer is running
- **Person XP** — total XP shown on PersonPage header
- **Chains help** — better empty-state text explaining how chains work
- **Coin Doubler** — clarified description "×2 coins from next completed challenge"
- **Deploy docs** — updated cross-compilation notes for GCC 15+

## 2026-05-31 — v0.4.1

### Code Quality
- **Zero compiler warnings** — fixed all missing initializers, unused variables
- **Removed dead code** — unused `effect_name()` function
- **Neglect banner** — now refreshes on historyChanged too
- **CLI help** — added `shop` and `buy` commands to help text

## 2026-05-31 — v0.4.0

### Deployment
- **Flatpak manifest** — `org.tomogichi.qt.json` for KDE runtime 6.7
- **Desktop file** — `org.tomogichi.qt.desktop` with mobile form factor
- **Install rules** — CMake installs binary + desktop file + changelog
- **DEPLOY.md** — instructions for Flatpak, native build, cross-compile

## 2026-05-31 — v0.3.5

### CLI
- **buy command** — purchase shop items from CLI (`buy xp_boost`, `buy entropy_reset`, etc.)
- **--version / --help** — command-line flags
- **Entropy/sync in stats** — shows entropy, sync %, coins in stats output

## 2026-05-31 — v0.3.4

### CLI parity
- **Entropy/Sync in status** — shows entropy, sync %, coins, boost indicators
- **shop command** — lists all 5 items with costs and descriptions

## 2026-05-31 — v0.3.3

### Fixes
- **Onboarding** — uses totalSessions (not capped practiceHistory) to detect first launch
- **CSV export** — uses absolute path from state.json directory (not relative)
- **Empty states** — Stats tabs show "No data yet" hints when no practice recorded
- **Session counter** — totalSessions property for accurate first-launch detection

## 2026-05-31 — v0.3.2

### Polish
- **Shop feedback** — shows "Not enough coins!" when buy fails
- **Todo progress** — "3/5 done" counter on TodayPage
- **Entropy bar** — always visible (shows 0/0 instead of hiding)
- **Task rename** — taskRename slot for editing task text
- **Calendar** — fixed first-day offset with proper empty cells

## 2026-05-31 — v0.3.1

### Polish
- **Onboarding** — welcome screen only on first launch (seen flag persisted)
- **Today's practice** — last 3 sessions shown on GuildPage with effect emoji
- **Challenge dialog** — person/skill now dropdowns (not free text)
- **Crash recovery** — auto-restore from .bak if state.json corrupted
- **Practiced-today ✓** — green checkmark on guild cards for characters who practiced
- **Undo effect** — redoEffect slot for correcting wrong effect choice

## 2026-05-31 — v0.3.0

### Polish & Bugfixes
- **Calendar heatmap** — fixed first-day offset (proper empty cell Repeater)
- **Delete confirmation** — all destructive deletes show confirmation dialog
- **Neglect banner** — refreshes dynamically after practice
- **Custom title** — user inputs title text (not just "★ Custom ★")
- **Boost indicators** — XP Boost / Coin Doubler active badges on GuildPage
- **Challenge delete** — ✕ button with confirmation on each challenge
- **CSV export** — Settings→System→Export CSV with effect column
- **Active boost tracking** — xpBoostActive/coinsDoubleActive QML properties

## 2026-05-31 — v0.2.1

### Improvements
- **Timer flow** — after stop+effect, stays on PersonPage (not popping to Guild)
- **Quick-start ▶** — round button on each guild card starts default skill timer in 1 tap
- **Neglect warning** — red banner on GuildPage when character hasn't practiced
- **Weekly stats** — "This week: X.Xh / Today: Xm" on GuildPage
- **Mood history** — last 30 mood entries visible in Journal
- **Archived skills** — visible in PersonPage with ↩ restore button
- **Coin toast** — 🏆 +N 💰 toast when challenge completed
- **Theme toggle** — System/Light/Dark in Settings→System
- **Coin Doubler** — fully implemented, doubles coins from next challenge
- **Merged Diary+Mood** into Journal tab (Settings: 8→6 tabs)

## 2026-05-31 — v0.2.0

### Core Mechanics
- **Effect tracking** — after each practice, log energized/neutral/tired/drained
- **Effect analytics** — per-skill %, time-of-day patterns, activity chains
- **Entropy + Sync** — entropy accumulates without practice, Rain reduces it, sync bonus for full-team days
- **XP multiplier** = (1.0 - entropy/200) × (1.0 + sync/200)
- **XP Boost** shop item — +50% XP on next practice
- **Coin Doubler** shop item — doubles coins from next challenge

### Qt Quick + Kirigami GUI
- 5 screens + Settings with 6 tabs
- GuildPage: 4 character cards, quick-start, coins, entropy/sync, neglect warning, weekly stats
- PersonPage: skill list, add/archive, archived skills with restore
- TimerPage: large display, overdue, stop→effect picker
- StatsPage: 5 tabs (Skills/Time/Chains/History/Week)
- TodayPage: monthly heatmap, schedule, todos, birthdays
- Settings: Tasks, Challenges, Journal (mood+diary), Archive, Shop, Birthdays, System + theme

### Shop (5 items)
- Entropy Reset (50💰), XP Boost (20💰), Skip Day (30💰), Custom Title (100💰), Coin Doubler (40💰)

### Known Issues
- Calendar heatmap first-day offset may render incorrectly on some QML versions
- Custom Title sets "★ Custom ★" (not user-input)
- No aarch64 cross-compilation yet
- No Flatpak packaging
