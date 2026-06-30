#include "engine.h"
#include <cmath>
#include <algorithm>
#include <sstream>
#include <ctime>
#include <map>

/* --- XP and Level Calculations --- */

int minutes_to_xp(int minutes) {
    return (minutes * 100) / 60;
}

int xp_to_level(int xp) {
    if (xp < 0) return 0;
    return static_cast<int>(std::sqrt(static_cast<double>(xp) / 100.0));
}

int xp_for_level(int level) {
    return level * level * 100;
}

int xp_progress_pct(int xp) {
    int lvl = xp_to_level(xp);
    int current_lvl_xp = xp_for_level(lvl);
    int next_lvl_xp = xp_for_level(lvl + 1);
    if (next_lvl_xp == current_lvl_xp) return 100;
    int progress = ((xp - current_lvl_xp) * 100) / (next_lvl_xp - current_lvl_xp);
    return progress;
}

/* --- Person Level (weighted) --- */

int calc_person_level(const std::vector<Skill>& skills) {
    if (skills.empty()) return 0;

    /* Weighted average: main skill 50%, each secondary 25% */
    float weighted = 0.0f;
    int secondary_count = 0;

    for (const auto& s : skills) {
        if (s.is_main) {
            weighted += s.level * 0.50f;
        } else {
            weighted += s.level * 0.25f;
            secondary_count++;
        }
    }

    /* Normalise: if we don't have exactly 2 secondary skills,
       scale so main+secondaries = 100% */
    float total_weight = 0.50f + secondary_count * 0.25f;
    if (total_weight > 0.0f) {
        weighted = weighted / total_weight;
    }

    return static_cast<int>(std::floor(weighted));
}

/* --- Titles --- */

std::string title_for_level(int level) {
    if (level >= 25) return "Grandmaster";
    if (level >= 20) return "Maestro";
    if (level >= 15) return "Expert";
    if (level >= 10) return "Artisan";
    if (level >= 5)  return "Apprentice";
    return "Novice";
}

/* --- Cross-Bonuses --- */

std::vector<CrossBonus> cross_bonus_rules() {
    return {
        /* From person/skill  ->  To person/skill, trigger level, bonus % */
        {"riff", "drawing",   "reef", "3d-model",   10, 0.05f},
        {"reef", "3d-model",  "riff", "drawing",    10, 0.05f},
        {"pitch","dance",     "rain", "meditation", 10, 0.05f},
        {"rain", "meditation","riff", "lefthand",   10, 0.05f},
        {"rain", "massage",   "reef", "anatomy",    10, 0.05f},
        {"reef", "anatomy",   "rain", "massage",    10, 0.05f},
        {"reef", "linux",     "pitch","driving",    10, 0.05f},
        {"pitch","kendama",   "riff", "sewing",     10, 0.05f},
    };
}

std::vector<CrossBonus> check_cross_bonuses(const std::string& person_id,
                                              const std::string& skill_name,
                                              int old_level, int new_level) {
    std::vector<CrossBonus> activated;
    for (const auto& rule : cross_bonus_rules()) {
        if (rule.from_person == person_id && rule.from_skill == skill_name) {
            /* Check if we crossed the trigger level */
            if (old_level < rule.trigger_level && new_level >= rule.trigger_level) {
                activated.push_back(rule);
            }
        }
    }
    return activated;
}

void apply_cross_bonus(std::vector<Person>& persons, const CrossBonus& cb) {
    Person* p = find_person(persons, cb.to_person);
    if (!p) return;
    Skill* s = find_skill(*p, cb.to_skill);
    if (!s) return;
    s->bonus += cb.bonus_pct;
}

std::string cross_bonus_to_string(const CrossBonus& cb) {
    std::ostringstream oss;
    oss << cb.from_person << "/" << cb.from_skill
        << "->" << cb.to_person << "/" << cb.to_skill;
    return oss.str();
}

CrossBonus cross_bonus_from_string(const std::string& s) {
    CrossBonus cb{};
    /* Format: "riff/drawing->reef/3d-model" */
    auto arrow = s.find("->");
    if (arrow == std::string::npos) return cb;

    std::string from = s.substr(0, arrow);
    std::string to = s.substr(arrow + 2);

    auto from_slash = from.find('/');
    auto to_slash = to.find('/');

    if (from_slash != std::string::npos) {
        cb.from_person = from.substr(0, from_slash);
        cb.from_skill = from.substr(from_slash + 1);
    }
    if (to_slash != std::string::npos) {
        cb.to_person = to.substr(0, to_slash);
        cb.to_skill = to.substr(to_slash + 1);
    }
    cb.trigger_level = 10;
    cb.bonus_pct = 0.05f;
    return cb;
}

