# Research Summary: timeto.me & IdleRPG / Idle Fantasy

## Research Date: May 31, 2026

---

## 1. timeto.me

### Overview
- **GitHub**: Medvedev91/timeto.me (645★, 43 forks, GPL-3.0)
- **F-Droid**: me.timeto.app
- **Platforms**: iOS, Google Play, F-Droid
- **Language**: Kotlin (Multiplatform)
- **Tagline**: "Goals, checklists, tasks, repetitive tasks, calendar and 24/7 time tracking."
- **Homepage**: https://timeto.me

### Core Mechanics

**The 24/7 Timer (the killer feature)**
The timer runs ALL THE TIME, 24/7. There is NO STOP BUTTON. To stop tracking
one goal, you MUST start the next one. This forces continuous time awareness —
even sleep, breakfast, and breaks are tracked as goals.

**Timer Types:**
- **Timer** (countdown): Set a duration, counts down
- **Stopwatch**: Counts up from zero
- **Overdue Timer**: Timer that has run past its deadline (turns red)
- **Purple Mode**: A special state (possibly for uncategorized/overflow time)
- **Pomodoro**: Built-in; toggle between work/break within timer

**Goals with Checklists:**
- Each goal can have a checklist
- Checklists are triggered by text features in goal notes
- Useful for morning/evening routines, workouts, work procedures
- Multiple checklists can be attached to a single goal
- Checklist items are tracked per session

**Tasks System:**
- Tasks can have: importance, pause state, timer type, due dates
- Tasks belong to folders (TaskFolderDb)
- Today view filters to today's tasks
- Collapsible task sections on home screen
- Repetitive/recurring tasks supported
- Task folders linked to activities (goals)

**Calendar & Events:**
- Event system with templates
- Calendar views (SummaryCalendarVm)
- Events have time data with event type detection

**Time Analytics:**
- Summary with charts (SummaryChartVm, SummaryVm)
- Pie charts for time distribution (PieChart.kt)
- History view of past intervals
- Color-coded time blocks (ColorEnum for different states)

**Additional Features:**
- Notes with text features (triggers, shortcuts)
- Shortcuts system
- Privacy controls
- Notifications with alarm support
- Backup/restore
- Donations, What's New, ratings prompts
- Demo data for onboarding

### UI Approach
- Kotlin Multiplatform (shared codebase for iOS + Android)
- SQLDelight for local database (SQLite)
- MVVM architecture (Vm base class with MutableStateFlow)
- Compose-based UI on Android (inferred from KMP structure)
- Reactive flows for real-time timer updates

### Gamification Elements
- **Continuous time tracking as game loop**: The 24/7 timer is gamification
  at its core — every second is "scored" to a goal
- **Red overdue state**: Visual urgency when timer expires
- **Activity naming/coloring**: Goals have names and colors
- **Checklist completion**: Sub-tasks give incremental progress feeling
- **Pie charts**: Visual feedback of time allocation
- **Task importance**: Priority system
- **Daily tasks**: "Today" filter creates daily quest-like structure

### Ideas Borrowable for tomogichi

1. **24/7 Timer = Character "Activity" Tracking**
   Assign each of the 4 characters (Riff/Reef/Pitch/Rain) an active tracking
   state. When practicing one domain, its character is "active." Switching
   domains switches the active character. This creates natural time-awareness.

2. **No Stop = Commitment Pressure**
   You can only be doing ONE thing at a time. Forces prioritization. The
   tomogichi characters could only have one "active practice session" at a time.

3. **Checklists = Practice Routines**
   Each character could have a checklist for their practice routine (warmup,
   drills, review, etc.). Completing all items earns bonus coins/XP.

4. **Timer States = Character Moods**
   Red (overdue) = character is stressed/sad. Green (pomodoro break) = relaxing.
   White (active) = focused. Purple = idle/neglected. This ties timer state
   directly to character emotional state.

5. **Pie Charts = Domain Balance**
   Show time distribution across the 4 characters. Encourage balanced growth.

6. **Calendar Integration**
   Daily todos mapped to calendar view. Events as "quests" with time windows.

7. **Repetitive Tasks = Daily Habits**
   Tasks that repeat daily/weekly map directly to habit tracking.

---

## 2. IdleRPG / Idle Fantasy

> NOTE: The user asked about "idlerpg" as an Android app on F-Droid. No app
> literally named "idlerpg" exists on F-Droid. The closest F-Droid + GitHub
> Android idle RPG is **Idle Fantasy** (tristinbaker/IdleFantasy). The dominant
> "IdleRPG" project (Gelbpunkt/IdleRPG, 201★) is a Discord bot, not Android.
> Both are analyzed below.

