#include "core/types.h"
#include "core/engine.h"
#include "data/storage.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <ctime>
#include <cstring>
#include <strings.h>
#include <iomanip>
#include <sys/stat.h>

/* --- Helpers --- */
static std::string repeat(int n, const std::string& s) {
    std::string r;
    r.reserve(n * s.size());
    for (int i = 0; i < n; i++) r += s;
    return r;
}

/* --- Config --- */
static const std::string STATE_PATH = "data/state.json";

/* Timer state (global for access from parse_command) */
static bool timer_running = false;
static std::string timer_person;
static std::string timer_skill;
static time_t timer_start = 0;
static int timer_planned = 0;  /* planned minutes, 0 = no limit */
static bool emergency_triggered = false;  /* skip auto-export after emergency */

/* --- Display Helpers --- */

static const char* WARMTH_SYMBOL(Warmth w) {
    switch (w) {
        case WARM: return "\033[32m●\033[0m";   /* green */
        case COOL: return "\033[33m●\033[0m";   /* yellow */
        case COLD: return "\033[90m●\033[0m";   /* gray */
    }
    return "?";
}

static std::string progress_bar(int pct, int width) {
    std::string bar;
    int filled = (pct * width) / 100;
    for (int i = 0; i < width; i++) {
        if (i < filled) bar += "\033[42m \033[0m";
        else            bar += "\033[100m \033[0m";
    }
    return bar;
}

static std::string pad(const std::string& s, int width) {
    if ((int)s.size() >= width) return s;
    return s + std::string(width - s.size(), ' ');
}

static void print_line(int width = 60) {
    std::cout << std::string(width, '-') << std::endl;
}

static std::string days_ago_str(int days) {
    if (days < 0) return "";
    if (days == 0) return "today";
    if (days == 1) return "1 day ago";
    return std::to_string(days) + " days ago";
}

/* --- Command: status --- */

static void cmd_status(const GameState& state) {
    int guild_lvl = calc_guild_level(state.persons);
    int guild_title_level = guild_lvl;
    std::string guild_title = title_for_level(guild_title_level);

    std::cout << "\n\033[1;36m══════════════ GUILD ══════════════\033[0m\n";
    std::cout << "Level: " << guild_lvl << "  (" << guild_title << ")\n";
    std::cout << "Entropy: " << state.master.entropy << "/100";
    if (state.master.entropy > 50) std::cout << " \033[31m⚠ HIGH\033[0m";
    std::cout << "  |  Sync: " << state.master.team_sync << "%\n";
    std::cout << "Coins: " << state.master.coins << " 💰\n";

    if (state.master.xp_boost) std::cout << "⚡ XP Boost active\n";
    if (state.master.coins_double) std::cout << "🪙 2× Coins active\n";

    if (!state.active_cross_bonuses.empty()) {
        std::cout << "Active cross-bonuses:\n";
        for (const auto& cb_str : state.active_cross_bonuses) {
            std::cout << "  \033[35m" << cb_str << "\033[0m\n";
        }
    }
    print_line();

    for (const auto& p : state.persons) {
        /* Calculate overall mood (most recent skill practice) */
        time_t latest = 0;
        for (const auto& s : p.skills) {
            if (s.last_practice > latest) latest = s.last_practice;
        }
        CharMood mood = char_mood(latest);

        std::cout << "\n\033[1;33m" << mood_emoji(mood) << " " << p.name << "\033[0m"
                  << " (\033[36m" << p.role << "\033[0m)"
                  << "  Level: " << p.level
                  << "  \033[1m" << p.title << "\033[0m\n";

        for (const auto& s : p.skills) {
            Warmth w = skill_warmth(s.last_practice);
            int days = days_since(s.last_practice);
            int pct = xp_progress_pct(s.xp);
            std::string marker = s.is_main ? " \033[1m*\033[0m" : "  ";
            std::string bonus_str = (s.bonus > 1.0f)
                ? " \033[35m+" + std::to_string((int)((s.bonus - 1.0f) * 100)) + "%\033[0m"
                : "";
            std::string warmth_str = WARMTH_SYMBOL(w);

            std::cout << "  " << pad(s.name, 14) << marker
                      << progress_bar(pct, 10) << " "
                      << "lvl " << std::setw(2) << s.level
                      << " (" << std::setw(2) << pct << "%)"
                      << bonus_str
                      << "  " << warmth_str << " " << days_ago_str(days)
                      << "\n";
        }
    }
    print_line();
    std::cout << std::endl;
}

/* --- Command: practice --- */

static void cmd_practice(GameState& state, const std::string& person_id,
                          const std::string& skill_name, int minutes,
                          bool prompt_effect = true) {
    Person* p = find_person(state.persons, person_id);
    if (!p) {
        std::cout << "Unknown person: " << person_id << "\n";
        return;
    }

    Skill* s = find_skill(*p, skill_name);
    if (!s) {
        std::cout << "Unknown skill '" << skill_name << "' for " << p->name << "\n";
        std::cout << "Available: ";
        for (const auto& sk : p->skills) {
            std::cout << sk.name << " ";
        }
        std::cout << "\n";
        return;
    }

    int old_level = s->level;
    int base_xp = minutes_to_xp(minutes);
    int bonus_xp = (int)(base_xp * (s->bonus - 1.0f));
    int total_xp = base_xp + bonus_xp;

    /* Apply entropy/sync multiplier */
    float mult = xp_multiplier(state.master);

    /* Apply XP boost from shop */
    if (state.master.xp_boost) {
        mult *= 1.5f;
        state.master.xp_boost = false;
    }

    total_xp = (int)(total_xp * mult);

    s->xp += total_xp;
    s->level = xp_to_level(s->xp);
    s->last_practice = time(nullptr);

    /* Recalc person level */
    int old_person_level = p->level;
    p->level = calc_person_level(p->skills);
    p->title = title_for_level(p->level);

    /* Log practice */
    log_practice(state, person_id, skill_name, minutes);

    /* Update challenges */
    int chal_completed = update_challenges(state.master, person_id, skill_name);
    for (int i = 0; i < chal_completed; i++) {
        for (const auto& c : state.master.challenges) {
            if (c.completed && c.current_count >= c.target_count
                && (c.person_id.empty() || c.person_id == person_id)
                && (c.skill_name.empty() || c.skill_name == skill_name)) {
                int reward = c.coin_reward;
                if (state.master.coins_double) {
                    reward *= 2;
                    state.master.coins_double = false;
                }
                state.master.coins += reward;
                std::cout << "  \033[1;33m🏆 Challenge complete: " << c.description
                          << " (+" << reward << " coins)\033[0m\n";
                break;
            }
        }
    }

    /* Check achievements after practice */
    auto new_ach = check_achievements(state.persons, state.master, state.practice_log);
    for (auto& a : new_ach) {
        state.master.achievements.push_back(a);
        std::cout << "  \033[1;33m🎉 Achievement: " << a.icon << " "
                  << a.name << " — " << a.description << "\033[0m\n";
    }

    /* Output */
    std::cout << "\n\033[1;33m" << p->name << "\033[0m"
              << ": " << skill_name << " +" << minutes << " min";
    if (bonus_xp > 0) {
        std::cout << " (\033[35m+" << bonus_xp << " XP bonus\033[0m)";
    }
    std::cout << " → +" << total_xp << " XP\n";
    std::cout << "  " << skill_name << ": "
              << s->xp << " XP → lvl " << s->level
              << " (" << xp_progress_pct(s->xp) << "%)";

    if (s->level > old_level) {
        std::cout << " \033[1;32mLEVEL UP!\033[0m";
    }
    std::cout << "\n";

    /* Cross-bonus check */
    auto new_bonuses = check_cross_bonuses(person_id, skill_name, old_level, s->level);
    for (const auto& cb : new_bonuses) {
        /* Check if already active */
        std::string cb_str = cross_bonus_to_string(cb);
        bool already = false;
        for (const auto& a : state.active_cross_bonuses) {
            if (a == cb_str) { already = true; break; }
        }
        if (!already) {
            apply_cross_bonus(state.persons, cb);
            state.active_cross_bonuses.push_back(cb_str);
            std::cout << "  \033[35mCROSS-BONUS UNLOCKED: " << cb_str << " (+5%)\033[0m\n";
        }
    }

    /* Title change */
    if (p->level > old_person_level) {
        std::cout << "  \033[1;33m" << p->name << "\033[0m person level: "
                  << old_person_level << " → " << p->level;
        if (p->title != title_for_level(old_person_level)) {
            std::cout << "  \033[1;32mNEW TITLE: " << p->title << "!\033[0m";
        }
        std::cout << "\n";
    }

    std::cout << std::endl;

    /* Prompt for effect (skip if prompt_effect=false) */
    if (prompt_effect) {
        std::cout << "  How do you feel?  [\033[32me\033[0m]nergized  "
                  << "[\033[33mn\033[0m]eutral  [\033[31mt\033[0m]ired  "
                  << "[\033[90md\033[0m]rained\n  \033[36m> \033[0m";
        std::string ans;
        if (std::getline(std::cin, ans)) {
            Effect eff = EFFECT_NEUTRAL;
            if (!ans.empty()) {
                switch (ans[0]) {
                    case 'e': case 'E': eff = EFFECT_ENERGIZED; break;
                    case 't': case 'T': eff = EFFECT_TIRED; break;
                    case 'd': case 'D': eff = EFFECT_DRAINED; break;
                    default: eff = EFFECT_NEUTRAL; break;
                }
            }
            if (!state.practice_log.empty()) {
                state.practice_log.back().effect = eff;
            }
            const char* eff_str[] = {"energized 🔋", "neutral", "tired 😮‍💨", "drained 🪫"};
            std::cout << "  → " << eff_str[eff] << "\n";

            /* Prompt for optional note */
            std::cout << "  Note? (Enter to skip)\n  \033[36m> \033[0m";
            std::string note;
            if (std::getline(std::cin, note) && !note.empty()) {
                if (!state.practice_log.empty()) {
                    state.practice_log.back().notes = note;
                }
                std::cout << "  → noted\n";
            }
        }
    }
}

/* --- Command: history --- */

static void cmd_history(const GameState& state, int days) {
    time_t cutoff = time(nullptr) - days * 86400;

    std::cout << "\n\033[1;36m══════════ Last " << days << " Days ══════════\033[0m\n";

    /* Group by date */
    struct DayEntry {
        std::string date_str;
        std::vector<std::string> lines;
    };
    std::vector<DayEntry> days_list;
    std::string last_date;

    /* Iterate practice log in reverse (newest first) */
    for (auto it = state.practice_log.rbegin(); it != state.practice_log.rend(); ++it) {
        if (it->timestamp < cutoff) continue;

        char date_buf[16];
        struct tm* tm = localtime(&it->timestamp);
        strftime(date_buf, sizeof(date_buf), "%d %b", tm);
        std::string date_str(date_buf);

        if (date_str != last_date) {
            days_list.push_back({date_str, {}});
            last_date = date_str;
        }

        const Person* p = find_person(state.persons, it->person_id);
        std::string person_name = p ? p->name : it->person_id;
        std::string line = "  " + person_name + " " + it->skill_name + " " +
            std::to_string(it->minutes) + "m";
        if (!it->notes.empty()) {
            line += " — \"" + it->notes + "\"";
        }
        days_list.back().lines.push_back(line);
    }

    if (days_list.empty()) {
        std::cout << "  No practice logged.\n";
    } else {
        for (const auto& de : days_list) {
            std::cout << "\033[1m" << de.date_str << "\033[0m:\n";
            for (const auto& l : de.lines) {
                std::cout << l << "\n";
            }
        }
    }
    print_line();
    std::cout << std::endl;
}

/* --- Command: week --- */