/* --- Person Definitions --- */

Person make_riff() {
    Person p;
    p.id = "riff";
    p.name = "Riff";
    p.role = "Designer";
    p.skills = {
        {"drawing",  0, 0, 0, true,  1.0f, false},
        {"sewing",   0, 0, 0, false, 1.0f, false},
        {"lefthand", 0, 0, 0, false, 1.0f, false},
    };
    p.level = calc_person_level(p.skills);
    p.title = title_for_level(p.level);
    return p;
}

Person make_reef() {
    Person p;
    p.id = "reef";
    p.name = "Reef";
    p.role = "Architect";
    p.skills = {
        {"3d-model", 0, 0, 0, true,  1.0f, false},
        {"linux",    0, 0, 0, false, 1.0f, false},
        {"anatomy",  0, 0, 0, false, 1.0f, false},
    };
    p.level = calc_person_level(p.skills);
    p.title = title_for_level(p.level);
    return p;
}

Person make_pitch() {
    Person p;
    p.id = "pitch";
    p.name = "Pitch";
    p.role = "Athlete";
    p.skills = {
        {"driving", 0, 0, 0, false, 1.0f, false},
        {"dance",   0, 0, 0, true,  1.0f, false},
        {"kendama", 0, 0, 0, false, 1.0f, false},
    };
    p.level = calc_person_level(p.skills);
    p.title = title_for_level(p.level);
    return p;
}

Person make_rain() {
    Person p;
    p.id = "rain";
    p.name = "Rain";
    p.role = "Regulator";
    p.skills = {
        {"ferment",   0, 0, 0, false, 1.0f, false},
        {"massage",   0, 0, 0, true,  1.0f, false},
        {"meditation",0, 0, 0, false, 1.0f, false},
    };
    p.level = calc_person_level(p.skills);
    p.title = title_for_level(p.level);
    return p;
}

/* --- Lookup helpers --- */

Person* find_person(std::vector<Person>& persons, const std::string& id) {
    for (auto& p : persons) {
        if (p.id == id) return &p;
    }
    return nullptr;
}

const Person* find_person(const std::vector<Person>& persons, const std::string& id) {
    for (const auto& p : persons) {
        if (p.id == id) return &p;
    }
    return nullptr;
}

Skill* find_skill(Person& person, const std::string& name) {
    for (auto& s : person.skills) {
        if (s.name == name) return &s;
    }
    return nullptr;
}

const Skill* find_skill(const Person& person, const std::string& name) {
    for (const auto& s : person.skills) {
        if (s.name == name) return &s;
    }
    return nullptr;
}

std::string default_skill(const std::string& person_id) {
    if (person_id == "riff")  return "drawing";
    if (person_id == "reef")  return "3d-model";
    if (person_id == "pitch") return "dance";
    if (person_id == "rain")  return "massage";
    return "";
}

/* --- Warmth --- */

int days_since(time_t t) {
    if (t == 0) return -1;  /* never practiced */
    time_t now = time(nullptr);
    double diff = difftime(now, t);
    return static_cast<int>(diff / 86400.0);
}

Warmth skill_warmth(time_t last_practice) {
    int d = days_since(last_practice);
    if (d <= 2) return WARM;
    if (d <= 5) return COOL;
    return COLD;
}

/* --- Guild Level --- */

int calc_guild_level(const std::vector<Person>& persons) {
    if (persons.empty()) return 0;
    float sum = 0;
    for (const auto& p : persons) {
        sum += p.level;
    }
    return static_cast<int>(std::floor(sum / persons.size()));
}

/* --- Skill Abbreviation Resolution --- */

static std::vector<std::pair<std::string, std::string>> skill_aliases = {
    {"sew", "sewing"},
    {"draw", "drawing"},
    {"left", "lefthand"},
    {"lh", "lefthand"},
    {"3d", "3d-model"},
    {"model", "3d-model"},
    {"lin", "linux"},
    {"anat", "anatomy"},
    {"drive", "driving"},
    {"dan", "dance"},
    {"ken", "kendama"},
    {"ferm", "ferment"},
    {"fer", "ferment"},
    {"mass", "massage"},
    {"mas", "massage"},
    {"med", "meditation"},
    {"medit", "meditation"},
};

std::string resolve_skill(const std::string& person_id, const std::string& input) {
    (void)person_id;  /* reserved for per-person aliases */
    for (const auto& alias : skill_aliases) {
        if (alias.first == input) return alias.second;
    }
    /* Return input as-is (caller will check if it's a valid skill) */
    return input;
}

/* --- Daily Todos --- */