### 2a. Idle Fantasy (Android App)

#### Overview
- **GitHub**: tristinbaker/IdleFantasy (unknown stars, GPL-3.0)
- **F-Droid**: com.tristinbaker.idlefantasy (via custom F-Droid repo)
- **Platform**: Android only
- **Language**: Kotlin
- **Tagline**: "Set your hero to work. Close the app. Come back to loot."

#### Core Mechanics

**Offline Idle Progression:**
- Pick a skill or dungeon, start a session, close the app
- Hero keeps training for up to 1 hour while app is closed
- Come back to collect XP and loot
- No stamina bar, no energy system, no pressure to stay in app

**20 Skills (3 categories):**
- **Gathering**: Mining, Fishing, Woodcutting, Farming, Agility
- **Crafting**: Smithing, Cooking, Fletching, Jewellery, Prayer (burn logs)
- **Combat**: Melee, Ranged, Magic — each with own skill tree

**12 Dungeons:**
- Progressive difficulty: Farm → Fortress Ruins → endgame
- Each has enemy roster, difficulty rating, potential drops
- Pre-dungeon gear check tells you how you'll fare

**100+ Quests:**
- Daily quests reset each morning (quick goals)
- Long-term Combat and Gathering quests track cumulative progress
- XP rewards on completion

**Pets & Inn:**
- Rare enemies drop collectible pets
- Pets grant permanent passive XP bonuses
- Inn: hire workers to auto-queue sessions
- Buy food to survive longer in tough dungeons

**Gear & Crafting:**
- Craft gear or buy from Shop
- Better gear = faster gathering, tougher dungeon survival

#### UI Approach
- Jetpack Compose + Material 3
- Room (SQLite) for persistence
- WorkManager for background sessions
- MVVM + Repository pattern
- Hilt for dependency injection
- No Google Play Services; F-Droid compatible
- NotificationCompat for session completion notifications

#### Gamification Elements
- **Idle/offline progression**: Progress happens without active play — the
  ultimate low-friction gamification
- **Skill trees**: Visible progression paths with levels
- **Gear progression**: Equipment as power scaling + visible achievement
- **Collectible pets**: Rare drops create "gotta catch 'em all" motivation
- **Daily quests**: Fresh goals every day to maintain engagement
- **Cumulative quests**: Long-term tracking creates streak-like motivation
- **Worker automation**: Unlockable helpers — progression metagame
- **Dungeon difficulty tiers**: Clear progression gates

#### Ideas Borrowable for tomogichi

1. **Offline Practice Sessions**
   Start a "practice session" for a character, close the CLI, come back later.
   The system tracks elapsed time and awards XP/coins accordingly.

2. **Skill Trees Per Character**
   Each of the 4 characters has their own skill tree:
   - Riff → Music/creativity skill tree
   - Reef → Physical/health skill tree
   - Pitch → Academic/learning skill tree
   - Rain → Social/emotional skill tree
   Sub-skills with levels that unlock new abilities or bonuses.

3. **Daily + Cumulative Quests**
   Daily: "Practice 30 min today" → coins
   Cumulative: "Practice 10 hours total this month" → rare item

4. **Pets as Achievement Unlocks**
   Completing milestones (e.g., 100-day streak) unlocks a companion pet
   that provides passive bonuses (e.g., 5% more coins from practice).

