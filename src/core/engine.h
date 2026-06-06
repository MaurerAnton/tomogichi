#ifndef TOMOGICHI_ENGINE_H
#define TOMOGICHI_ENGINE_H

#include "types.h"
#include <vector>
#include <string>

/* --- XP and Level Calculations --- */

/* Convert minutes of practice to XP. 60 minutes = 100 XP. */
int minutes_to_xp(int minutes);

/* Calculate skill level from XP: floor(sqrt(xp / 100)) */
int xp_to_level(int xp);

/* XP needed to reach a given level (ceiling). */
int xp_for_level(int level);

/* XP progress within current level (0..99) */
int xp_progress_pct(int xp);

/* --- Person Level (weighted) --- */
int calc_person_level(const std::vector<Skill>& skills);

/* --- Titles --- */
std::string title_for_level(int level);

/* --- Cross-Bonuses --- */

/* All defined cross-bonus rules */
std::vector<CrossBonus> cross_bonus_rules();

/* Check newly reached levels and return bonuses to activate */
std::vector<CrossBonus> check_cross_bonuses(const std::string& person_id,
                                              const std::string& skill_name,
                                              int old_level, int new_level);

/* Apply cross-bonus to target skill (adds bonus_pct) */
void apply_cross_bonus(std::vector<Person>& persons, const CrossBonus& cb);

/* Get active cross-bonuses as strings (for display/save) */
std::string cross_bonus_to_string(const CrossBonus& cb);
CrossBonus cross_bonus_from_string(const std::string& s);

/* --- Person Definitions --- */
Person make_riff();
Person make_reef();
Person make_pitch();
Person make_rain();

/* Find person by id */
Person* find_person(std::vector<Person>& persons, const std::string& id);
const Person* find_person(const std::vector<Person>& persons, const std::string& id);

/* Find skill by name */
Skill* find_skill(Person& person, const std::string& name);
const Skill* find_skill(const Person& person, const std::string& name);

/* Get default (main) skill name for a person */
std::string default_skill(const std::string& person_id);

/* Days since last practice */
int days_since(time_t t);

/* --- Warmth status --- */
enum Warmth { WARM, COOL, COLD };
Warmth skill_warmth(time_t last_practice);

/* --- Guild Level --- */
int calc_guild_level(const std::vector<Person>& persons);

/* Resolve skill abbreviation to full name for a given person.
   Returns empty string if not found. */
std::string resolve_skill(const std::string& person_id, const std::string& input);

/* --- Daily Todos --- */

/* Reset daily todos if it's a new day. Returns true if reset happened. */
bool reset_daily_todos(MasterState& master);

/* --- Challenges --- */

/* Update challenge progress after practice. Returns number of newly completed. */
int update_challenges(MasterState& master, const std::string& person_id,
                      const std::string& skill_name);

/* --- Calendar --- */

/* Find the currently active calendar slot. Returns nullptr if none. */
const CalendarSlot* current_calendar_slot(const MasterState& master);

/* --- Coins --- */

/* Add coins and print message */
void add_coins(MasterState& master, int amount);

/* --- Character Mood (for notifications) --- */

enum CharMood { MOOD_HAPPY, MOOD_NEUTRAL, MOOD_SAD, MOOD_LONELY };

/* Get mood based on days since last practice */
CharMood char_mood(time_t last_practice);

/* Mood to emoji+text */
const char* mood_emoji(CharMood m);
const char* mood_text(CharMood m);

/* Check all persons and return the most neglected one */
const Person* most_neglected(const std::vector<Person>& persons);

/* --- Notifications --- */

/* Send desktop notification via notify-send. Returns true on success. */
bool notify_send(const std::string& title, const std::string& body);

/* Generate a character-specific notification message */
std::string char_message(const Person& p, CharMood mood);

/* --- Statistics --- */

struct WeeklyStats {
    std::string person_id;
    std::string person_name;
    int total_minutes;
    int days_active;
    int current_streak;     /* consecutive days with practice */
    int best_streak;
};

/* Compute weekly stats per person from practice log */
std::vector<WeeklyStats> compute_weekly_stats(const std::vector<Person>& persons,
                                               const std::vector<PracticeEntry>& log,
                                               int days_back);

/* Total practice time across all persons for N days */
int total_practice_time(const std::vector<PracticeEntry>& log, int days_back);