bool reset_daily_todos(MasterState& master) {
    time_t now = time(nullptr);
    struct tm tm_now;
    localtime_r(&now, &tm_now);

    struct tm tm_last;
    localtime_r(&master.daily_reset, &tm_last);

    /* Same day? Don't reset */
    if (tm_now.tm_year == tm_last.tm_year &&
        tm_now.tm_mon  == tm_last.tm_mon &&
        tm_now.tm_mday == tm_last.tm_mday) {
        return false;
    }

    /* New day: reset all todos to undone */
    for (auto& dt : master.daily_todos) {
        dt.done = false;
    }
    master.daily_reset = now;
    return true;
}

/* --- Challenges --- */

int update_challenges(MasterState& master, const std::string& person_id,
                      const std::string& skill_name) {
    int completed = 0;
    time_t now = time(nullptr);

    for (auto& c : master.challenges) {
        if (c.completed) continue;

        /* Check if challenge matches this practice */
        bool person_match = c.person_id.empty() || c.person_id == person_id;
        bool skill_match = c.skill_name.empty() || c.skill_name == skill_name;
        if (!person_match || !skill_match) continue;

        /* Check deadline */
        if (c.target_days > 0 && c.started > 0) {
            double days_elapsed = difftime(now, c.started) / 86400.0;
            if (days_elapsed > c.target_days) continue; /* expired */
        }

        c.current_count++;
        if (c.current_count >= c.target_count) {
            c.completed = true;
            master.coins += c.coin_reward;
            completed++;
        }
    }
    return completed;
}

/* --- Calendar --- */

const CalendarSlot* current_calendar_slot(const MasterState& master) {
    time_t now = time(nullptr);
    struct tm tm_now;
    localtime_r(&now, &tm_now);

    int dow = tm_now.tm_wday;  /* 0=Sun */
    int hour = tm_now.tm_hour;
    int minute = tm_now.tm_min;

    /* Find best match: slot with same day/hour/minute, or closest hour */
    const CalendarSlot* best = nullptr;
    int best_diff = 9999;

    for (const auto& cs : master.calendar) {
        if (cs.day_of_week != dow) continue;

        int diff = abs((cs.hour * 60 + cs.minute) - (hour * 60 + minute));
        /* Within 30 minutes window */
        if (diff <= 30 && diff < best_diff) {
            best = &cs;
            best_diff = diff;
        }
    }
    return best;
}

/* --- Coins --- */

void add_coins(MasterState& master, int amount) {
    master.coins += amount;
}

/* --- Character Mood --- */

CharMood char_mood(time_t last_practice) {
    int d = days_since(last_practice);
    if (d <= 1)  return MOOD_HAPPY;
    if (d <= 3)  return MOOD_NEUTRAL;
    if (d <= 7)  return MOOD_SAD;
    return MOOD_LONELY;
}

const char* mood_emoji(CharMood m) {
    switch (m) {
        case MOOD_HAPPY:  return "😊";
        case MOOD_NEUTRAL:return "😐";
        case MOOD_SAD:    return "😢";
        case MOOD_LONELY: return "💔";
    }
    return "?";
}

const char* mood_text(CharMood m) {
    switch (m) {
        case MOOD_HAPPY:  return "happy";
        case MOOD_NEUTRAL:return "okay";
        case MOOD_SAD:    return "sad";
        case MOOD_LONELY: return "lonely";
    }
    return "?";
}

const Person* most_neglected(const std::vector<Person>& persons) {
    const Person* worst = nullptr;
    int worst_days = -1;

    for (const auto& p : persons) {
        /* Find the most recent practice across all skills */
        time_t latest = 0;
        for (const auto& s : p.skills) {
            if (s.last_practice > latest) latest = s.last_practice;
        }
        int d = days_since(latest);
        if (d > worst_days) {
            worst_days = d;
            worst = &p;
        }
    }
    return worst;
}

/* --- Notifications --- */

bool notify_send(const std::string& title, const std::string& body) {
    std::string cmd = "notify-send";
    /* Escape quotes */
    auto escape = [](const std::string& s) -> std::string {
        std::string out;
        for (char c : s) {
            if (c == '"' || c == '\\') out += '\\';
            out += c;
        }
        return out;
    };
    cmd += " \"" + escape(title) + "\"";
    if (!body.empty()) {
        cmd += " \"" + escape(body) + "\"";
    }
    cmd += " -a tomogichi -i dialog-information 2>/dev/null";
    /* Add timeout to avoid hanging if no notification daemon */
    cmd = "timeout 3 " + cmd;
    return (system(cmd.c_str()) == 0);
}