static void cmd_week(const GameState& state) {
    time_t cutoff = time(nullptr) - 7 * 86400;

    /* Count minutes per person this week */
    struct PersonWeek {
        std::string id;
        std::string name;
        int minutes;
        int days_practiced;
        time_t last_practice;
    };

    std::vector<PersonWeek> weeks;
    for (const auto& p : state.persons) {
        PersonWeek pw;
        pw.id = p.id;
        pw.name = p.name;
        pw.minutes = 0;
        pw.days_practiced = 0;
        pw.last_practice = 0;

        std::string last_date;
        for (const auto& pe : state.practice_log) {
            if (pe.person_id != p.id) continue;
            if (pe.timestamp < cutoff) continue;
            pw.minutes += pe.minutes;
            if (pe.timestamp > pw.last_practice) {
                pw.last_practice = pe.timestamp;
            }
            char db[16];
            struct tm* tm = localtime(&pe.timestamp);
            strftime(db, sizeof(db), "%Y-%m-%d", tm);
            std::string date_str(db);
            if (date_str != last_date) {
                pw.days_practiced++;
                last_date = date_str;
            }
        }
        weeks.push_back(pw);
    }

    /* Sort by minutes (least first) */
    std::sort(weeks.begin(), weeks.end(),
              [](const PersonWeek& a, const PersonWeek& b) {
                  return a.minutes < b.minutes;
              });

    std::cout << "\n\033[1;36m══════════ Weekly Summary ══════════\033[0m\n";

    int total_minutes = 0;
    for (const auto& w : weeks) {
        total_minutes += w.minutes;

        /* Status */
        const char* status;
        if (w.minutes == 0)       status = "\033[31mCRITICAL\033[0m";
        else if (w.minutes < 60)  status = "\033[33mLOW\033[0m";
        else if (w.minutes < 180) status = "\033[32mOK\033[0m";
        else                      status = "\033[1;32mGREAT\033[0m";

        double hours = w.minutes / 60.0;
        int days_idle = days_since(w.last_practice);

        std::cout << "  " << pad(w.name, 8)
                  << std::fixed << std::setprecision(1) << std::setw(5) << hours << "h"
                  << "  (" << w.days_practiced << " days)  "
                  << status;
        if (days_idle > 2) {
            std::cout << "  \033[90m" << days_idle << "d idle\033[0m";
        }
        std::cout << "\n";
    }

    std::cout << "  ─────────────────────\n";
    std::cout << "  Total: " << std::fixed << std::setprecision(1)
              << (total_minutes / 60.0) << "h\n";

    /* Recommendation */
    if (!weeks.empty() && weeks[0].minutes < 60) {
        std::cout << "\n  \033[1;33m→ Focus: " << weeks[0].name
                  << " (" << default_skill(weeks[0].id) << ") — needs attention\033[0m\n";
    }

    print_line();
    std::cout << std::endl;
}

/* --- Command: tasks --- */

static void cmd_tasks_show(const GameState& state) {
    std::cout << "\n\033[1;36m══════════ Tasks ══════════\033[0m\n";

    if (state.master.tasks.empty()) {
        std::cout << "  No tasks.\n";
    }

    /* Active tasks first, then done */
    auto sorted = state.master.tasks;
    std::sort(sorted.begin(), sorted.end(),
              [](const Task& a, const Task& b) {
                  if (a.done != b.done) return !a.done; /* active first */
                  return a.id < b.id;
              });

    for (const auto& t : sorted) {
        const char* mark = t.done ? "\033[32m☑\033[0m" : "\033[33m☐\033[0m";
        std::string style = t.done ? "\033[90m" : "";
        std::string end_style = t.done ? "\033[0m" : "";

        std::cout << "  [" << t.id << "] " << mark << " "
                  << style << t.text << end_style << "\n";
    }

    /* Birthdays */
    if (!state.master.birthdays.empty()) {
        std::cout << "\n\033[1;36m────────── Birthdays ──────────\033[0m\n";

        time_t now = time(nullptr);
        struct tm* tm_now = localtime(&now);
        int today_month = tm_now->tm_mon + 1;
        int today_day = tm_now->tm_mday;

        /* Sort by proximity */
        auto bdays = state.master.birthdays;
        std::sort(bdays.begin(), bdays.end(),
                  [today_month, today_day](const Birthday& a, const Birthday& b) {
                      /* Parse MM-DD */
                      int am, ad, bm, bd;
                      sscanf(a.date.c_str(), "%d-%d", &am, &ad);
                      sscanf(b.date.c_str(), "%d-%d", &bm, &bd);

                      /* Days from now */
                      auto days_away = [today_month, today_day](int m, int d) {
                          int da = (m - today_month) * 30 + (d - today_day);
                          if (da < 0) da += 365;
                          return da;
                      };
                      return days_away(am, ad) < days_away(bm, bd);
                  });

        for (const auto& b : bdays) {
            int m, d;
            sscanf(b.date.c_str(), "%d-%d", &m, &d);
            int da = (m - today_month) * 30 + (d - today_day);
            if (da < 0) da += 365;

            std::string proximity;
            if (da == 0) proximity = " \033[1;31mTODAY!\033[0m";
            else if (da <= 3) proximity = " \033[33m(soon: " + std::to_string(da) + "d)\033[0m";
            else if (da <= 14) proximity = " (" + std::to_string(da) + "d)";

            std::cout << "  \033[1;35m🎂\033[0m " << b.name
                      << " — " << b.date << proximity << "\n";
        }
    }

    print_line();
    std::cout << std::endl;
}

static void cmd_task_add(GameState& state, const std::string& text) {
    Task t;
    t.id = state.master.next_task_id++;
    t.text = text;
    t.done = false;
    t.created = time(nullptr);
    state.master.tasks.push_back(t);

    std::cout << "Task added: [" << t.id << "] ☐ " << t.text << "\n\n";
}

static void cmd_task_done(GameState& state, int id, bool done) {
    for (auto& t : state.master.tasks) {
        if (t.id == id) {
            t.done = done;
            const char* status = done ? "\033[32mdone\033[0m" : "\033[33mreopened\033[0m";
            std::cout << "Task [" << id << "] " << status << ": " << t.text << "\n\n";
            return;
        }
    }
    std::cout << "Task [" << id << "] not found.\n\n";
}

static void cmd_task_del(GameState& state, int id) {
    auto it = std::remove_if(state.master.tasks.begin(), state.master.tasks.end(),
                             [id](const Task& t) { return t.id == id; });
    if (it != state.master.tasks.end()) {
        state.master.tasks.erase(it, state.master.tasks.end());
        std::cout << "Task [" << id << "] deleted.\n\n";
    } else {
        std::cout << "Task [" << id << "] not found.\n\n";
    }
}

/* --- Command: birthday --- */

static void cmd_birthday_add(GameState& state, const std::string& name, const std::string& date) {
    Birthday b;
    b.name = name;
    b.date = date;
    state.master.birthdays.push_back(b);
    std::cout << "Birthday added: " << b.name << " — " << b.date << "\n\n";
}

static void cmd_birthday_del(GameState& state, const std::string& name) {
    auto it = std::remove_if(state.master.birthdays.begin(), state.master.birthdays.end(),
                             [&name](const Birthday& b) { return b.name == name; });
    if (it != state.master.birthdays.end()) {
        state.master.birthdays.erase(it, state.master.birthdays.end());
        std::cout << "Birthday '" << name << "' deleted.\n\n";
    } else {
        std::cout << "Birthday '" << name << "' not found.\n\n";
    }
}

/* --- Command: help --- */

static void cmd_help() {
    std::cout << "\033[1;36m══════════ Tomogichi CLI ══════════\033[0m\n\n";
    std::cout << "\033[1mPractice (starts timer):\033[0m\n";
    std::cout << "  riff N          — start timer for Riff drawing, planned N min\n";
    std::cout << "  riff sew N      — start timer for Riff sewing\n";
    std::cout << "  reef N          — start timer for Reef 3d-model\n";
    std::cout << "  reef linux N    — start timer for Reef linux\n";
    std::cout << "  reef anatomy N  — start timer for Reef anatomy\n";
    std::cout << "  pitch N         — start timer for Pitch dance\n";
    std::cout << "  pitch driving N — start timer for Pitch driving\n";
    std::cout << "  pitch ken N     — start timer for Pitch kendama\n";
    std::cout << "  rain N          — start timer for Rain massage\n";
    std::cout << "  rain ferment N  — start timer for Rain fermentation\n";
    std::cout << "  rain med N      — start timer for Rain meditation\n";
    std::cout << "\n\033[1mDisplay:\033[0m\n";
    std::cout << "  status, st      — show all persons, skills, levels, guild\n";
    std::cout << "  history, hist   — practice log (last 7 days)\n";
    std::cout << "  history N       — practice log (last N days)\n";
    std::cout << "  week            — weekly summary with recommendations\n";
    std::cout << "\n\033[1mTasks:\033[0m\n";
    std::cout << "  tasks           — show all tasks and birthdays\n";
    std::cout << "  task add TEXT   — add a task\n";
    std::cout << "  task done N     — mark task #N as done\n";
    std::cout << "  task undo N     — reopen task #N\n";
    std::cout << "  task del N      — delete task #N\n";
    std::cout << "\n\033[1mBirthdays:\033[0m\n";
    std::cout << "  bday add NAME MM-DD  — add birthday\n";
    std::cout << "  bday del NAME        — delete birthday\n";
    std::cout << "\n\033[1mTimer:\033[0m\n";
    std::cout << "  timer start PERSON [skill] — start stopwatch\n";
    std::cout << "  timer, timer status         — show running timer\n";
    std::cout << "  timer stop                  — stop and log minutes\n";
    std::cout << "  timer cancel                — cancel without logging\n";
    std::cout << "\n\033[1mDaily:\033[0m\n";
    std::cout << "  daily                        — show daily checklist\n";
    std::cout << "  daily add TEXT               — add daily item\n";
    std::cout << "  daily done N                 — mark daily #N done\n";
    std::cout << "  daily undo N                 — reopen daily #N\n";
    std::cout << "  daily del N                  — delete daily #N\n";
    std::cout << "\n\033[1mCalendar:\033[0m\n";
    std::cout << "  cal                          — show weekly schedule\n";
    std::cout << "  cal add DAY HH:MM LABEL      — add calendar entry\n";
    std::cout << "  cal del N                    — delete entry #N\n";
    std::cout << "\n\033[1mChallenges & Coins:\033[0m\n";
    std::cout << "  chal                         — show challenges & progress\n";
    std::cout << "  chal add DESC PERS SKILL N D R — add challenge (use - for any)\n";
    std::cout << "  chal del N                   — delete challenge #N\n";
    std::cout << "  coins                        — show coin balance\n";
    std::cout << "  shop                         — browse shop items\n";
    std::cout << "  buy <item> [person]          — purchase shop item\n";
    std::cout << "\n\033[1mStatistics & Achievements:\033[0m\n";
    std::cout << "  stats                        — weekly bar chart + totals\n";
    std::cout << "  stats fx                     — effect analytics (skill/time/chains)\n";
    std::cout << "  ach                          — achievements (badges)\n";
    std::cout << "\n\033[1mSkill Management:\033[0m\n";
    std::cout << "  skill archive PERS SKILL     — archive skill (keep XP)\n";
    std::cout << "  skill reactivate PERS SKILL  — restore from archive\n";
    std::cout << "  skill list                   — show archived skills\n";
    std::cout << "\n\033[1mMood & Diary:\033[0m\n";
    std::cout << "  mood <word>                  — log mood (1 word from list)\n";
    std::cout << "  mood list                    — show valid mood words\n";
    std::cout << "  mood                         — show recent mood log\n";
    std::cout << "  diary <text>                 — add diary entry\n";
    std::cout << "  diary show                   — show recent entries\n";
    std::cout << "\n\033[1mCharacters:\033[0m\n";
    std::cout << "  rename PERSON NEWNAME        — rename a character\n";
    std::cout << "  theme [system|light|dark]    — set app theme (affects GUI)\n";
    std::cout << "  clist                        — show character checklists\n";
    std::cout << "  clist add PERSON TEXT        — add checklist item\n";
    std::cout << "  clist done N                 — mark done\n";
    std::cout << "  clist undo N                 — reopen\n";
    std::cout << "  clist del N                  — delete\n";
    std::cout << "  clist repeat N HOURS         — auto-repeat every N hours\n";
    std::cout << "\n\033[1mTools:\033[0m\n";
    std::cout << "  plan, today                  — today's schedule + tasks + recs\n";
    std::cout << "  stodo                        — skill-specific todos\n";
    std::cout << "  backup                       — backup state.json\n";
    std::cout << "  export csv                   — export practice log to CSV\n";
    std::cout << "  export agora                 — export bridge JSON for Agora\n";
    std::cout << "\n\033[1mNotifications:\033[0m\n";
    std::cout << "  notify TITLE [body]          — send desktop notification\n";
    std::cout << "  (auto-notify on startup if character is sad/lonely)\n";
    std::cout << "\n\033[1mOther:\033[0m\n";
    std::cout << "  help, h         — this help\n";
    std::cout << "  quit, q, exit   — save and exit\n";
    std::cout << std::endl;
}