/* Daily breakdown: minutes per day for last N days (for heatmap/bar) */
struct DayBreakdown {
    std::string date;       /* "YYYY-MM-DD" */
    int minutes;
};
std::vector<DayBreakdown> daily_breakdown(const std::vector<PracticeEntry>& log,
                                           int days_back);

/* Monthly heatmap: which days had practice (0/1/2/3 intensity levels) */
struct HeatmapDay {
    int day_of_month;       /* 1..31 */
    int level;              /* 0=none, 1=low, 2=medium, 3=high */
};
std::vector<HeatmapDay> monthly_heatmap(const std::vector<PracticeEntry>& log);

/* Pie chart data: percentage per character */
struct PieSlice {
    std::string person_id;
    std::string person_name;
    int minutes;
    float percent;
};
std::vector<PieSlice> pie_data(const std::vector<Person>& persons,
                                const std::vector<PracticeEntry>& log,
                                int days_back);

/* --- Achievements --- */

/* All possible achievement definitions */
std::vector<Achievement> achievement_definitions();

/* Check and unlock any achievements based on current state.
   Returns list of newly unlocked achievements. */
std::vector<Achievement> check_achievements(const std::vector<Person>& persons,
                                             const MasterState& master,
                                             const std::vector<PracticeEntry>& log);

/* --- Skill Archive --- */

/* Move a skill from active to archive. Returns true on success. */
bool archive_skill(Person& person, const std::string& skill_name);

/* Reactivate an archived skill. Returns true on success. */
bool reactivate_skill(Person& person, const std::string& skill_name);

/* Replace an active skill with a new one (old → archive). */
bool replace_skill(Person& person, const std::string& old_name,
                   const Skill& new_skill);

/* Delete a skill permanently (active or archived). Cannot delete the last active skill. */
bool delete_skill(Person& person, const std::string& skill_name);

/* Rename a skill (active or archived). Updates cross-bonus rules that reference it. */
bool rename_skill(Person& person, const std::string& old_name, const std::string& new_name);

/* --- Mood & Diary --- */

/* Valid mood words */
std::vector<std::string> mood_words();

/* Check if word is valid mood */
bool is_valid_mood(const std::string& word);

/* --- Effect Analytics --- */

/* Per-skill effect distribution: how many practices of each skill had each effect.
   Returns: for each person+skill combo, counts of [energized, neutral, tired, drained]. */
struct SkillEffectStats {
    std::string person_id;
    std::string person_name;
    std::string skill_name;
    int counts[4];   /* EFFECT_ENERGIZED, NEUTRAL, TIRED, DRAINED */
};
std::vector<SkillEffectStats> skill_effect_stats(const std::vector<PracticeEntry>& log,
                                                  const std::vector<Person>& persons);

/* Time-of-day effect patterns: morning(6-12), afternoon(12-18), evening(18-24), night(0-6) */
struct TimeEffectStats {
    int hour_bucket;  /* 0=morning, 1=afternoon, 2=evening, 3=night */
    int counts[4];
    int total;
};
std::vector<TimeEffectStats> time_effect_stats(const std::vector<PracticeEntry>& log);

/* What activities tend to precede energized sessions?
   Returns pairs of (skill_before, skill_after, energized_pct). */
struct ActivityChain {
    std::string person_id;
    std::string before_skill;
    std::string after_skill;
    int energized_pct;   /* 0-100 */
    int sample_count;
};
std::vector<ActivityChain> activity_chains(const std::vector<PracticeEntry>& log,
                                            const std::vector<Person>& persons);

/* --- Entropy & Sync --- */

/* Update entropy and team sync based on practice log.
   Call once per day (during daily reset).
   - Entropy: +5 per day without any practice, +10 if Rain hasn't practiced in 3+ days.
     -5 when Rain meditates/ferments/massages in a day. Min 0, max 100.
   - Team sync: % of 4 characters who practiced today * 100 / 4.
     Resets daily. */
void update_entropy_sync(MasterState &master, const std::vector<PracticeEntry> &log);

/* Get XP multiplier from entropy and sync.
   multiplier = (1.0 - entropy / 200.0) * (1.0 + team_sync / 200.0)
   entropy penalty: up to -50% at entropy=100
   sync bonus: up to +50% at sync=100 */
float xp_multiplier(const MasterState &master);

#endif /* TOMOGICHI_ENGINE_H */