std::string char_message(const Person& p, CharMood mood) {
    std::string msg;
    switch (mood) {
        case MOOD_HAPPY:
            msg = p.name + " feels great! Keep going!";
            break;
        case MOOD_NEUTRAL:
            msg = p.name + " is doing fine.";
            break;
        case MOOD_SAD:
            msg = p.name + " misses your attention...";
            break;
        case MOOD_LONELY:
            msg = p.name + " feels abandoned. Please come back.";
            break;
    }
    return msg;
}

/* --- Statistics --- */

std::vector<WeeklyStats> compute_weekly_stats(const std::vector<Person>& persons,
                                               const std::vector<PracticeEntry>& log,
                                               int days_back) {
    std::vector<WeeklyStats> result;
    time_t cutoff = time(nullptr) - days_back * 86400;

    for (const auto& p : persons) {
        WeeklyStats ws;
        ws.person_id = p.id;
        ws.person_name = p.name;
        ws.total_minutes = 0;
        ws.days_active = 0;

        /* Collect unique days */
        std::vector<std::string> practice_days;
        for (const auto& pe : log) {
            if (pe.person_id != p.id) continue;
            if (pe.timestamp < cutoff) continue;
            ws.total_minutes += pe.minutes;

            char db[16];
            struct tm tm;
            localtime_r(&pe.timestamp, &tm);
            strftime(db, sizeof(db), "%Y-%m-%d", &tm);
            std::string day(db);

            bool found = false;
            for (const auto& d : practice_days) {
                if (d == day) { found = true; break; }
            }
            if (!found) practice_days.push_back(day);
        }
        ws.days_active = (int)practice_days.size();

        /* Compute streak (consecutive days from today backward) */
        ws.current_streak = 0;
        ws.best_streak = 0;
        if (!practice_days.empty()) {
            /* Sort days */
            std::sort(practice_days.begin(), practice_days.end());
            /* Current streak: count from today backward */
            time_t now = time(nullptr);
            int streak = 0;
            int best = 0;
            int consecutive = 1;

            for (size_t i = 1; i < practice_days.size(); i++) {
                /* Check if consecutive days */
                /* Simple check: parse and compare */
                int y1, m1, d1, y2, m2, d2;
                sscanf(practice_days[i-1].c_str(), "%d-%d-%d", &y1, &m1, &d1);
                sscanf(practice_days[i].c_str(), "%d-%d-%d", &y2, &m2, &d2);
                /* Convert to Julian-ish: compare */
                if (y1 == y2 && m1 == m2 && d2 == d1 + 1) {
                    consecutive++;
                } else if (y1 == y2 && m2 == m1 + 1 && d1 == 31 && d2 == 1) {
                    consecutive++; /* month boundary, approximate */
                } else {
                    if (consecutive > best) best = consecutive;
                    consecutive = 1;
                }
            }
            if (consecutive > best) best = consecutive;

            /* Check if last practice was today or yesterday */
            const std::string& last_day = practice_days.back();
            char today_buf[16];
            struct tm today_tm;
            localtime_r(&now, &today_tm);
            strftime(today_buf, sizeof(today_buf), "%Y-%m-%d", &today_tm);
            std::string today(today_buf);

            if (last_day == today) {
                streak = consecutive;
            } else {
                /* Yesterday? */
                time_t yesterday_t = now - 86400;
                struct tm yes_tm;
                localtime_r(&yesterday_t, &yes_tm);
                char yes_buf[16];
                strftime(yes_buf, sizeof(yes_buf), "%Y-%m-%d", &yes_tm);
                if (last_day == std::string(yes_buf)) {
                    streak = consecutive;
                }
            }
            ws.current_streak = streak;
            ws.best_streak = best;
        }

        result.push_back(ws);
    }
    return result;
}

int total_practice_time(const std::vector<PracticeEntry>& log, int days_back) {
    time_t cutoff = time(nullptr) - days_back * 86400;
    int total = 0;
    for (const auto& pe : log) {
        if (pe.timestamp >= cutoff) total += pe.minutes;
    }
    return total;
}

std::vector<DayBreakdown> daily_breakdown(const std::vector<PracticeEntry>& log,
                                           int days_back) {
    std::vector<DayBreakdown> result;
    time_t now = time(nullptr);

    for (int i = days_back - 1; i >= 0; i--) {
        time_t day_start = now - i * 86400;
        struct tm tm;
        localtime_r(&day_start, &tm);
        tm.tm_hour = 0; tm.tm_min = 0; tm.tm_sec = 0;
        time_t start = mktime(&tm);
        time_t end = start + 86400;

        char db[16];
        strftime(db, sizeof(db), "%Y-%m-%d", &tm);

        int mins = 0;
        for (const auto& pe : log) {
            if (pe.timestamp >= start && pe.timestamp < end) {
                mins += pe.minutes;
            }
        }
        result.push_back({std::string(db), mins});
    }
    return result;
}

