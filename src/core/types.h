#ifndef TOMOGICHI_TYPES_H
#define TOMOGICHI_TYPES_H

#include <string>
#include <vector>
#include <ctime>

struct Skill {
    std::string name;
    int xp;
    int level;
    time_t last_practice;
    bool is_main;
    float bonus;
    bool archived;          /* hidden from active tracking */
};

struct Person {
    std::string id;
    std::string name;
    std::string role;
    std::vector<Skill> skills;
    std::vector<Skill> archived_skills;  /* retired but XP preserved */
    int level;
    std::string title;
};

struct CrossBonus {
    std::string from_person;
    std::string from_skill;
    std::string to_person;
    std::string to_skill;
    int trigger_level;
    float bonus_pct;
};

enum Effect { EFFECT_ENERGIZED, EFFECT_NEUTRAL, EFFECT_TIRED, EFFECT_DRAINED, EFFECT_NONE };

struct PracticeEntry {
    std::string person_id;
    std::string skill_name;
    int minutes;
    time_t timestamp;
    Effect effect;          /* how user felt after this practice */
    std::string notes;      /* optional free-text note */
};

struct Task {
    int id;
    std::string text;
    bool done;
    time_t created;
};

struct Birthday {
    std::string name;
    std::string date;
};

struct DailyTodo {
    std::string text;
    bool done;
};

struct CalendarSlot {
    int day_of_week;
    int hour;
    int minute;
    std::string person_id;
    std::string skill_name;
    std::string label;
};

struct Challenge {
    std::string id;
    std::string description;
    std::string person_id;
    std::string skill_name;
    int target_count;
    int target_days;
    int coin_reward;
    int current_count;
    time_t started;
    bool completed;
};

struct Achievement {
    std::string id;
    std::string name;
    std::string description;
    std::string icon;
    bool unlocked;
    time_t unlocked_at;
};

struct SkillTodo {
    std::string person_id;
    std::string skill_name;
    std::string text;
    bool done;
};

struct MoodEntry {
    std::string word;       /* e.g. "focused", "tired", "creative" */
    time_t timestamp;
};

struct DiaryEntry {
    std::string text;
    time_t timestamp;
};

struct MasterState {
    std::vector<Task> tasks;
    std::vector<Birthday> birthdays;
    std::vector<DailyTodo> daily_todos;
    std::vector<CalendarSlot> calendar;
    std::vector<Challenge> challenges;
    std::vector<Achievement> achievements;
    std::vector<SkillTodo> skill_todos;
    std::vector<MoodEntry> mood_log;
    std::vector<DiaryEntry> diary_log;
    int next_task_id;
    int coins;
    time_t daily_reset;
    int entropy;             /* 0-100: accumulated neglect, 100 = max penalty */
    int team_sync;           /* 0-100: all-4-practice bonus */
    time_t entropy_updated;  /* last entropy recalculation */
    bool xp_boost;           /* shop item: +50% XP on next practice */
    bool coins_double;       /* shop item: double coins from next challenge */
    bool onboarding_seen;    /* don't show welcome screen again */
};

#endif /* TOMOGICHI_TYPES_H */