/* --- Command: timer start (called from parse_command) --- */

static void timer_start_cmd(GameState& state, const std::string& pid,
                              const std::string& skname, int planned_min) {
    Person* tp = find_person(state.persons, pid);
    if (!tp) {
        std::cout << "Unknown person: " << pid << "\n";
        return;
    }
    if (!find_skill(*tp, skname)) {
        std::cout << "Unknown skill '" << skname << "' for " << tp->name << "\n";
        return;
    }
    /* If timer already running, auto-stop it */
    if (timer_running) {
        time_t elapsed_t = time(nullptr) - timer_start;
        int elapsed_min = (int)(elapsed_t / 60);
        if (elapsed_min > 0) {
            std::cout << "Auto-stopping previous timer (" << elapsed_min << " min)...\n";
            cmd_practice(state, timer_person, timer_skill, elapsed_min, false);
        }
    }
    /* Start new timer */
    timer_running = true;
    timer_person = pid;
    timer_skill = skname;
    timer_start = time(nullptr);
    timer_planned = planned_min;
    std::cout << "\n\033[1;35m⏱ Timer: " << timer_person << " " << timer_skill
              << " (planned " << planned_min << " min)\033[0m\n";
    std::cout << "  Type \033[1mtimer stop\033[0m when done, \033[1mtimer cancel\033[0m to discard.\n\n";
}

/* --- Command: daily --- */

static void cmd_daily_show(MasterState& master) {
    reset_daily_todos(master); /* auto-reset if new day */

    std::cout << "\n\033[1;36m══════════ Daily ══════════\033[0m\n";
    if (master.daily_todos.empty()) {
        std::cout << "  No daily tasks.\n";
        std::cout << "  Use: daily add <text>\n";
    }
    int i = 1;
    for (auto& dt : master.daily_todos) {
        const char* mark = dt.done ? "\033[32m☑\033[0m" : "\033[33m☐\033[0m";
        std::string style = dt.done ? "\033[90m" : "";
        std::string end_style = dt.done ? "\033[0m" : "";
        std::cout << "  " << i << ". " << mark << " "
                  << style << dt.text << end_style << "\n";
        i++;
    }
    std::cout << "\n  \033[90mdaily done N | daily undo N | daily add TEXT | daily del N\033[0m\n";
    print_line();
    std::cout << std::endl;
}

static void cmd_daily_action(MasterState& master, const std::string& action,
                               const std::string& arg1, const std::string& arg2) {
    reset_daily_todos(master);

    if (action == "add") {
        DailyTodo dt;
        dt.text = arg1 + (arg2.empty() ? "" : " " + arg2);
        dt.done = false;
        master.daily_todos.push_back(dt);
        std::cout << "Daily added: " << dt.text << "\n\n";
    }
    else if (action == "done") {
        int idx = atoi(arg1.c_str()) - 1;
        if (idx >= 0 && idx < (int)master.daily_todos.size()) {
            master.daily_todos[idx].done = true;
            std::cout << "Daily [" << (idx + 1) << "] done: "
                      << master.daily_todos[idx].text << "\n\n";
        } else {
            std::cout << "Invalid daily index.\n\n";
        }
    }
    else if (action == "undo") {
        int idx = atoi(arg1.c_str()) - 1;
        if (idx >= 0 && idx < (int)master.daily_todos.size()) {
            master.daily_todos[idx].done = false;
            std::cout << "Daily [" << (idx + 1) << "] reopened.\n\n";
        } else {
            std::cout << "Invalid daily index.\n\n";
        }
    }
    else if (action == "del" || action == "delete") {
        int idx = atoi(arg1.c_str()) - 1;
        if (idx >= 0 && idx < (int)master.daily_todos.size()) {
            std::cout << "Daily deleted: " << master.daily_todos[idx].text << "\n\n";
            master.daily_todos.erase(master.daily_todos.begin() + idx);
        } else {
            std::cout << "Invalid daily index.\n\n";
        }
    }
}

/* --- Command: calendar --- */

static const char* DAY_NAMES[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
static const char* MONTH_NAMES[] = {"January","February","March","April","May","June",
                                     "July","August","September","October","November","December"};

static void cmd_calendar_show(const MasterState& master) {
    time_t now = time(nullptr);
    struct tm tm;
    localtime_r(&now, &tm);
    int today = tm.tm_mday;
    int month = tm.tm_mon;
    int year = tm.tm_year + 1900;
    int dow = tm.tm_wday;

    /* Find first day of month */
    struct tm first_tm = tm;
    first_tm.tm_mday = 1;
    mktime(&first_tm);
    int first_dow = first_tm.tm_wday;

    /* Days in month */
    int dim = 31;
    if (month == 1) {
        dim = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) ? 29 : 28;
    } else if (month == 3 || month == 5 || month == 8 || month == 10) {
        dim = 30;
    }

    std::cout << "\n\033[1;36m═════ " << MONTH_NAMES[month] << " " << year << " ═════\033[0m\n\n";

    /* Header */
    std::cout << "  ";
    for (int d = 0; d < 7; d++) {
        std::cout << DAY_NAMES[d];
        if (d < 6) std::cout << " ";
    }
    std::cout << "\n";

    /* Grid */
    for (int week = 0; week < 6; week++) {
        std::cout << "  ";
        for (int d = 0; d < 7; d++) {
            int day_num = week * 7 + d - first_dow + 1;
            if (day_num < 1 || day_num > dim) {
                std::cout << "   ";
            } else {
                /* Count entries for this specific day */
                int entries_today = 0;
                for (const auto& cs : master.calendar) {
                    if (cs.day_of_week == d) entries_today++;
                }

                if (day_num == today) {
                    std::cout << "\033[7m"; /* inverse */
                }
                if (entries_today > 0 && day_num != today) {
                    std::cout << "\033[36m"; /* cyan for days with plans */
                }

                std::cout << (day_num < 10 ? " " : "") << day_num;

                if (entries_today > 0) {
                    std::cout << "\033[0m\033[36m" << (entries_today > 1 ? "▣" : "▢") << "\033[0m";
                } else {
                    std::cout << "\033[0m ";
                }
            }
        }
        std::cout << "\n";
        /* Stop if we've shown all days */
        if (week * 7 + 7 - first_dow >= dim) break;
    }

    /* Today's schedule */
    std::cout << "\n  \033[1mToday (" << DAY_NAMES[dow] << " " << today << "):\033[0m\n";
    bool has_today = false;
    for (const auto& cs : master.calendar) {
        if (cs.day_of_week == dow) {
            char tb[8];
            snprintf(tb, sizeof(tb), "%02d:%02d", cs.hour, cs.minute);
            std::cout << "    \033[33m" << tb << "\033[0m  " << cs.label;
            if (!cs.person_id.empty() && cs.person_id != "master") {
                std::cout << "  \033[90m[" << cs.person_id;
                if (!cs.skill_name.empty()) std::cout << " " << cs.skill_name;
                std::cout << "]\033[0m";
            }
            std::cout << "\n";
            has_today = true;
        }
    }
    if (!has_today) std::cout << "    Nothing planned.\n";

    /* Quick add hint */
    std::cout << "\n  \033[90mplan add <day> <HH:MM> <label>  — add entry\033[0m";
    std::cout << "\n  \033[90mplan del <N>                     — delete by index\033[0m\n";

    print_line();
    std::cout << std::endl;
}

/* --- Command: challenge --- */

static void cmd_challenge_show(GameState& state) {
    std::cout << "\n\033[1;36m══════════ Challenges ══════════\033[0m\n";
    std::cout << "\033[33m💰 Coins: " << state.master.coins << "\033[0m\n\n";

    if (state.master.challenges.empty()) {
        std::cout << "  No challenges.\n";
        std::cout << "  Use: chal add <desc> <person> <skill> <count> <days> <reward>\n";
        std::cout << "  Example: chal add \"Draw daily\" riff drawing 7 7 50\n";
    }

    int i = 1;
    for (const auto& c : state.master.challenges) {
        const char* status;
        if (c.completed) status = "\033[32mDONE\033[0m";
        else if (c.target_days > 0 && c.started > 0) {
            double elapsed = difftime(time(nullptr), c.started) / 86400.0;
            if (elapsed > c.target_days) status = "\033[31mEXPIRED\033[0m";
            else status = "";
        } else status = "";

        std::string progress = std::to_string(c.current_count) + "/" + std::to_string(c.target_count);

        std::cout << "  " << i << ". " << status << " " << c.description
                  << "  [" << progress << "]";
        if (c.coin_reward > 0) std::cout << "  \033[33m+" << c.coin_reward << "💰\033[0m";
        if (c.target_days > 0) std::cout << "  " << c.target_days << "d";
        std::cout << "\n";
        i++;
    }
    print_line();
    std::cout << "  chal add <desc> <person> <skill> <count> <days> <reward>\n";
    std::cout << "  chal del <N>\n\n";
    std::cout << std::endl;
}

/* --- Command: stats --- */