std::vector<HeatmapDay> monthly_heatmap(const std::vector<PracticeEntry>& log) {
    std::vector<HeatmapDay> result;
    time_t now = time(nullptr);
    struct tm tm;
    localtime_r(&now, &tm);

    int days_in_month = 31;
    int m = tm.tm_mon;
    if (m == 1) { /* February */
        int y = tm.tm_year + 1900;
        days_in_month = (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0)) ? 29 : 28;
    } else if (m == 3 || m == 5 || m == 8 || m == 10) {
        days_in_month = 30;
    }

    for (int d = 1; d <= days_in_month; d++) {
        tm.tm_mday = d;
        tm.tm_hour = 0; tm.tm_min = 0; tm.tm_sec = 0;
        time_t start = mktime(&tm);
        time_t end = start + 86400;

        int mins = 0;
        for (const auto& pe : log) {
            if (pe.timestamp >= start && pe.timestamp < end) {
                mins += pe.minutes;
            }
        }

        int level = 0;
        if (mins >= 120) level = 3;
        else if (mins >= 60) level = 2;
        else if (mins > 0) level = 1;

        result.push_back({d, level});
    }
    return result;
}

std::vector<PieSlice> pie_data(const std::vector<Person>& persons,
                                const std::vector<PracticeEntry>& log,
                                int days_back) {
    std::vector<PieSlice> result;
    time_t cutoff = time(nullptr) - days_back * 86400;
    int grand_total = 0;

    for (const auto& p : persons) {
        PieSlice ps;
        ps.person_id = p.id;
        ps.person_name = p.name;
        ps.minutes = 0;
        for (const auto& pe : log) {
            if (pe.person_id == p.id && pe.timestamp >= cutoff) {
                ps.minutes += pe.minutes;
            }
        }
        grand_total += ps.minutes;
        result.push_back(ps);
    }

    for (auto& ps : result) {
        if (grand_total > 0) {
            ps.percent = (ps.minutes * 100.0f) / grand_total;
        } else {
            ps.percent = 0.0f;
        }
    }
    return result;
}

/* --- Achievements --- */

std::vector<Achievement> achievement_definitions() {
    return {
        {"first_practice", "First Step", "Log your first practice session", "👣", false, 0},
        {"hour_1", "One Hour Club", "Accumulate 1 hour of practice", "⏱", false, 0},
        {"hour_10", "Dedicated", "Accumulate 10 hours of practice", "🔥", false, 0},
        {"hour_50", "Master in Making", "Accumulate 50 hours of practice", "💎", false, 0},
        {"streak_3", "Momentum", "3-day practice streak", "📿", false, 0},
        {"streak_7", "Weekly Warrior", "7-day practice streak", "⚔", false, 0},
        {"streak_30", "Unstoppable", "30-day practice streak", "👑", false, 0},
        {"all_four", "Full Party", "Practice all 4 characters in one day", "🎭", false, 0},
        {"level_5_any", "Apprentice", "Reach level 5 in any skill", "⭐", false, 0},
        {"level_10_any", "Artisan", "Reach level 10 in any skill", "🌟", false, 0},
        {"level_20_any", "Maestro", "Reach level 20 in any skill", "✨", false, 0},
        {"coins_100", "Collector", "Earn 100 coins", "🪙", false, 0},
        {"coins_1000", "Treasurer", "Earn 1000 coins", "💰", false, 0},
        {"daily_7", "Consistent", "Complete daily todos 7 days in a row", "✅", false, 0},
        {"challenge_5", "Challenger", "Complete 5 challenges", "🏆", false, 0},
        {"challenge_20", "Overachiever", "Complete 20 challenges", "🎪", false, 0},
    };
}

