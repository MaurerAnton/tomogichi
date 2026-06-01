#ifndef TOMOGICHI_STORAGE_H
#define TOMOGICHI_STORAGE_H

#include "../core/types.h"
#include <vector>
#include <string>

struct GameState {
    std::vector<Person> persons;
    std::vector<std::string> active_cross_bonuses;  /* serialised CrossBonus */
    MasterState master;
    std::vector<PracticeEntry> practice_log;
};

/* Load state from JSON file. Returns true on success. */
bool load_state(const std::string& path, GameState& state);

/* Save state to JSON file (atomic: write tmp, rename). Returns true on success. */
bool save_state(const std::string& path, const GameState& state);

/* Create default initial state */
GameState default_state();

/* Add a practice entry to log */
void log_practice(GameState& state, const std::string& person_id,
                  const std::string& skill_name, int minutes,
                  Effect effect = EFFECT_NONE,
                  const std::string& notes = "");

#endif /* TOMOGICHI_STORAGE_H */