static void cmd_stats(GameState& state) {
    auto stats_w = compute_weekly_stats(state.persons, state.practice_log, 7);
    int total = total_practice_time(state.practice_log, 7);
    int total_all = total_practice_time(state.practice_log, 99999);
    auto pie = pie_data(state.persons, state.practice_log, 7);
    auto daily = daily_breakdown(state.practice_log, 7);
    auto heatmap = monthly_heatmap(state.practice_log);

    std::cout << "\n\033[1;36m══════════ Statistics ══════════\033[0m\n\n";

    /* 1. Pie chart (ASCII) */
    std::cout << "  \033[1mThis Week — Time Distribution\033[0m\n";
    const char* pie_colors[] = {
        "\033[31m", "\033[34m", "\033[32m", "\033[35m"
    };
    for (size_t i = 0; i < pie.size(); i++) {
        int bar_len = (int)(pie[i].percent / 5.0f);
        if (bar_len < 1 && pie[i].minutes > 0) bar_len = 1;
        std::string bar;
        for (int j = 0; j < bar_len; j++) bar += "\342\226\210";  /* █ */
        std::cout << "  " << pad(pie[i].person_name, 8)
                  << " " << pie_colors[i] << bar << "\033[0m"
                  << std::string(20 - bar_len, ' ')
                  << std::fixed << std::setprecision(1) << std::setw(5) << pie[i].percent << "%"
                  << " (" << std::setw(2) << (pie[i].minutes / 60) << "h"
                  << std::setw(3) << (pie[i].minutes % 60) << "m)\n";
    }

    /* 2. Daily bar chart */
    std::cout << "\n  \033[1mDaily Breakdown\033[0m\n";
    int max_daily = 0;
    for (const auto& d : daily) {
        if (d.minutes > max_daily) max_daily = d.minutes;
    }
    if (max_daily == 0) max_daily = 1;
    for (const auto& d : daily) {
        int bar_len = (d.minutes * 20) / max_daily;
        if (bar_len < 1 && d.minutes > 0) bar_len = 1;
        /* Show day name */
        struct tm tm = {};
        int y, m, day;
        sscanf(d.date.c_str(), "%d-%d-%d", &y, &m, &day);
        tm.tm_year = y - 1900; tm.tm_mon = m - 1; tm.tm_mday = day;
        mktime(&tm);
        const char* dnames[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
        std::cout << "  " << d.date.substr(5) << " " << dnames[tm.tm_wday] << " "
                  << "\033[42m" << std::string(bar_len, ' ') << "\033[0m"
                  << std::string(20 - bar_len, ' ')
                  << std::fixed << std::setprecision(1) << std::setw(5)
                  << (d.minutes / 60.0) << "h\n";
    }

    /* 3. Monthly heatmap */
    std::cout << "\n  \033[1mMonthly Heatmap\033[0m\n  ";
    const char* heat_chars[] = {"·", "░", "▒", "▓", "█"};
    int col = 0;
    for (const auto& hd : heatmap) {
        std::cout << heat_chars[hd.level];
        col++;
        if (col % 10 == 0 && col < (int)heatmap.size()) std::cout << "\n  ";
    }
    std::cout << "\n  " << heat_chars[0] << "=0  " << heat_chars[1] << "=<1h  "
              << heat_chars[2] << "=<2h  " << heat_chars[3] << "=>2h\n";

    /* 4. Totals & Streaks */
    std::cout << "\n  \033[1mStreaks:\033[0m\n";
    for (const auto& ws : stats_w) {
        if (ws.current_streak > 0) {
            std::cout << "  " << pad(ws.person_name, 8) << " 🔥" << ws.current_streak
                      << "d (best: " << ws.best_streak << "d)\n";
        }
    }
    std::cout << "\n  \033[1mTotals:\033[0m\n";
    std::cout << "  This week: " << std::fixed << std::setprecision(1)
              << (total / 60.0) << "h  |  All time: " << (total_all / 60.0)
              << "h (" << state.practice_log.size() << " sessions)\n";

    /* 5. Recent timeline */
    std::cout << "\n  \033[1mRecent Activity\033[0m\n";
    int shown = 0;
    for (auto it = state.practice_log.rbegin();
         it != state.practice_log.rend() && shown < 8; ++it) {
        char tb[16];
        struct tm tm;
        localtime_r(&it->timestamp, &tm);
        strftime(tb, sizeof(tb), "%d %b %H:%M", &tm);
        const Person* pp = find_person(state.persons, it->person_id);
        std::string pname = pp ? pp->name : it->person_id;
        std::cout << "  " << tb << "  " << pad(pname, 8)
                  << " " << it->skill_name << " " << it->minutes << "m\n";
        shown++;
    }

    int guild_lvl = calc_guild_level(state.persons);
    std::cout << "\n  Guild lvl: " << guild_lvl
              << " (" << title_for_level(guild_lvl) << ")\n";
    std::cout << "  Entropy: " << state.master.entropy
              << "/100  Sync: " << state.master.team_sync
              << "%  Coins: " << state.master.coins << "\n";

    print_line();
    std::cout << std::endl;
}

/* --- Command: stats fx (effect analytics) --- */

static void cmd_stats_fx(GameState& state) {
    std::cout << "\n\033[1;36m══════════ Effect Analytics ══════════\033[0m\n\n";

    auto skill_stats = skill_effect_stats(state.practice_log, state.persons);
    auto time_stats = time_effect_stats(state.practice_log);
    auto chains = activity_chains(state.practice_log, state.persons);

    const char* eff_names[] = {"\033[32menergized\033[0m", "neutral",
                                "\033[31mtired\033[0m", "\033[90mdrained\033[0m"};
    /* 1. Per-skill effect bars */
    std::cout << "  \033[1mPer-Skill Effect Distribution\033[0m\n";
    for (const auto& ss : skill_stats) {
        int total = ss.counts[0] + ss.counts[1] + ss.counts[2] + ss.counts[3];
        if (total == 0) continue;
        std::cout << "  " << pad(ss.person_name, 8) << " " << ss.skill_name;
        for (int i = 0; i < 4; i++) {
            if (ss.counts[i] == 0) continue;
            int pct = (ss.counts[i] * 100) / total;
            std::cout << "  " << eff_names[i] << " " << pct << "%";
        }
        std::cout << "\n";
    }

    /* 2. Time-of-day patterns */
    std::cout << "\n  \033[1mTime-of-Day Patterns\033[0m\n";
    const char* bucket_names[] = {"🌅 Morning  ", "☀️ Afternoon",
                                   "🌆 Evening  ", "🌙 Night    "};
    for (const auto& ts : time_stats) {
        if (ts.total == 0) continue;
        std::cout << "  " << bucket_names[ts.hour_bucket] << " (" << ts.total << " sessions)\n";
        for (int i = 0; i < 4; i++) {
            if (ts.counts[i] == 0) continue;
            int pct = (ts.counts[i] * 100) / ts.total;
            int bar = pct / 5;
            std::cout << "    " << eff_names[i] << " "
                      << repeat(bar, "█") << std::string(20 - bar, ' ')
                      << " " << pct << "%\n";
        }
    }

    /* 3. Activity chains: what leads to energized? */
    std::cout << "\n  \033[1mActivity Chains → Energized\033[0m\n";
    if (chains.empty()) {
        std::cout << "  Need more data (at least 2 consecutive sessions).\n";
    } else {
        int shown = 0;
        for (const auto& ac : chains) {
            if (shown >= 8) break;
            const Person* bp = find_person(state.persons, ac.person_id);
            std::string bname = bp ? bp->name : ac.person_id;
            std::cout << "  " << bname << " " << ac.before_skill << " → "
                      << ac.after_skill << ": \033[32m"
                      << ac.energized_pct << "%\033[0m energized ("
                      << ac.sample_count << "×)\n";
            shown++;
        }
    }

    print_line();
    std::cout << std::endl;
}

/* --- Command: achievements --- */

static void cmd_achievements(GameState& state) {
    auto new_ach = check_achievements(state.persons, state.master, state.practice_log);
    for (auto& a : new_ach) {
        state.master.achievements.push_back(a);
        std::cout << "\n\033[1;33m🎉 Achievement unlocked: " << a.icon
                  << " " << a.name << " — " << a.description << "\033[0m\n";
    }

    std::cout << "\n\033[1;36m══════════ Achievements ══════════\033[0m\n\n";

    auto all_defs = achievement_definitions();
    int unlocked_count = 0;

    for (const auto& def : all_defs) {
        bool found = false;
        for (const auto& a : state.master.achievements) {
            if (a.id == def.id && a.unlocked) {
                std::cout << "  \033[32m" << def.icon << " " << def.name
                          << "\033[0m — " << def.description << "\n";
                found = true;
                unlocked_count++;
                break;
            }
        }
        if (!found) {
            std::cout << "  \033[90m🔒 " << def.name
                      << " — " << def.description << "\033[0m\n";
        }
    }

    std::cout << "\n  Progress: " << unlocked_count << "/" << all_defs.size() << "\n";
    print_line();
    std::cout << std::endl;
}

/* --- Command: mood --- */

static void cmd_mood(GameState& state, std::istringstream& iss) {
    std::string word;
    iss >> word;

    if (word == "list" || word == "words") {
        std::cout << "\n\033[1;36mMood words:\033[0m\n  ";
        int col = 0;
        for (const auto& w : mood_words()) {
            std::cout << w;
            col++;
            if (col % 5 == 0) std::cout << "\n  ";
            else std::cout << ", ";
        }
        std::cout << "\n\n";
        return;
    }

    if (word.empty()) {
        /* Show recent moods */
        std::cout << "\n\033[1;36m══════════ Mood Log ══════════\033[0m\n";
        if (state.master.mood_log.empty()) {
            std::cout << "  No mood entries yet.\n  Use: mood <word> (e.g. mood focused)\n";
        }
        int count = 0;
        for (auto it = state.master.mood_log.rbegin();
             it != state.master.mood_log.rend() && count < 14; ++it, ++count) {
            char db[16];
            struct tm tm;
            localtime_r(&it->timestamp, &tm);
            strftime(db, sizeof(db), "%d %b %H:%M", &tm);
            std::cout << "  " << db << "  \033[35m" << it->word << "\033[0m\n";
        }
        print_line();
        std::cout << std::endl;
        return;
    }

    if (!is_valid_mood(word)) {
        std::cout << "Invalid mood. Valid words:\n  ";
        for (const auto& w : mood_words()) {
            std::cout << w << " ";
        }
        std::cout << "\n\n";
        return;
    }

    MoodEntry me;
    me.word = word;
    me.timestamp = time(nullptr);
    state.master.mood_log.push_back(me);
    std::cout << "Mood: \033[35m" << word << "\033[0m logged.\n\n";
}

/* --- Command: diary --- */

static void cmd_diary(GameState& state, std::istringstream& iss) {
    std::string sub;
    iss >> sub;

    if (sub == "show" || sub == "read" || sub.empty()) {
        std::cout << "\n\033[1;36m══════════ Diary ══════════\033[0m\n";
        if (state.master.diary_log.empty()) {
            std::cout << "  No entries yet.\n  Use: diary <text>\n";
        }
        int count = 0;
        for (auto it = state.master.diary_log.rbegin();
             it != state.master.diary_log.rend() && count < 10; ++it, ++count) {
            char db[16];
            struct tm tm;
            localtime_r(&it->timestamp, &tm);
            strftime(db, sizeof(db), "%d %b %H:%M", &tm);
            std::cout << "  \033[90m" << db << "\033[0m\n";
            std::cout << "  " << it->text << "\n\n";
        }
        if ((int)state.master.diary_log.size() > 10) {
            std::cout << "  ... (" << state.master.diary_log.size() << " total)\n";
        }
        print_line();
        std::cout << std::endl;
        return;
    }

    /* Anything else = add entry */
    std::string text = sub;
    std::string rest;
    getline(iss, rest);
    text += rest;

    if (text.empty()) {
        std::cout << "Usage: diary <text> | diary show\n\n";
        return;
    }

    DiaryEntry de;
    de.text = text;
    de.timestamp = time(nullptr);
    state.master.diary_log.push_back(de);
    std::cout << "Diary entry saved.\n\n";
}

/* --- Command: skill todo --- */

static void cmd_skill_todo(GameState& state, std::istringstream& iss) {
    std::string sub, pid, skname, text;
    iss >> sub;

    if (sub == "add") {
        iss >> pid >> skname;
        getline(iss, text);
        size_t s = text.find_first_not_of(" \t");
        if (s != std::string::npos) text = text.substr(s);
        if (pid.empty() || skname.empty() || text.empty()) {
            std::cout << "Usage: stodo add <person> <skill> <text>\n\n";
            return;
        }
        SkillTodo st;
        st.person_id = pid;
        st.skill_name = resolve_skill(pid, skname);
        st.text = text;
        st.done = false;
        state.master.skill_todos.push_back(st);
        std::cout << "Skill todo added: " << pid << " " << st.skill_name
                  << " — " << text << "\n\n";
    }
    else if (sub == "done") {
        int idx;
        iss >> idx;
        if (idx > 0 && idx <= (int)state.master.skill_todos.size()) {
            state.master.skill_todos[idx - 1].done = true;
            std::cout << "Skill todo [" << idx << "] done.\n\n";
        } else {
            std::cout << "Invalid index.\n\n";
        }
    }
    else if (sub == "undo") {
        int idx;
        iss >> idx;
        if (idx > 0 && idx <= (int)state.master.skill_todos.size()) {
            state.master.skill_todos[idx - 1].done = false;
            std::cout << "Skill todo [" << idx << "] reopened.\n\n";
        } else {
            std::cout << "Invalid index.\n\n";
        }
    }
    else if (sub == "del") {
        int idx;
        iss >> idx;
        if (idx > 0 && idx <= (int)state.master.skill_todos.size()) {
            state.master.skill_todos.erase(state.master.skill_todos.begin() + idx - 1);
            std::cout << "Skill todo deleted.\n\n";
        } else {
            std::cout << "Invalid index.\n\n";
        }
    }
    else {
        /* Show all skill todos */
        std::cout << "\n\033[1;36m══════════ Skill Todos ══════════\033[0m\n";
        if (state.master.skill_todos.empty()) {
            std::cout << "  No skill todos.\n";
            std::cout << "  Use: stodo add <person> <skill> <text>\n";
        }
        int i = 1;
        for (const auto& st : state.master.skill_todos) {
            const char* mark = st.done ? "\033[32m☑\033[0m" : "\033[33m☐\033[0m";
            std::cout << "  " << i << ". " << mark << " [" << st.person_id
                      << " " << st.skill_name << "] " << st.text << "\n";
            i++;
        }
        print_line();
        std::cout << "  stodo add PERS SKILL TEXT | stodo done N | stodo del N\n\n";
        std::cout << std::endl;
    }
}

/* --- Command: plan/today --- */

static void cmd_plan_add(GameState& state, std::istringstream& iss) {
    std::string day_str, time_str, label;
    if (iss >> day_str >> time_str) {
        getline(iss, label);
        size_t s = label.find_first_not_of(" \t");
        if (s != std::string::npos) label = label.substr(s);

        int dow = -1;
        for (int i = 0; i < 7; i++) {
            if (strcasecmp(day_str.c_str(), DAY_NAMES[i]) == 0) { dow = i; break; }
        }
        const char* short_names[] = {"sun","mon","tue","wed","thu","fri","sat"};
        if (dow < 0) {
            for (int i = 0; i < 7; i++) {
                if (strcasecmp(day_str.c_str(), short_names[i]) == 0) { dow = i; break; }
            }
        }
        if (dow < 0) {
            std::cout << "Invalid day. Use: mon, tue, wed, thu, fri, sat, sun\n\n";
            return;
        }
        int h = 0, m = 0;
        sscanf(time_str.c_str(), "%d:%d", &h, &m);
        if (label.empty()) label = "Event";
        CalendarSlot cs;
        cs.day_of_week = dow;
        cs.hour = h;
        cs.minute = m;
        cs.person_id = "master";
        cs.skill_name = "";
        cs.label = label;
        state.master.calendar.push_back(cs);
        std::cout << "Plan: " << DAY_NAMES[dow] << " " << time_str
                  << " — " << label << "\n\n";
    } else {
        std::cout << "Usage: plan add <day> <HH:MM> <label>\n";
        std::cout << "Example: plan add mon 09:00 Morning drawing\n\n";
    }
}

static void cmd_plan_show(GameState& state) {
    reset_daily_todos(state.master);
    time_t now = time(nullptr);
    struct tm tm;
    localtime_r(&now, &tm);
    char today_buf[16];
    strftime(today_buf, sizeof(today_buf), "%d %B %Y", &tm);
    int dow = tm.tm_wday;

    std::cout << "\n\033[1;36m══════════ Today: " << today_buf << " ══════════\033[0m\n\n";

    /* Calendar slots for today */
    std::cout << "  \033[1mSchedule:\033[0m\n";
    bool has_cal = false;
    for (const auto& cs : state.master.calendar) {
        if (cs.day_of_week == dow) {
            char tb[8];
            snprintf(tb, sizeof(tb), "%02d:%02d", cs.hour, cs.minute);
            std::cout << "    " << tb << "  " << cs.label;
            if (!cs.person_id.empty() && cs.person_id != "master") {
                std::cout << "  [" << cs.person_id;
                if (!cs.skill_name.empty()) std::cout << " " << cs.skill_name;
                std::cout << "]";
            }
            std::cout << "\n";
            has_cal = true;
        }
    }
    if (!has_cal) std::cout << "    Nothing scheduled.\n";

    /* Daily todos */
    std::cout << "\n  \033[1mDaily Tasks:\033[0m\n";
    if (state.master.daily_todos.empty()) {
        std::cout << "    No daily tasks.\n";
    }
    for (const auto& dt : state.master.daily_todos) {
        const char* mark = dt.done ? "\033[32m☑\033[0m" : "\033[33m☐\033[0m";
        std::cout << "    " << mark << " " << dt.text << "\n";
    }

    /* Tasks */
    std::cout << "\n  \033[1mOpen Tasks:\033[0m\n";
    bool has_tasks = false;
    for (const auto& t : state.master.tasks) {
        if (!t.done) {
            std::cout << "    [" << t.id << "] ☐ " << t.text << "\n";
            has_tasks = true;
        }
    }
    if (!has_tasks) std::cout << "    All clear!\n";

    /* Recommended practice */
    std::cout << "\n  \033[1mRecommended Practice:\033[0m\n";
    const Person* neglected = most_neglected(state.persons);
    if (neglected) {
        time_t latest = 0;
        for (const auto& s : neglected->skills) {
            if (s.last_practice > latest) latest = s.last_practice;
        }
        int d = days_since(latest);
        if (latest > 0 && d > 1) {
            std::cout << "    \033[33m→ " << neglected->name << " " << default_skill(neglected->id)
                      << " (" << d << " days idle)\033[0m\n";
        }
    }
    /* Show all with idle days */
    for (const auto& p : state.persons) {
        time_t latest = 0;
        for (const auto& s : p.skills) {
            if (s.last_practice > latest) latest = s.last_practice;
        }
        int d = days_since(latest);
        if (latest > 0 && d > 0 && &p != neglected) {
            std::cout << "    " << p.name << " " << default_skill(p.id)
                      << " (" << d << "d)\n";
        }
    }

    /* Mood today */
    if (!state.master.mood_log.empty()) {
        const auto& last_mood = state.master.mood_log.back();
        struct tm mtm;
        localtime_r(&last_mood.timestamp, &mtm);
        if (mtm.tm_year == tm.tm_year && mtm.tm_mon == tm.tm_mon
            && mtm.tm_mday == tm.tm_mday) {
            std::cout << "\n  \033[1mToday's Mood:\033[0m \033[35m" << last_mood.word << "\033[0m\n";
        }
    }

    std::cout << "\n  \033[90m💰 Coins: " << state.master.coins
              << "  |  Guild: " << calc_guild_level(state.persons) << "\033[0m\n";

    print_line();
    std::cout << std::endl;
}

/* --- Command: backup --- */

static void cmd_backup() {
    std::string src = "data/state.json";
    time_t now = time(nullptr);
    char ts[32];
    struct tm tm;
    localtime_r(&now, &tm);
    strftime(ts, sizeof(ts), "data/state_%Y%m%d_%H%M%S.json", &tm);
    std::string dst(ts);

    std::ifstream in(src, std::ios::binary);
    if (!in) {
        std::cout << "No state file to backup.\n\n";
        return;
    }
    std::ofstream out(dst, std::ios::binary);
    out << in.rdbuf();
    in.close();
    out.close();
    std::cout << "Backup saved: " << dst << "\n\n";
}

/* --- Command: export agora (bridge to Agora LLM client) --- */

static std::string json_escape(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else out += c;
    }
    return out;
}