std::vector<Achievement> check_achievements(const std::vector<Person>& persons,
                                             const MasterState& master,
                                             const std::vector<PracticeEntry>& log) {
    std::vector<Achievement> unlocked;

    /* Get existing achievement IDs */
    std::vector<std::string> existing;
    for (const auto& a : master.achievements) {
        if (a.unlocked) existing.push_back(a.id);
    }

    auto try_unlock = [&](const Achievement& def) -> bool {
        for (const auto& id : existing) {
            if (id == def.id) return false; /* already unlocked */
        }
        return true; /* not yet unlocked */
    };

    /* Helper: find definition by id */
    auto find_def = [](const std::string& id) -> Achievement {
        for (const auto& a : achievement_definitions()) {
            if (a.id == id) {
                Achievement copy = a;
                copy.unlocked = true;
                copy.unlocked_at = time(nullptr);
                return copy;
            }
        }
        return Achievement{};
    };

    int total_mins = total_practice_time(log, 99999);
    auto stats = compute_weekly_stats(persons, log, 99999);

    /* first_practice */
    if (!log.empty() && try_unlock(find_def("first_practice"))) {
        unlocked.push_back(find_def("first_practice"));
    }

    /* Hour milestones */
    if (total_mins >= 60 && try_unlock(find_def("hour_1"))) {
        unlocked.push_back(find_def("hour_1"));
    }
    if (total_mins >= 600 && try_unlock(find_def("hour_10"))) {
        unlocked.push_back(find_def("hour_10"));
    }
    if (total_mins >= 3000 && try_unlock(find_def("hour_50"))) {
        unlocked.push_back(find_def("hour_50"));
    }

    /* Streaks */
    int best_streak = 0;
    for (const auto& ws : stats) {
        if (ws.current_streak > best_streak) best_streak = ws.current_streak;
    }
    if (best_streak >= 3 && try_unlock(find_def("streak_3"))) {
        unlocked.push_back(find_def("streak_3"));
    }
    if (best_streak >= 7 && try_unlock(find_def("streak_7"))) {
        unlocked.push_back(find_def("streak_7"));
    }
    if (best_streak >= 30 && try_unlock(find_def("streak_30"))) {
        unlocked.push_back(find_def("streak_30"));
    }

    /* All four in one day */
    if (!log.empty()) {
        time_t today_start = time(nullptr);
        struct tm tm;
        localtime_r(&today_start, &tm);
        tm.tm_hour = 0; tm.tm_min = 0; tm.tm_sec = 0;
        time_t today_t = mktime(&tm);

        bool riff_today = false, reef_today = false, pitch_today = false, rain_today = false;
        for (const auto& pe : log) {
            if (pe.timestamp < today_t) continue;
            if (pe.person_id == "riff") riff_today = true;
            if (pe.person_id == "reef") reef_today = true;
            if (pe.person_id == "pitch") pitch_today = true;
            if (pe.person_id == "rain") rain_today = true;
        }
        if (riff_today && reef_today && pitch_today && rain_today
            && try_unlock(find_def("all_four"))) {
            unlocked.push_back(find_def("all_four"));
        }
    }

    /* Skill levels */
    for (const auto& p : persons) {
        for (const auto& s : p.skills) {
            if (s.archived) continue;
            if (s.level >= 5 && try_unlock(find_def("level_5_any"))) {
                unlocked.push_back(find_def("level_5_any"));
            }
            if (s.level >= 10 && try_unlock(find_def("level_10_any"))) {
                unlocked.push_back(find_def("level_10_any"));
            }
            if (s.level >= 20 && try_unlock(find_def("level_20_any"))) {
                unlocked.push_back(find_def("level_20_any"));
            }
        }
    }

    /* Coins */
    if (master.coins >= 100 && try_unlock(find_def("coins_100"))) {
        unlocked.push_back(find_def("coins_100"));
    }
    if (master.coins >= 1000 && try_unlock(find_def("coins_1000"))) {
        unlocked.push_back(find_def("coins_1000"));
    }

    /* Challenges */
    int chal_done = 0;
    for (const auto& c : master.challenges) {
        if (c.completed) chal_done++;
    }
    if (chal_done >= 5 && try_unlock(find_def("challenge_5"))) {
        unlocked.push_back(find_def("challenge_5"));
    }
    if (chal_done >= 20 && try_unlock(find_def("challenge_20"))) {
        unlocked.push_back(find_def("challenge_20"));
    }

    /* Daily todos streak (approximate: check if 7 days of completed todos exist) */
    /* Simplified: just check if daily_reset shows 7+ days of activity */
    /* We'll skip daily_7 for now — needs more sophisticated tracking */

    return unlocked;
}

/* --- Skill Archive --- */

bool archive_skill(Person& person, const std::string& skill_name) {
    for (auto it = person.skills.begin(); it != person.skills.end(); ++it) {
        if (it->name == skill_name) {
            if (it->is_main) return false; /* cannot archive main skill */
            it->archived = true;
            person.archived_skills.push_back(*it);
            person.skills.erase(it);
            person.level = calc_person_level(person.skills);
            person.title = title_for_level(person.level);
            return true;
        }
    }
    return false;
}

bool reactivate_skill(Person& person, const std::string& skill_name) {
    for (auto it = person.archived_skills.begin(); it != person.archived_skills.end(); ++it) {
        if (it->name == skill_name) {
            it->archived = false;
            person.skills.push_back(*it);
            person.archived_skills.erase(it);
            person.level = calc_person_level(person.skills);
            person.title = title_for_level(person.level);
            return true;
        }
    }
    return false;
}