5. **Gear as Unlockable Bonuses**
   Spend coins to buy "gear" (e.g., "Metronome" = +10% music XP, "Running
   Shoes" = +10% fitness XP). Crafting = combining coins + achievement tokens.

6. **Worker/Assistant = Automation**
   After hitting certain levels, unlock "assistants" that auto-log certain
   activities or provide reminders.

7. **Background Timer (CLI equivalent)**
   Since tomogichi is CLI-based, a daemon mode where timers continue running
   in background, and checking in shows accumulated time/rewards.

---

### 2b. IdleRPG Discord Bot (Supplementary)

#### Overview
- **GitHub**: Gelbpunkt/IdleRPG (201★, 72 forks, AGPL-3.0, ARCHIVED)
- **Platform**: Discord bot (Python)
- **Homepage**: https://idlerpg.xyz
- **Status**: Archived (no longer maintained)

#### Core Mechanics

**Character System:**
- Create character with name (3-20 chars)
- XP-based level system (levels 1-30, lookup table)
- Money (gold) economy

**7 Character Classes:**
- **Warrior**: Bonus defense
- **Thief**: Access to steal command
- **Mage**: Bonus damage
- **Ranger**: Pet that hunts for gear
- **Raider**: Bonus raidstats for raid battles
- **Ritualist**: Bonus favor from sacrifices, 2x loot chance
- **Paragon**: Bonus damage AND defense (donator-only)

**Class Evolution:**
- Second class unlocks at level 12
- First class selection free; changing costs $5,000
- 24-hour cooldown on class changes

**30 Adventures (Maze/Dungeon System):**
- Named adventures from "Spider Cave" to "Divine Intervention"
- Active maze exploration (N/E/S/W movement, attack, defend, recover)
- Loot drops with rarity

**Guild System:**
- Guild creation, membership, officers, leaders
- Guild cooldowns for actions

**Gods & Favor:**
- Follow a god, sacrifice loot for favor points
- Ritualist class gets bonus favor

**Trading & Economy:**
- Player-to-player trading
- Store system
- Gambling mechanics

**Social/Competitive:**
- Tournaments, raids, raid battles, alliances
- Trivia, chess, snowball fights, werewolf
- Marriage system
- Global events, seasonal events (halloween, valentine, easter, wintersday)

#### Gamification Elements
- **Class system with tradeoffs**: Meaningful choices with different playstyles
- **Evolution/progression gates**: Level 12 unlocks second class
- **Rarity/loot tables**: Variable rewards from adventures
- **Social structures**: Guilds, alliances, marriages — community stickiness
- **Gods/favor**: Sacrifice economy, long-term investment
- **Cooldowns**: Daily/weekly limits create routine check-in patterns
- **Seasonal events**: Time-limited content drives re-engagement
- **Badge system**: Achievement badges

#### Ideas Borrowable for tomogichi

1. **Character Class Evolution**
   At certain XP thresholds, each character "evolves" and gains new
   abilities/visuals. E.g., Riff could evolve from "Apprentice" → "Maestro."

2. **Class-Specific Abilities**
   - Riff: "Compose" — bonus coins for creative sessions
   - Reef: "Endurance" — longer practice sessions earn bonus XP
   - Pitch: "Study Focus" — bonus XP on consecutive-day streaks
   - Rain: "Empathy" — bonus for social/reflection activities

3. **Sacrifice/Favor System**
   Spend coins to "sacrifice" to a character for permanent stat boosts
   or special abilities. Creates a resource sink.

4. **Guild/Team Mechanic**
   The 4 characters are a TEAM. Their combined levels unlock team bonuses.
   Balance across all 4 is rewarded more than maxing one.

5. **Seasonal Challenges**
   Monthly themed challenges with special rewards. Time-limited to drive
   consistent engagement.

6. **Item Drop System**
   Random loot drops from practice sessions (based on session quality/
   duration). Creates surprise-and-delight moments.

7. **Adventure = Challenge Mode**
   Optional "adventure" challenge: commit to a difficult streak (e.g.,
   7 days straight) with bonus rewards but penalty for failure.

---

## 3. Cross-Cutting Patterns & Design Insights

### Timer as Core Loop
Both timeto.me and Idle Fantasy use time as the fundamental game mechanic:
- timeto.me: 24/7 continuous time attribution to goals
- Idle Fantasy: Background sessions that progress offline

For tomogichi: A daemon-based background timer that tracks which character
domain you're currently "practicing" and awards time-based XP/coins.

### Low-Friction Engagement
- timeto.me: No stop button removes decision paralysis
- Idle Fantasy: Set-and-forget with notification when done
- IdleRPG: Cooldowns create natural return rhythm

For tomogichi: Minimize barriers. `tomogichi start riff` starts tracking.
`tomogichi stop` or switching character auto-completes the session.

### Visible Progression Systems
- Idle Fantasy: Skill trees, gear, pets, quest log
- IdleRPG: Levels, class evolutions, badges
- timeto.me: Pie charts, timer colors, checklist completion

For tomogichi: ASCII art character portraits that evolve with levels,
skill bars, coin counter, quest log visible on status screen.

### Reward Variety
- Multiple reward types keep engagement fresh:
  - XP (levels)
  - Coins (economy)
  - Loot/gear (equippable bonuses)
  - Pets (collectible passives)
  - Badges (achievements)
  - Favor (sacrifice economy)

For tomogichi: At minimum, XP + Coins + Achievements. Add gear/pets
as stretch goals.

### Daily Reset Rhythm
All three apps use daily resets to create routine check-in behavior:
- timeto.me: Today's tasks
- Idle Fantasy: Daily quests reset each morning
- IdleRPG: Daily cooldowns on commands

For tomogichi: Daily todos reset, daily quest ("Practice 30 min today"),
streak tracking, calendar integration.