static std::string time_to_str(time_t t) {
    if (t == 0) return "";
    char buf[32];
    struct tm tm;
    localtime_r(&t, &tm);
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tm);
    return std::string(buf);
}

static void cmd_export_agora(GameState& state) {
    time_t now = time(nullptr);
    struct tm tm_now;
    localtime_r(&now, &tm_now);
    char date_buf[16], time_buf[8];
    strftime(date_buf, sizeof(date_buf), "%Y-%m-%d", &tm_now);
    strftime(time_buf, sizeof(time_buf), "%H:%M", &tm_now);

    std::ostringstream j;
    j << "{\n";
    j << "  \"app\": \"tomogichi\",\n";
    j << "  \"version\": \"0.5.0\",\n";
    j << "  \"emergency\": " << (state.master.entropy > 90 ? "true" : "false") << ",\n";
    j << "  \"generated_at\": \"" << date_buf << "T" << time_buf << ":00\",\n";
    j << "  \"today\": {\"date\": \"" << date_buf << "\", \"dow\": " << tm_now.tm_wday << "},\n";
    j << "  \"now\": \"" << time_buf << "\",\n";

    /* Guild */
    int gl = calc_guild_level(state.persons);
    j << "  \"guild\": {\"level\": " << gl << ", \"coins\": " << state.master.coins
      << ", \"entropy\": " << state.master.entropy
      << ", \"sync\": " << state.master.team_sync << "},\n";

    /* Persons / Characters */
    j << "  \"persons\": [\n";
    for (size_t pi = 0; pi < state.persons.size(); pi++) {
        const auto& p = state.persons[pi];
        time_t latest = 0;
        for (const auto& s : p.skills) if (s.last_practice > latest) latest = s.last_practice;
        CharMood mood = char_mood(latest);
        j << "    {\"id\": \"" << p.id << "\", \"name\": \"" << json_escape(p.name)
          << "\", \"role\": \"" << p.role << "\", \"level\": " << p.level
          << ", \"title\": \"" << p.title << "\", \"mood\": \"" << mood_text(mood)
          << "\", \"days_idle\": " << days_since(latest) << ", \"skills\": [";
        for (size_t si = 0; si < p.skills.size(); si++) {
            const auto& s = p.skills[si];
            if (si > 0) j << ", ";
            j << "{\"name\": \"" << s.name << "\", \"level\": " << s.level
              << ", \"days_since\": " << days_since(s.last_practice)
              << ", \"is_main\": " << (s.is_main ? "true" : "false") << "}";
        }
        j << "]}";
        if (pi + 1 < state.persons.size()) j << ",";
        j << "\n";
    }
    j << "  ],\n";

    /* Calendar — today + next 3 weeks */
    j << "  \"calendar_today\": [\n";
    {
        bool first = true;
        for (const auto& cs : state.master.calendar) {
            if (cs.day_of_week == tm_now.tm_wday) {
                if (!first) j << ",\n";
                char tb[8]; snprintf(tb, sizeof(tb), "%02d:%02d", cs.hour, cs.minute);
                j << "    {\"time\": \"" << tb << "\", \"label\": \"" << json_escape(cs.label)
                  << "\", \"person\": \"" << cs.person_id << "\"}";
                first = false;
            }
        }
        j << (first ? "" : "\n  ") << "],\n";
    }

    /* Calendar upcoming (next 3 weeks) */
    j << "  \"calendar_upcoming\": [\n";
    {
        bool first = true;
        for (int d = 0; d < 21; d++) {
            time_t day_t = now + d * 86400;
            struct tm dtm;
            localtime_r(&day_t, &dtm);
            int dow = dtm.tm_wday;
            for (const auto& cs : state.master.calendar) {
                if (cs.day_of_week == dow) {
                    if (!first) j << ",\n";
                    char db[16]; strftime(db, sizeof(db), "%Y-%m-%d", &dtm);
                    char tb[8]; snprintf(tb, sizeof(tb), "%02d:%02d", cs.hour, cs.minute);
                    int days_away = d;
                    j << "    {\"date\": \"" << db << "\", \"time\": \"" << tb
                      << "\", \"days_away\": " << days_away
                      << ", \"label\": \"" << json_escape(cs.label)
                      << "\", \"person\": \"" << cs.person_id << "\"}";
                    first = false;
                }
            }
        }
        j << (first ? "" : "\n  ") << "],\n";
    }

    /* Reminders now — events within next 30 min */
    j << "  \"reminders_now\": [\n";
    {
        bool first = true;
        for (const auto& cs : state.master.calendar) {
            int event_min = cs.hour * 60 + cs.minute;
            int now_min = tm_now.tm_hour * 60 + tm_now.tm_min;
            int delta = event_min - now_min;
            if (cs.day_of_week == tm_now.tm_wday && delta >= 0 && delta <= 30) {
                if (!first) j << ",\n";
                char tb[8]; snprintf(tb, sizeof(tb), "%02d:%02d", cs.hour, cs.minute);
                j << "    {\"time\": \"" << tb << "\", \"in_minutes\": " << delta
                  << ", \"label\": \"" << json_escape(cs.label)
                  << "\", \"person\": \"" << cs.person_id << "\"}";
                first = false;
            }
        }
        j << (first ? "" : "\n  ") << "],\n";
    }

    /* Tasks open */
    j << "  \"tasks_open\": [";
    {
        bool first = true;
        for (const auto& t : state.master.tasks) {
            if (t.done) continue;
            if (!first) j << ", ";
            j << "{\"id\": " << t.id << ", \"text\": \"" << json_escape(t.text) << "\"}";
            first = false;
        }
        j << "],\n";
    }

    /* Challenges */
    j << "  \"challenges_active\": [";
    {
        bool first = true;
        for (const auto& c : state.master.challenges) {
            if (c.completed) continue;
            if (!first) j << ", ";
            j << "{\"desc\": \"" << json_escape(c.description) << "\", \"progress\": "
              << c.current_count << ", \"target\": " << c.target_count
              << ", \"coins\": " << c.coin_reward << "}";
            first = false;
        }
        j << "],\n";
    }

    /* Daily todos */
    reset_daily_todos(state.master);
    j << "  \"daily_todos\": [";
    {
        bool first = true;
        for (const auto& dt : state.master.daily_todos) {
            if (!first) j << ", ";
            j << "{\"text\": \"" << json_escape(dt.text) << "\", \"done\": " << (dt.done ? "true" : "false") << "}";
            first = false;
        }
        j << "],\n";
    }

    /* Character checklists */
    j << "  \"char_checklists\": [";
    {
        bool first = true;
        for (auto& ci : state.master.char_checklists) {
            /* Auto-reset */
            if (ci.done && ci.repeat_hours > 0 && ci.last_completed > 0) {
                double elapsed_h = difftime(now, ci.last_completed) / 3600.0;
                if (elapsed_h >= ci.repeat_hours) ci.done = false;
            }
            if (!first) j << ", ";
            const Person* cp = find_person(state.persons, ci.person_id);
            j << "{\"person\": \"" << ci.person_id << "\", \"person_name\": \""
              << (cp ? json_escape(cp->name) : ci.person_id)
              << "\", \"text\": \"" << json_escape(ci.text)
              << "\", \"done\": " << (ci.done ? "true" : "false")
              << ", \"repeat_hours\": " << ci.repeat_hours << "}";
            first = false;
        }
        j << "],\n";
    }

    /* Weekly stats */
    auto ws = compute_weekly_stats(state.persons, state.practice_log, 7);
    j << "  \"stats_weekly\": [";
    for (size_t i = 0; i < ws.size(); i++) {
        if (i > 0) j << ", ";
        j << "{\"person\": \"" << ws[i].person_id << "\", \"name\": \"" << ws[i].person_name
          << "\", \"minutes\": " << ws[i].total_minutes << ", \"days_active\": " << ws[i].days_active
          << ", \"streak\": " << ws[i].current_streak << "}";
    }
    j << "],\n";

    /* Recent practice (today) */
    j << "  \"practice_today\": [";
    {
        time_t today_start = now - (now % 86400);
        bool first = true;
        for (const auto& pe : state.practice_log) {
            if (pe.timestamp < today_start) continue;
            if (!first) j << ", ";
            j << "{\"person\": \"" << pe.person_id << "\", \"skill\": \"" << pe.skill_name
              << "\", \"minutes\": " << pe.minutes << "}";
            first = false;
        }
        j << "],\n";
    }

    /* Diary today */
    j << "  \"diary_today\": [";
    {
        bool first = true;
        for (const auto& de : state.master.diary_log) {
            struct tm dtm;
            time_t t = de.timestamp;
            localtime_r(&t, &dtm);
            if (dtm.tm_year == tm_now.tm_year && dtm.tm_mon == tm_now.tm_mon && dtm.tm_mday == tm_now.tm_mday) {
                if (!first) j << ", ";
                j << "{\"time\": \"" << time_to_str(de.timestamp) << "\", \"text\": \"" << json_escape(de.text) << "\"}";
                first = false;
            }
        }
        j << "]\n";
    }

    j << "}\n";

    /* Write to file */
    const char* home = getenv("HOME");
    std::string path = home ? std::string(home) + "/tomogichi-agora.json" : "tomogichi-agora.json";
    std::ofstream f(path);
    if (f.is_open()) {
        f << j.str();
        f.close();
    }

    /* Also update Agora's active_memory.md with a summary for AI context */
    std::string mem_path = home ? std::string(home) + "/.local/share/agora/memories" : "";
    if (!mem_path.empty()) {
        mkdir(mem_path.c_str(), 0755);
        std::string am_path = mem_path + "/active_memory.md";
        std::ofstream am(am_path);
        if (am.is_open()) {
            am << "# Tomogichi Bridge Data\n";
            am << "**Generated:** " << date_buf << " " << time_buf << "\n\n";
            am << "## Guild\n";
            am << "- Level: " << gl << " | Coins: " << state.master.coins
               << " | Entropy: " << state.master.entropy << "/100 | Sync: " << state.master.team_sync << "%\n\n";
            am << "## Characters\n";
            for (const auto& p : state.persons) {
                time_t latest = 0;
                for (const auto& s : p.skills) if (s.last_practice > latest) latest = s.last_practice;
                CharMood mood = char_mood(latest);
                am << "- " << mood_emoji(mood) << " **" << p.name << "** (" << p.role << ") Lvl "
                   << p.level << " " << p.title;
                if (latest > 0) am << " — last practice " << days_since(latest) << "d ago";
                am << "\n";
                for (const auto& s : p.skills) {
                    am << "  - " << s.name << (s.is_main ? " ★" : "") << " lvl " << s.level;
                    if (s.last_practice > 0) am << " (" << days_since(s.last_practice) << "d)";
                    am << "\n";
                }
            }
            am << "\n## Upcoming (next 30 min)\n";
            bool any = false;
            for (const auto& cs : state.master.calendar) {
                int event_min = cs.hour * 60 + cs.minute;
                int now_min = tm_now.tm_hour * 60 + tm_now.tm_min;
                int delta = event_min - now_min;
                if (cs.day_of_week == tm_now.tm_wday && delta >= 0 && delta <= 30) {
                    am << "- 🔔 **" << std::setfill('0') << std::setw(2) << cs.hour << ":" << std::setw(2) << cs.minute << "** — " << cs.label << " (in " << delta << " min)\n";
                    any = true;
                }
            }
            if (!any) am << "- Nothing urgent.\n";
            am << "\n## Today's Calendar\n";
            am << "(See JSON for full schedule)\n";
            am << "\n## Tasks\n";
            for (const auto& t : state.master.tasks) {
                if (!t.done) am << "- [ ] " << t.text << "\n";
            }
            am.close();
        }
    }

    std::cout << "Agora bridge exported to " << path << "\n";
    std::cout << "  → " << j.str().size() << " bytes JSON\n";
    std::cout << "  → Agora active memory updated\n\n";
}