bool replace_skill(Person& person, const std::string& old_name,
                   const Skill& new_skill) {
    for (auto& s : person.skills) {
        if (s.name == old_name) {
            if (s.is_main) return false;
            s.archived = true;
            person.archived_skills.push_back(s);
            s = new_skill;
            s.archived = false;
            person.level = calc_person_level(person.skills);
            person.title = title_for_level(person.level);
            return true;
        }
    }
    return false;
}

bool delete_skill(Person& person, const std::string& skill_name) {
    /* Check active skills first */
    auto it = std::find_if(person.skills.begin(), person.skills.end(),
        [&](const Skill& s) { return s.name == skill_name; });
    if (it != person.skills.end()) {
        /* Cannot delete the only remaining active skill */
        if (person.skills.size() <= 1) return false;
        bool was_main = it->is_main;
        person.skills.erase(it);
        if (was_main && !person.skills.empty()) {
            person.skills[0].is_main = true;
        }
        person.level = calc_person_level(person.skills);
        person.title = title_for_level(person.level);
        return true;
    }
    /* Check archived skills */
    auto ait = std::find_if(person.archived_skills.begin(), person.archived_skills.end(),
        [&](const Skill& s) { return s.name == skill_name; });
    if (ait != person.archived_skills.end()) {
        person.archived_skills.erase(ait);
        return true;
    }
    return false;
}

bool rename_skill(Person& person, const std::string& old_name, const std::string& new_name) {
    if (old_name == new_name) return false;
    if (new_name.empty()) return false;

    /* Check new name doesn't already exist */
    for (const auto& s : person.skills) {
        if (s.name == new_name) return false;
    }
    for (const auto& s : person.archived_skills) {
        if (s.name == new_name) return false;
    }

    /* Find in active skills */
    for (auto& s : person.skills) {
        if (s.name == old_name) {
            s.name = new_name;
            return true;
        }
    }
    /* Find in archived */
    for (auto& s : person.archived_skills) {
        if (s.name == old_name) {
            s.name = new_name;
            return true;
        }
    }
    return false;
}

/* --- Rename Person --- */

bool rename_person(Person& person, const std::string& new_name) {
    if (new_name.empty()) return false;
    person.name = new_name;
    return true;
}

/* --- Mood & Diary --- */

std::vector<std::string> mood_words() {
    return {
        "happy", "sad", "tired", "energetic", "anxious", "calm",
        "focused", "distracted", "motivated", "lazy", "creative",
        "blocked", "grateful", "frustrated", "peaceful", "restless",
        "excited", "bored", "hopeful", "drained", "curious",
        "overwhelmed", "content", "proud", "lonely"
    };
}

bool is_valid_mood(const std::string& word) {
    for (const auto& w : mood_words()) {
        if (w == word) return true;
    }
    return false;
}

/* --- Effect Analytics --- */

std::vector<SkillEffectStats> skill_effect_stats(const std::vector<PracticeEntry>& log,
                                                  const std::vector<Person>& persons) {
    /* Map: "person_id|skill_name" → counts[4] */
    std::map<std::string, SkillEffectStats> map;
    std::map<std::string, std::string> pid_to_name;
    for (const auto& p : persons) pid_to_name[p.id] = p.name;

    for (const auto& pe : log) {
        std::string key = pe.person_id + "|" + pe.skill_name;
        auto& s = map[key];
        s.person_id = pe.person_id;
        s.person_name = pid_to_name.count(pe.person_id) ? pid_to_name[pe.person_id] : pe.person_id;
        s.skill_name = pe.skill_name;
        int idx = (int)pe.effect;
        if (idx >= 0 && idx < 4) s.counts[idx]++;
    }

    std::vector<SkillEffectStats> result;
    for (auto& kv : map) result.push_back(kv.second);
    return result;
}

std::vector<TimeEffectStats> time_effect_stats(const std::vector<PracticeEntry>& log) {
    TimeEffectStats buckets[4] = {};
    for (int i = 0; i < 4; i++) buckets[i].hour_bucket = i;

    for (const auto& pe : log) {
        struct tm tm;
        time_t t = pe.timestamp;
        localtime_r(&t, &tm);
        int h = tm.tm_hour;
        int bucket;
        if (h >= 6 && h < 12) bucket = 0;      /* morning */
        else if (h >= 12 && h < 18) bucket = 1; /* afternoon */
        else if (h >= 18 && h < 24) bucket = 2; /* evening */
        else bucket = 3;                         /* night */

        int idx = (int)pe.effect;
        if (idx >= 0 && idx < 4) buckets[bucket].counts[idx]++;
        buckets[bucket].total++;
    }

    std::vector<TimeEffectStats> result;
    for (int i = 0; i < 4; i++) result.push_back(buckets[i]);
    return result;
}

std::vector<ActivityChain> activity_chains(const std::vector<PracticeEntry>& log,
                                            const std::vector<Person>& /*persons*/) {
    /* Look at consecutive practices: what was the effect of B after doing A? */
    /* Only pairs within same day (within 6 hours) */
    std::map<std::string, std::pair<int,int>> chain; /* key = "A→B", (energized, total) */

    for (size_t i = 1; i < log.size(); i++) {
        const auto& prev = log[i-1];
        const auto& curr = log[i];
        /* Same day? Within 6 hours? */
        if (curr.timestamp - prev.timestamp > 6 * 3600) continue;
        if (curr.timestamp < prev.timestamp) continue;

        std::string key = prev.person_id + ":" + prev.skill_name + "→"
                        + curr.person_id + ":" + curr.skill_name;
        auto& p = chain[key];
        p.second++;
        if (curr.effect == EFFECT_ENERGIZED) p.first++;
    }

    std::vector<ActivityChain> result;
    for (const auto& kv : chain) {
        if (kv.second.second < 2) continue; /* need at least 2 samples */
        ActivityChain ac;
        size_t arrow = kv.first.find("→");
        ac.person_id = kv.first.substr(0, arrow);
        ac.before_skill = ac.person_id.substr(ac.person_id.find(":") + 1);
        ac.person_id = ac.person_id.substr(0, ac.person_id.find(":"));
        ac.after_skill = kv.first.substr(arrow + 2); /* UTF-8: → is 3 bytes */
        /* Actually → in UTF-8: \xe2\x86\x92 = 3 bytes */
        std::string after = kv.first.substr(arrow + 3); /* skip 3-byte UTF-8 char */
        /* Simpler: just find the pattern */
        ac.after_skill = after.substr(after.find(":") + 1);
        ac.energized_pct = (kv.second.first * 100) / kv.second.second;
        ac.sample_count = kv.second.second;
        result.push_back(ac);
    }

    /* Sort by energized_pct descending */
    std::sort(result.begin(), result.end(),
              [](const ActivityChain& a, const ActivityChain& b) {
                  return a.energized_pct > b.energized_pct;
              });
    return result;
}

/* --- Entropy & Sync --- */

void update_entropy_sync(MasterState &master, const std::vector<PracticeEntry> &log) {
    time_t now = time(nullptr);

    /* Only update once per day */
    if (master.entropy_updated > 0) {
        struct tm last_tm, now_tm;
        localtime_r(&master.entropy_updated, &last_tm);
        localtime_r(&now, &now_tm);
        if (last_tm.tm_year == now_tm.tm_year &&
            last_tm.tm_mon == now_tm.tm_mon &&
            last_tm.tm_yday == now_tm.tm_yday) return; /* already updated today */
    }

    master.entropy_updated = now;

    /* Calculate today's cutoff */
    time_t today_start = now - (now % 86400);

    /* Count who practiced today */
    bool practiced[4] = {false, false, false, false};
    std::string ids[4] = {"riff", "reef", "pitch", "rain"};
    bool rain_practiced = false;

    for (const auto &pe : log) {
        if (pe.timestamp < today_start) continue;
        for (int i = 0; i < 4; i++) {
            if (pe.person_id == ids[i]) practiced[i] = true;
        }
        if (pe.person_id == "rain" &&
            (pe.skill_name == "meditation" || pe.skill_name == "ferment" ||
             pe.skill_name == "massage")) {
            rain_practiced = true;
        }
    }

    /* Sync: % of 4 who practiced */
    int practiced_count = 0;
    for (int i = 0; i < 4; i++) { if (practiced[i]) practiced_count++; }
    master.team_sync = (practiced_count * 100) / 4;

    /* Entropy changes */
    if (practiced_count == 0) {
        master.entropy += 5; /* no practice at all today */
    } else {
        /* Check if Rain hasn't practiced in 3+ days */
        bool rain_recent = false;
        time_t cutoff_3d = now - 3 * 86400;
        for (const auto &pe : log) {
            if (pe.person_id == "rain" && pe.timestamp > cutoff_3d) {
                rain_recent = true; break;
            }
        }
        if (!rain_recent) master.entropy += 10;
    }

    /* Rain's actions reduce entropy */
    if (rain_practiced) {
        master.entropy -= 5;
    }

    /* Clamp */
    if (master.entropy < 0) master.entropy = 0;
    if (master.entropy > 100) master.entropy = 100;
}

float xp_multiplier(const MasterState &master) {
    float penalty = 1.0f - (master.entropy / 200.0f);
    float bonus = 1.0f + (master.team_sync / 200.0f);
    return penalty * bonus;
}