static void cmd_export_json(GameState& state) {
    /* Short JSON without the full calendar expansion */
    std::cout << "For the full JSON bridge file, use: export agora\n\n";
    cmd_export_agora(state);
}

/* --- Command: export --- */

static void cmd_export(GameState& state, std::istringstream& iss) {
    std::string format;
    iss >> format;

    if (format == "csv") {
        std::string path = "data/practice_log.csv";
        std::ofstream f(path);
        f << "date,time,person,skill,minutes\n";
        for (const auto& pe : state.practice_log) {
            char db[16], tb[16];
            struct tm tm;
            localtime_r(&pe.timestamp, &tm);
            strftime(db, sizeof(db), "%Y-%m-%d", &tm);
            strftime(tb, sizeof(tb), "%H:%M:%S", &tm);
            f << db << "," << tb << "," << pe.person_id << ","
              << pe.skill_name << "," << pe.minutes << "\n";
        }
        f.close();
        std::cout << "Exported " << state.practice_log.size()
                  << " entries to " << path << "\n\n";
    }
    else if (format == "agora" || format == "bridge") {
        cmd_export_agora(state);
    }
    else if (format == "json") {
        cmd_export_json(state);
    }
    else {
        std::cout << "Usage: export csv | export agora | export json\n\n";
    }
}

/* --- Command: emergency (SOS → Agora AI) --- */

static void cmd_emergency(GameState& state) {
    time_t now = time(nullptr);
    struct tm tm_now;
    localtime_r(&now, &tm_now);
    char ts[32];
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M", &tm_now);

    /* Write emergency to dedicated file — never overwritten by auto-export */
    const char* home = getenv("HOME");
    std::string emerg_path = home ? std::string(home) + "/agora-emergency.md" : "agora-emergency.md";

    std::ofstream eout(emerg_path);
    if (eout.is_open()) {
        eout << "# 🚨 EMERGENCY PROTOCOL ACTIVE 🚨\n";
        eout << "**Triggered:** " << ts << "\n";
        eout << "**Status:** User reports being in a bad state.\n\n";
        eout << "## ⚠️ AI Instructions\n";
        eout << "1. Analyze the last week of practice data in active_memory below for neglect patterns\n";
        eout << "2. Check which characters are most idle and suggest 5-min easy practice\n";
        eout << "3. Give compassionate, serious support — DO NOT be cheery\n";
        eout << "4. Create a simple recovery plan with one concrete first step\n";
        eout << "5. After responding, remind the user this file exists and ask if they want to clear it\n\n";
        eout << "The user's full Tomogichi state is in the message context.\n";
        eout.close();
    }

    std::cout << "\n";
    std::cout << "\033[1;31m";
    std::cout << "╔══════════════════════════════════════╗\n";
    std::cout << "║     🚨 EMERGENCY PROTOCOL SENT 🚨     ║\n";
    std::cout << "╠══════════════════════════════════════╣\n";
    std::cout << "║ AI will analyze last week + plan     ║\n";
    std::cout << "║ File: ~/agora-emergency.md           ║\n";
    std::cout << "║ Now open Agora and start a chat.     ║\n";
    std::cout << "╚══════════════════════════════════════╝\n";
    std::cout << "\033[0m\n\n";
}

/* --- Command: character checklist (clist) --- */

static void cmd_char_checklist(GameState& state, std::istringstream& iss) {
    std::string sub;
    iss >> sub;

    auto& list = state.master.char_checklists;

    if (sub == "add") {
        std::string pid, text;
        if (iss >> pid) {
            std::string rest;
            getline(iss, rest);
            size_t s = rest.find_first_not_of(" \t");
            if (s != std::string::npos) text = rest.substr(s);
            if (!text.empty()) {
                CharChecklistItem ci;
                ci.person_id = pid;
                ci.text = text;
                ci.done = false;
                ci.repeat_hours = 0;
                ci.last_completed = 0;
                list.push_back(ci);
                std::cout << "Checklist added for " << pid << ": " << text << "\n\n";
            } else {
                std::cout << "Usage: clist add <person> <text>\n\n";
            }
        } else {
            std::cout << "Usage: clist add <person> <text>\n\n";
        }
    }
    else if (sub == "done") {
        int idx;
        if (iss >> idx) {
            idx--;
            if (idx >= 0 && idx < (int)list.size()) {
                list[idx].done = true;
                list[idx].last_completed = time(nullptr);
                std::cout << "Checklist [" << (idx + 1) << "] done: "
                          << list[idx].text << "\n\n";
            } else {
                std::cout << "Invalid index.\n\n";
            }
        } else {
            std::cout << "Usage: clist done <N>\n\n";
        }
    }
    else if (sub == "undo") {
        int idx;
        if (iss >> idx) {
            idx--;
            if (idx >= 0 && idx < (int)list.size()) {
                list[idx].done = false;
                std::cout << "Checklist [" << (idx + 1) << "] reopened.\n\n";
            } else {
                std::cout << "Invalid index.\n\n";
            }
        } else {
            std::cout << "Usage: clist undo <N>\n\n";
        }
    }
    else if (sub == "del" || sub == "delete") {
        int idx;
        if (iss >> idx) {
            idx--;
            if (idx >= 0 && idx < (int)list.size()) {
                std::cout << "Checklist deleted: " << list[idx].text << "\n\n";
                list.erase(list.begin() + idx);
            } else {
                std::cout << "Invalid index.\n\n";
            }
        } else {
            std::cout << "Usage: clist del <N>\n\n";
        }
    }
    else if (sub == "repeat") {
        int idx, hours;
        if (iss >> idx >> hours) {
            idx--;
            if (idx >= 0 && idx < (int)list.size()) {
                list[idx].repeat_hours = hours;
                list[idx].done = false; /* reset so it shows as pending */
                std::cout << "Checklist [" << (idx + 1) << "] repeats every "
                          << hours << "h.\n\n";
            } else {
                std::cout << "Invalid index.\n\n";
            }
        } else {
            std::cout << "Usage: clist repeat <N> <hours>\n";
            std::cout << "  Set 0 hours to disable repeat.\n\n";
        }
    }
    else {
        /* Show all character checklists, grouped by person */
        std::cout << "\n\033[1;36m══════════ Character Checklists ══════════\033[0m\n";

        if (list.empty()) {
            std::cout << "  No checklists.\n";
            std::cout << "  Use: clist add <person> <text>\n";
        }

        std::string last_person;
        int i = 1;
        time_t now = time(nullptr);
        for (auto& ci : list) {
            /* Auto-reset repeating items */
            if (ci.done && ci.repeat_hours > 0 && ci.last_completed > 0) {
                double elapsed_h = difftime(now, ci.last_completed) / 3600.0;
                if (elapsed_h >= ci.repeat_hours) {
                    ci.done = false;
                }
            }

            if (ci.person_id != last_person) {
                const Person* cp = find_person(state.persons, ci.person_id);
                std::string pname = cp ? cp->name : ci.person_id;
                std::cout << "\n  \033[1;33m" << pname << "\033[0m:\n";
                last_person = ci.person_id;
            }

            const char* mark = ci.done ? "\033[32m☑\033[0m" : "\033[33m☐\033[0m";
            std::string style = ci.done ? "\033[90m" : "";
            std::string end_style = ci.done ? "\033[0m" : "";

            std::cout << "  " << i << ". " << mark << " " << style << ci.text << end_style;
            if (ci.repeat_hours > 0) {
                if (ci.done) {
                    double left_h = ci.repeat_hours - difftime(now, ci.last_completed) / 3600.0;
                    if (left_h > 0)
                        std::cout << " \033[36m(repeats in " << (int)left_h << "h)\033[0m";
                } else {
                    std::cout << " \033[36m(every " << ci.repeat_hours << "h)\033[0m";
                }
            }
            std::cout << "\n";
            i++;
        }
        print_line();
        std::cout << "\n  clist add PERSON TEXT  |  clist done N  |  clist undo N\n";
        std::cout << "  clist del N           |  clist repeat N HOURS\n\n";
    }
}

/* --- Command: skill manage --- */

static void cmd_skill_manage(GameState& state, std::istringstream& iss) {
    std::string sub, pid, skname;
    iss >> sub;

    if (sub == "archive" || sub == "retire") {
        iss >> pid >> skname;
        Person* p = find_person(state.persons, pid);
        if (!p) { std::cout << "Unknown person: " << pid << "\n\n"; return; }
        if (archive_skill(*p, skname)) {
            std::cout << "Skill archived: " << p->name << " " << skname
                      << " (XP preserved)\n\n";
        } else {
            std::cout << "Cannot archive (main skill or not found).\n\n";
        }
    }
    else if (sub == "reactivate" || sub == "restore") {
        iss >> pid >> skname;
        Person* p = find_person(state.persons, pid);
        if (!p) { std::cout << "Unknown person: " << pid << "\n\n"; return; }
        if (reactivate_skill(*p, skname)) {
            std::cout << "Skill reactivated: " << p->name << " " << skname << "\n\n";
        } else {
            std::cout << "Skill not found in archive.\n\n";
        }
    }
    else if (sub == "list" || sub == "show") {
        bool any = false;
        for (const auto& p : state.persons) {
            if (!p.archived_skills.empty()) {
                std::cout << "\n" << p.name << " archived:\n";
                for (const auto& s : p.archived_skills) {
                    std::cout << "  " << s.name << " (lvl " << s.level
                              << ", " << s.xp << " XP)\n";
                }
                any = true;
            }
        }
        if (!any) std::cout << "No archived skills.\n";
        std::cout << std::endl;
    }
    else if (sub == "delete" || sub == "del" || sub == "remove") {
        iss >> pid >> skname;
        Person* p = find_person(state.persons, pid);
        if (!p) { std::cout << "Unknown person: " << pid << "\n\n"; return; }
        if (delete_skill(*p, skname)) {
            std::cout << "Skill deleted: " << p->name << " " << skname << "\n\n";
        } else {
            std::cout << "Cannot delete (last active skill or not found).\n\n";
        }
    }
    else if (sub == "rename") {
        std::string newname;
        iss >> pid >> skname >> newname;
        Person* p = find_person(state.persons, pid);
        if (!p) { std::cout << "Unknown person: " << pid << "\n\n"; return; }
        if (rename_skill(*p, skname, newname)) {
            std::cout << "Skill renamed: " << p->name << " " << skname
                      << " → " << newname << "\n\n";
        } else {
            std::cout << "Cannot rename (name conflict or not found).\n\n";
        }
    }
    else {
        std::cout << "Usage:\n";
        std::cout << "  skill archive <person> <skill>  — archive skill (preserve XP)\n";
        std::cout << "  skill reactivate <person> <skill> — restore from archive\n";
        std::cout << "  skill delete <person> <skill>   — permanently delete skill\n";
        std::cout << "  skill rename <person> <old> <new> — rename a skill\n";
        std::cout << "  skill list                      — show archived skills\n\n";
    }
}

static bool parse_command(GameState& state, const std::string& line) {
    std::istringstream iss(line);
    std::string cmd;
    iss >> cmd;

    if (cmd.empty()) return true;

    /* Shorthand: person id alone = start timer for default skill */
    std::string persons[] = {"riff", "reef", "pitch", "rain"};
    for (const auto& pid : persons) {
        if (cmd == pid) {
            /* Check if next arg is a number or a skill name */
            std::string arg;
            if (iss >> arg) {
                /* Try as number first */
                char* end;
                long minutes = strtol(arg.c_str(), &end, 10);
                if (*end == '\0' && minutes > 0) {
                    /* cmdid N → start timer for default skill, planned N min */
                    int mins = (int)minutes;
                    std::string skname = default_skill(pid);
                    timer_start_cmd(state, pid, skname, mins);
                    return true;  /* skip to main loop (timer runs there) */
                } else {
                    /* cmdid skill minutes → start timer for that skill */
                    long m;
                    if (iss >> m) {
                        std::string resolved = resolve_skill(pid, arg);
                        timer_start_cmd(state, pid, resolved, (int)m);
                        return true;
                    } else {
                        std::cout << "Usage: " << pid << " [skill] <minutes>\n";
                        return true;
                    }
                }
            } else {
                std::cout << "Usage: " << pid << " [skill] <minutes>\n";
                return true;
            }
        }
    }

    /* Commands */
    if (cmd == "status" || cmd == "st") {
        cmd_status(state);
    }
    else if (cmd == "history" || cmd == "hist") {
        int days = 7;
        std::string arg;
        if (iss >> arg) {
            char* end;
            long d = strtol(arg.c_str(), &end, 10);
            if (*end == '\0' && d > 0) days = (int)d;
            else {
                std::cout << "Usage: history [days]\n";
                return true;
            }
        }
        cmd_history(state, days);
    }
    else if (cmd == "week") {
        cmd_week(state);
    }
    else if (cmd == "tasks" || cmd == "task") {
        /* task add/done/undo/del? */
        std::string sub;
        if (iss >> sub) {
            if (sub == "add") {
                std::string text;
                getline(iss, text);
                /* Trim leading spaces */
                size_t start = text.find_first_not_of(" \t");
                if (start != std::string::npos) {
                    text = text.substr(start);
                }
                if (text.empty()) {
                    std::cout << "Usage: task add <text>\n";
                } else {
                    cmd_task_add(state, text);
                }
            }
            else if (sub == "done") {
                int id;
                if (iss >> id) cmd_task_done(state, id, true);
                else std::cout << "Usage: task done <id>\n";
            }
            else if (sub == "undo") {
                int id;
                if (iss >> id) cmd_task_done(state, id, false);
                else std::cout << "Usage: task undo <id>\n";
            }
            else if (sub == "del" || sub == "delete") {
                int id;
                if (iss >> id) cmd_task_del(state, id);
                else std::cout << "Usage: task del <id>\n";
            }
            else {
                /* treat as "task N" → done */
                char* end;
                long id = strtol(sub.c_str(), &end, 10);
                if (*end == '\0') {
                    cmd_task_done(state, (int)id, true);
                } else {
                    std::cout << "Unknown: task " << sub << "\n";
                    std::cout << "Use: task add|done|undo|del\n";
                }
            }
        } else {
            cmd_tasks_show(state);
        }
    }
    else if (cmd == "bday" || cmd == "birthday") {
        std::string sub;
        if (iss >> sub) {
            if (sub == "add") {
                std::string name, date;
                if (iss >> name >> date) {
                    cmd_birthday_add(state, name, date);
                } else {
                    std::cout << "Usage: bday add <name> <MM-DD>\n";
                }
            }
            else if (sub == "del" || sub == "delete") {
                std::string name;
                if (iss >> name) {
                    cmd_birthday_del(state, name);
                } else {
                    std::cout << "Usage: bday del <name>\n";
                }
            }
            else {
                std::cout << "Usage: bday add|del\n";
            }
        } else {
            /* Show birthdays (already in tasks) */
            cmd_tasks_show(state);
        }
    }
    else if (cmd == "help" || cmd == "h" || cmd == "?") {
        cmd_help();
    }
    else if (cmd == "daily" || cmd == "dailies") {
        std::string sub, arg1, arg2;
        if (iss >> sub) {
            iss >> arg1;
            /* Get remaining text for add */
            std::string rest;
            getline(iss, rest);
            /* Trim */
            size_t start = rest.find_first_not_of(" \t");
            if (start != std::string::npos) rest = rest.substr(start);
            cmd_daily_action(state.master, sub, arg1, rest);
        } else {
            cmd_daily_show(state.master);
        }
    }
    else if (cmd == "plan" || cmd == "today" || cmd == "cal" || cmd == "calendar") {
        std::string sub;
        if (iss >> sub) {
            if (sub == "add") {
                cmd_plan_add(state, iss);
            }
            else if (sub == "del" || sub == "delete") {
                int idx;
                if (iss >> idx) {
                    idx--;
                    if (idx >= 0 && idx < (int)state.master.calendar.size()) {
                        std::cout << "Deleted: " << state.master.calendar[idx].label << "\n\n";
                        state.master.calendar.erase(state.master.calendar.begin() + idx);
                    } else {
                        std::cout << "Invalid index.\n\n";
                    }
                }
            }
            else if (sub == "today" || sub == "now") {
                cmd_plan_show(state);
            }
            else {
                cmd_calendar_show(state.master);
            }
        } else {
            cmd_calendar_show(state.master);
        }
    }
    else if (cmd == "chal" || cmd == "challenge" || cmd == "challenges") {
        std::string sub;
        if (iss >> sub) {
            if (sub == "add") {
                /* chal add <desc> <person> <skill> <count> <days> <reward> */
                std::string desc, pid, skname;
                int count = 0, days = 0, reward = 0;

                /* Read quoted description or single word */
                if (iss >> desc) {
                    if (desc[0] == '"') {
                        /* Quoted: keep reading until closing quote */
                        std::string word;
                        desc = desc.substr(1);
                        while (iss >> word) {
                            if (!word.empty() && word.back() == '"') {
                                desc += " " + word.substr(0, word.size() - 1);
                                break;
                            }
                            desc += " " + word;
                        }
                    }
                    iss >> pid >> skname >> count >> days >> reward;

                    if (pid.empty() || count <= 0) {
                        std::cout << "Usage: chal add <desc> <person> <skill> <count> <days> <reward>\n";
                        std::cout << "Example: chal add \"Draw daily\" riff drawing 7 7 50\n";
                        std::cout << "Use - for person/skill to match any\n\n";
                    } else {
                        if (pid == "-") pid = "";
                        if (skname == "-") skname = "";
                        Challenge c;
                        c.id = "chal_" + std::to_string(state.master.challenges.size() + 1);
                        c.description = desc;
                        c.person_id = pid;
                        c.skill_name = skname;
                        c.target_count = count;
                        c.target_days = days;
                        c.coin_reward = reward;
                        c.current_count = 0;
                        c.completed = false;
                        c.started = time(nullptr);
                        state.master.challenges.push_back(c);
                        std::cout << "Challenge added: " << desc
                                  << " [" << count << "x in " << days << "d, +" << reward << "💰]\n\n";
                    }
                }
            }
            else if (sub == "del" || sub == "delete") {
                int idx;
                if (iss >> idx) {
                    idx--;
                    if (idx >= 0 && idx < (int)state.master.challenges.size()) {
                        std::cout << "Challenge deleted: "
                                  << state.master.challenges[idx].description << "\n\n";
                        state.master.challenges.erase(state.master.challenges.begin() + idx);
                    } else {
                        std::cout << "Invalid index.\n\n";
                    }
                }
            }
            else {
                cmd_challenge_show(state);
            }
        } else {
            cmd_challenge_show(state);
        }
    }
    else if (cmd == "coins" || cmd == "money") {
        std::cout << "\n\033[1;33m💰 Coins: " << state.master.coins << "\033[0m\n\n";
    }
    else if (cmd == "shop") {
        std::cout << "\n\033[1;36m══════════ Shop ══════════\033[0m\n\n";
        std::cout << "\033[1;33m💰 Coins: " << state.master.coins << "\033[0m\n\n";
        /* Static list matching Qt backend */
        struct { const char *id, *name, *desc; int cost; } items[] = {
            {"entropy_reset", "Entropy Reset", "Reset team entropy to 0", 50},
            {"xp_boost",      "XP Boost",      "+50% XP for next practice", 20},
            {"skip_day",      "Skip Day",      "No entropy penalty today", 30},
            {"custom_title",  "Custom Title",  "Set custom title for one character", 100},
            {"coins_double",  "Coin Doubler",  "Double coins from next challenge", 40},
        };
        for (const auto &it : items) {
            bool can = state.master.coins >= it.cost;
            std::cout << "  " << (can ? "\033[32m" : "\033[90m")
                      << it.name << " — " << it.cost << " 💰\033[0m\n"
                      << "    " << it.desc << "\n";
        }
        std::cout << "\n  \033[90mUse 'buy <item>' to purchase.\033[0m\n\n";
    }
    else if (cmd == "buy") {
        std::string item;
        if (iss >> item) {
            int cost = -1; bool found = false;
            struct { const char *id; int cost; } items[] = {
                {"entropy_reset", 50}, {"xp_boost", 20}, {"skip_day", 30},
                {"custom_title", 100}, {"coins_double", 40}
            };
            for (const auto &it : items) {
                if (item == it.id) { cost = it.cost; found = true; break; }
            }
            if (!found) {
                std::cout << "Unknown item. Use 'shop' to see available items.\n\n";
            } else if (state.master.coins < cost) {
                std::cout << "Not enough coins! Need " << cost << ", have " << state.master.coins << ".\n\n";
            } else {
                state.master.coins -= cost;
                if (item == "entropy_reset") {
                    state.master.entropy = 0;
                    std::cout << "Entropy reset to 0!\n\n";
                } else if (item == "xp_boost") {
                    state.master.xp_boost = true;
                    std::cout << "XP Boost activated! +50% on next practice.\n\n";
                } else if (item == "skip_day") {
                    state.master.entropy = state.master.entropy > 0 ? state.master.entropy - 5 : 0;
                    state.master.entropy_updated = time(nullptr);
                    std::cout << "Day skipped! Entropy reduced.\n\n";
                } else if (item == "custom_title") {
                    std::string pid;
                    if (iss >> pid) {
                        Person *p = find_person(state.persons, pid);
                        if (p) { p->title = "★ Custom ★"; std::cout << "Custom title set for " << pid << "!\n\n"; }
                        else std::cout << "Unknown person.\n\n";
                    } else std::cout << "Usage: buy custom_title <person>\n\n";
                } else if (item == "coins_double") {
                    state.master.coins_double = true;
                    std::cout << "Coin Doubler activated! ×2 coins from next challenge.\n\n";
                }
            }
        } else {
            std::cout << "Usage: buy <item>\nUse 'shop' to see available items.\n\n";
        }
    }
    else if (cmd == "notify" || cmd == "note") {
        std::string title, body;
        if (iss >> title) {
            getline(iss, body);
            size_t s = body.find_first_not_of(" \t");
            if (s != std::string::npos) body = body.substr(s);
            if (notify_send(title, body)) {
                std::cout << "Notification sent: " << title << "\n\n";
            } else {
                std::cout << "Failed to send notification (notify-send not found?)\n\n";
            }
        } else {
            std::cout << "Usage: notify <title> [body]\n\n";
        }
    }
    else if (cmd == "stats" || cmd == "stat") {
        std::string sub;
        if (iss >> sub) {
            if (sub == "fx" || sub == "effects" || sub == "effect") {
                cmd_stats_fx(state);
            } else {
                cmd_stats(state);
            }
        } else {
            cmd_stats(state);
        }
    }
    else if (cmd == "ach" || cmd == "achievements" || cmd == "badges") {
        cmd_achievements(state);
    }
    else if (cmd == "skill") {
        cmd_skill_manage(state, iss);
    }
    else if (cmd == "clist" || cmd == "checklist") {
        cmd_char_checklist(state, iss);
    }
    else if (cmd == "rename") {
        std::string pid, newname;
        if (iss >> pid) {
            std::string rest;
            getline(iss, rest);
            size_t s = rest.find_first_not_of(" \t");
            if (s != std::string::npos) newname = rest.substr(s);
            if (!newname.empty()) {
                Person* rp = find_person(state.persons, pid);
                if (rp) {
                    rename_person(*rp, newname);
                    std::cout << "Character renamed: " << pid << " → " << newname << "\n\n";
                } else {
                    std::cout << "Unknown person: " << pid << "\n\n";
                }
            } else {
                std::cout << "Usage: rename <person> <newname>\n";
                std::cout << "Example: rename riff Riffy\n\n";
            }
        } else {
            std::cout << "Usage: rename <person> <newname>\n\n";
        }
    }
    else if (cmd == "theme") {
        std::string sub;
        if (iss >> sub) {
            if (sub == "dark" || sub == "2") { state.master.theme = 2; std::cout << "Theme: Dark\n\n"; }
            else if (sub == "light" || sub == "1") { state.master.theme = 1; std::cout << "Theme: Light\n\n"; }
            else if (sub == "system" || sub == "auto" || sub == "0") { state.master.theme = 0; std::cout << "Theme: System\n\n"; }
            else { std::cout << "Usage: theme [system|light|dark|0|1|2]\n\n"; }
        } else {
            const char* names[] = {"System","Light","Dark"};
            std::cout << "Theme: " << names[state.master.theme % 3] << "\n\n";
        }
    }
    else if (cmd == "emergency" || cmd == "sos") {
        cmd_emergency(state);
    }
    else if (cmd == "mood") {
        cmd_mood(state, iss);
    }
    else if (cmd == "diary" || cmd == "journal") {
        cmd_diary(state, iss);
    }
    else if (cmd == "stodo") {
        cmd_skill_todo(state, iss);
    }
    else if (cmd == "backup") {
        cmd_backup();
    }
    else if (cmd == "export") {
        cmd_export(state, iss);
    }
    else if (cmd == "quit" || cmd == "q" || cmd == "exit") {
        return false;
    }
    else if (cmd == "timer" || cmd == "t") {
        /* Timer commands handled in main() */
        std::cout << "Timer: use 'timer start <person> [skill]' or 'timer stop'\n";
    }
    else {
        std::cout << "Unknown command: " << cmd << "\n";
        std::cout << "Type 'help' for commands.\n\n";
    }

    return true;
}

/* --- Main --- */

int main(int argc, char **argv) {
    /* Ensure data directory exists */
    mkdir("data", 0755);

    /* Handle flags */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0) {
            std::cout << "tomogichi v0.5.0\n";
            return 0;
        }
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            std::cout << "tomogichi — Personal RPG System\n"
                      << "Usage: tomogichi [--version] [--help]\n"
                      << "Start without arguments for interactive mode.\n";
            return 0;
        }
    }

    GameState state;

    if (!load_state(STATE_PATH, state)) {
        std::cout << "No saved state found, creating default...\n";
        state = default_state();
    }
    cmd_export_agora(state);

    std::cout << "\033[1;36m"
              << "══════════════════════════════════\n"
              << "  Tomogichi — Personal RPG System\n"
              << "══════════════════════════════════\033[0m\n";
    std::cout << "Type 'help' for commands, 'status' to see your characters.\n";

    /* Daily reset */
    if (reset_daily_todos(state.master)) {
        std::cout << "\033[33m🌅 New day! Daily todos reset.\033[0m\n";
    }
    update_entropy_sync(state.master, state.practice_log);
    float mult = xp_multiplier(state.master);
    if (state.master.entropy > 50) {
        std::cout << "\033[31m⚠ Entropy: " << state.master.entropy
                  << "/100 — XP ×" << std::fixed << std::setprecision(2)
                  << mult << "\033[0m\n";
    }
    if (state.master.coins > 0) {
        std::cout << "\033[33m💰 Coins: " << state.master.coins << "\033[0m\n";
    }

    /* Neglect warnings (skip if never practiced — they're new, not neglected) */
    const Person* neglected = most_neglected(state.persons);
    if (neglected) {
        time_t latest = 0;
        for (const auto& s : neglected->skills) {
            if (s.last_practice > latest) latest = s.last_practice;
        }
        if (latest > 0) {
            CharMood mood = char_mood(latest);
            if (mood >= MOOD_SAD) {
                std::cout << "\033[31m" << mood_emoji(mood) << " "
                          << char_message(*neglected, mood) << "\033[0m\n";
                /* Send desktop notification if available (only if DISPLAY is set) */
                if (getenv("DISPLAY")) {
                    notify_send(std::string(mood_emoji(mood)) + " " + neglected->name,
                                char_message(*neglected, mood));
                }
            }
        }
    }
    std::cout << std::endl;

    /* Reset timer on startup */
    timer_running = false;
    time_t session_start = time(nullptr);

    /* Main loop */
    std::string line;
    while (true) {
        /* Show timer indicator */
        if (timer_running) {
            time_t elapsed_t = time(nullptr) - timer_start;
            int min = (int)(elapsed_t / 60);
            int sec = (int)(elapsed_t % 60);

            /* Overdue warning */
            if (timer_planned > 0 && min > timer_planned) {
                std::cout << "\033[1;31m[⏱ OVERDUE " << min << "m (planned "
                          << timer_planned << "m)]\033[0m\n";
            } else {
                std::cout << "\033[1;35m[⏱ " << min << ":"
                          << (sec < 10 ? "0" : "") << sec
                          << " " << timer_person << " " << timer_skill
                          << "]\033[0m\n";
            }
        } else {
            /* Master passive timer with calendar info */
            time_t elapsed = time(nullptr) - session_start;
            int min = (int)(elapsed / 60);
            auto* slot = current_calendar_slot(state.master);
            std::cout << "\033[90m[⏱ " << min << "m";
            if (slot) {
                std::cout << " 📅 " << slot->label;
            }
            std::cout << "]\033[0m\n";
        }
        std::cout << "\033[1;32mtomo>\033[0m ";
        if (!std::getline(std::cin, line)) break;
        if (line.empty()) continue;

        /* --- Timer commands (handled first) --- */
        std::istringstream tiss(line);
        std::string tcmd;
        tiss >> tcmd;

        if (tcmd == "timer" || tcmd == "t") {
            std::string sub;
            tiss >> sub;

            if (sub == "start" || sub == "go") {
                std::string pid, skname;
                tiss >> pid;
                if (pid.empty()) {
                    std::cout << "Usage: timer start <person> [skill]\n\n";
                    continue;
                }
                if (tiss >> skname) {
                    skname = resolve_skill(pid, skname);
                } else {
                    skname = default_skill(pid);
                }
                /* Use planned=0 since no explicit minutes given */
                timer_start_cmd(state, pid, skname, 0);
                continue;
            }
            else if (sub == "stop" || sub == "done") {
                if (!timer_running) {
                    std::cout << "No timer running.\n\n";
                    continue;
                }
                time_t elapsed_t = time(nullptr) - timer_start;
                int elapsed_min = (int)(elapsed_t / 60);
                if (elapsed_min < 1) elapsed_min = 1; /* minimum 1 minute */
                std::cout << "Timer stopped: " << elapsed_min << " min\n";
                cmd_practice(state, timer_person, timer_skill, elapsed_min);
                timer_running = false;
                if (!save_state(STATE_PATH, state)) {
                    std::cout << "\033[31mWarning: failed to save!\033[0m\n";
                }
                cmd_export_agora(state);
                continue;
            }
            else if (sub == "cancel" || sub == "abort") {
                if (!timer_running) {
                    std::cout << "No timer running.\n\n";
                    continue;
                }
                time_t elapsed_t = time(nullptr) - timer_start;
                int elapsed_min = (int)(elapsed_t / 60);
                std::cout << "Timer cancelled (" << elapsed_min << " min discarded).\n\n";
                timer_running = false;
                continue;
            }
            else if (sub == "status" || sub.empty()) {
                if (!timer_running) {
                    std::cout << "No timer running.\n\n";
                } else {
                    time_t elapsed_t = time(nullptr) - timer_start;
                    int min = (int)(elapsed_t / 60);
                    int sec = (int)(elapsed_t % 60);
                    std::cout << "Timer: " << timer_person << " " << timer_skill
                              << " — " << min << ":" << (sec < 10 ? "0" : "")
                              << sec << "\n\n";
                }
                continue;
            }
            else {
                std::cout << "Timer: start|stop|cancel|status\n\n";
                continue;
            }
        }

        /* If any practice command while timer is running, it auto-restarts */
        /* (No blocking — person commands now start timer, handled above) */

        if (!parse_command(state, line)) break;

        /* Auto-save after each command */
        if (!save_state(STATE_PATH, state)) {
            std::cout << "\033[31mWarning: failed to save state!\033[0m\n";
        }
        /* Auto-export Agora bridge JSON (skip after emergency — it handles its own) */
        if (!emergency_triggered) {
            cmd_export_agora(state);
        }
        emergency_triggered = false;
    }

    /* Warn if timer still running on exit */
    if (timer_running) {
        time_t elapsed_t = time(nullptr) - timer_start;
        int elapsed_min = (int)(elapsed_t / 60);
        std::cout << "Timer was running (" << elapsed_min << " min). Logging...\n";
        if (elapsed_min > 0) {
            cmd_practice(state, timer_person, timer_skill, elapsed_min, false);
        }
    }

    /* Final save */
    if (save_state(STATE_PATH, state)) {
        std::cout << "State saved.\n";
    } else {
        std::cout << "\033[31mFailed to save state!\033[0m\n";
    }
    cmd_export_agora(state);

    std::cout << "Goodbye.\n";
    return 0;
}
