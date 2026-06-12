#include "storage.h"
#include "../core/engine.h"
#include "../json.hpp"
#include <fstream>
#include <cstdio>
#include <ctime>
#include <algorithm>

using json = nlohmann::json;

/* --- Helpers --- */

static std::string time_to_iso(time_t t) {
    char buf[32];
    struct tm* tm = localtime(&t);
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", tm);
    return std::string(buf);
}

static time_t iso_to_time(const std::string& s) {
    struct tm tm = {};
    if (s.size() < 19) return 0;
    /* Parse "YYYY-MM-DDTHH:MM:SS" */
    sscanf(s.c_str(), "%d-%d-%dT%d:%d:%d",
           &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
           &tm.tm_hour, &tm.tm_min, &tm.tm_sec);
    tm.tm_year -= 1900;
    tm.tm_mon -= 1;
    tm.tm_isdst = -1;
    return mktime(&tm);
}

/* --- Load --- */

bool load_state(const std::string& path, GameState& state) {
    std::ifstream f(path);
    if (!f.is_open()) return false;

    try {
        json j;
        f >> j;
        f.close();

        state = default_state(); /* start fresh, overwrite below */

        /* Persons */
        if (j.contains("persons") && j["persons"].is_array()) {
            state.persons.clear();
            for (const auto& jp : j["persons"]) {
                Person p;
                p.id = jp.value("id", "");
                p.name = jp.value("name", "");
                p.role = jp.value("role", "");

                if (jp.contains("skills")) {
                    for (const auto& js : jp["skills"]) {
                        Skill sk;
                        sk.name = js.value("name", "");
                        sk.xp = js.value("xp", 0);
                        sk.is_main = js.value("is_main", false);
                        sk.bonus = js.value("bonus", 1.0f);
                        if (js.contains("last") && !js["last"].is_null()) {
                            sk.last_practice = iso_to_time(js["last"].get<std::string>());
                        } else {
                            sk.last_practice = 0;
                        }
                        sk.archived = js.value("archived", false);
                        sk.level = xp_to_level(sk.xp);
                        p.skills.push_back(sk);
                    }
                }
                /* Archived skills */
                if (jp.contains("archived_skills")) {
                    for (const auto& js : jp["archived_skills"]) {
                        Skill sk;
                        sk.name = js.value("name", "");
                        sk.xp = js.value("xp", 0);
                        sk.is_main = js.value("is_main", false);
                        sk.bonus = js.value("bonus", 1.0f);
                        sk.archived = true;
                        if (js.contains("last") && !js["last"].is_null()) {
                            sk.last_practice = iso_to_time(js["last"].get<std::string>());
                        }
                        sk.level = xp_to_level(sk.xp);
                        p.archived_skills.push_back(sk);
                    }
                }
                p.level = calc_person_level(p.skills);
                p.title = title_for_level(p.level);
                state.persons.push_back(p);
            }
        }

        /* Cross bonuses */
        if (j.contains("cross_bonuses") && j["cross_bonuses"].is_array()) {
            state.active_cross_bonuses.clear();
            for (const auto& cb : j["cross_bonuses"]) {
                state.active_cross_bonuses.push_back(cb.get<std::string>());
            }
        }

        /* Master tasks */
        if (j.contains("master") && j["master"].is_object()) {
            const auto& jm = j["master"];
            if (jm.contains("tasks")) {
                state.master.tasks.clear();
                for (const auto& jt : jm["tasks"]) {
                    Task t;
                    t.id = jt.value("id", 0);
                    t.text = jt.value("text", "");
                    t.done = jt.value("done", false);
                    if (jt.contains("created") && !jt["created"].is_null()) {
                        t.created = iso_to_time(jt["created"].get<std::string>());
                    } else {
                        t.created = 0;
                    }
                    state.master.tasks.push_back(t);
                }
            }
            if (jm.contains("birthdays")) {
                state.master.birthdays.clear();
                for (const auto& jb : jm["birthdays"]) {
                    Birthday b;
                    b.name = jb.value("name", "");
                    b.date = jb.value("date", "");
                    state.master.birthdays.push_back(b);
                }
            }
            state.master.next_task_id = jm.value("next_task_id", 1);
            state.master.coins = jm.value("coins", 0);
            state.master.daily_reset = 0;
            if (jm.contains("daily_reset") && !jm["daily_reset"].is_null()) {
                state.master.daily_reset = iso_to_time(jm["daily_reset"].get<std::string>());
            }
            state.master.entropy = jm.value("entropy", 0);
            state.master.team_sync = jm.value("team_sync", 0);
            state.master.entropy_updated = 0;
            if (jm.contains("entropy_updated") && !jm["entropy_updated"].is_null()) {
                state.master.entropy_updated = iso_to_time(jm["entropy_updated"].get<std::string>());
            }
            state.master.xp_boost = jm.value("xp_boost", false);
            state.master.coins_double = jm.value("coins_double", false);
            state.master.onboarding_seen = jm.value("onboarding_seen", false);
            state.master.theme = jm.value("theme", 0);

            /* Daily todos */
            if (jm.contains("daily_todos")) {
                state.master.daily_todos.clear();
                for (const auto& jd : jm["daily_todos"]) {
                    DailyTodo dt;
                    dt.text = jd.value("text", "");
                    dt.done = jd.value("done", false);
                    state.master.daily_todos.push_back(dt);
                }
            }

            /* Calendar */
            if (jm.contains("calendar")) {
                state.master.calendar.clear();
                for (const auto& jc : jm["calendar"]) {
                    CalendarSlot cs;
                    cs.day_of_week = jc.value("day", 0);
                    cs.hour = jc.value("hour", 0);
                    cs.minute = jc.value("minute", 0);
                    cs.person_id = jc.value("person", "master");
                    cs.skill_name = jc.value("skill", "");
                    cs.label = jc.value("label", "");
                    state.master.calendar.push_back(cs);
                }
            }

            /* Challenges */
            if (jm.contains("challenges")) {
                state.master.challenges.clear();
                for (const auto& jch : jm["challenges"]) {
                    Challenge c;
                    c.id = jch.value("id", "");
                    c.description = jch.value("description", "");
                    c.person_id = jch.value("person", "");
                    c.skill_name = jch.value("skill", "");
                    c.target_count = jch.value("target_count", 0);
                    c.target_days = jch.value("target_days", 0);
                    c.coin_reward = jch.value("coin_reward", 0);
                    c.current_count = jch.value("current_count", 0);
                    c.completed = jch.value("completed", false);
                    c.started = 0;
                    if (jch.contains("started") && !jch["started"].is_null()) {
                        c.started = iso_to_time(jch["started"].get<std::string>());
                    }
                    state.master.challenges.push_back(c);
                }
            }

            /* Achievements */
            if (jm.contains("achievements")) {
                state.master.achievements.clear();
                for (const auto& ja : jm["achievements"]) {
                    Achievement a;
                    a.id = ja.value("id", "");
                    a.name = ja.value("name", "");
                    a.description = ja.value("description", "");
                    a.icon = ja.value("icon", "");
                    a.unlocked = ja.value("unlocked", false);
                    a.unlocked_at = 0;
                    if (ja.contains("unlocked_at") && !ja["unlocked_at"].is_null()) {
                        a.unlocked_at = iso_to_time(ja["unlocked_at"].get<std::string>());
                    }
                    state.master.achievements.push_back(a);
                }
            }

            /* Skill todos */
            if (jm.contains("skill_todos")) {
                state.master.skill_todos.clear();
                for (const auto& js : jm["skill_todos"]) {
                    SkillTodo st;
                    st.person_id = js.value("person", "");
                    st.skill_name = js.value("skill", "");
                    st.text = js.value("text", "");
                    st.done = js.value("done", false);
                    state.master.skill_todos.push_back(st);
                }
            }

            /* Mood log */
            if (jm.contains("mood_log")) {
                state.master.mood_log.clear();
                for (const auto& jmood : jm["mood_log"]) {
                    MoodEntry me;
                    me.word = jmood.value("word", "");
                    me.timestamp = iso_to_time(jmood.value("time", ""));
                    state.master.mood_log.push_back(me);
                }
            }

            /* Diary log */
            if (jm.contains("diary_log")) {
                state.master.diary_log.clear();
                for (const auto& jd : jm["diary_log"]) {
                    DiaryEntry de;
                    de.text = jd.value("text", "");
                    de.timestamp = iso_to_time(jd.value("time", ""));
                    state.master.diary_log.push_back(de);
                }
            }
        }

        /* Practice log */
        if (j.contains("practice_log") && j["practice_log"].is_array()) {
            state.practice_log.clear();
            for (const auto& jl : j["practice_log"]) {
                PracticeEntry pe;
                pe.person_id = jl.value("person", "");
                pe.skill_name = jl.value("skill", "");
                pe.minutes = jl.value("minutes", 0);
                pe.timestamp = iso_to_time(jl.value("time", ""));
                std::string eff = jl.value("effect", "");
                if (eff == "energized") pe.effect = EFFECT_ENERGIZED;
                else if (eff == "tired") pe.effect = EFFECT_TIRED;
                else if (eff == "drained") pe.effect = EFFECT_DRAINED;
                else pe.effect = EFFECT_NEUTRAL;
                pe.notes = jl.value("notes", "");
                state.practice_log.push_back(pe);
            }
        }

        return true;
    } catch (const std::exception& e) {
        (void)e;
        return false;
    }
}

/* --- Save --- */

bool save_state(const std::string& path, const GameState& state) {
    json j;

    /* Persons */
    j["persons"] = json::array();
    for (const auto& p : state.persons) {
        json jp;
        jp["id"] = p.id;
        jp["name"] = p.name;
        jp["role"] = p.role;
        jp["skills"] = json::array();
        for (const auto& s : p.skills) {
            json js;
            js["name"] = s.name;
            js["xp"] = s.xp;
            js["is_main"] = s.is_main;
            js["bonus"] = s.bonus;
            if (s.last_practice > 0) {
                js["last"] = time_to_iso(s.last_practice);
            }
            js["archived"] = s.archived;
            jp["skills"].push_back(js);
        }
        /* Archived skills */
        jp["archived_skills"] = json::array();
        for (const auto& s : p.archived_skills) {
            json js;
            js["name"] = s.name;
            js["xp"] = s.xp;
            js["is_main"] = s.is_main;
            js["bonus"] = s.bonus;
            js["archived"] = true;
            if (s.last_practice > 0) js["last"] = time_to_iso(s.last_practice);
            jp["archived_skills"].push_back(js);
        }
        j["persons"].push_back(jp);
    }

    /* Cross bonuses */
    j["cross_bonuses"] = state.active_cross_bonuses;

    /* Master */
    json jm;
    jm["tasks"] = json::array();
    for (const auto& t : state.master.tasks) {
        json jt;
        jt["id"] = t.id;
        jt["text"] = t.text;
        jt["done"] = t.done;
        if (t.created > 0) jt["created"] = time_to_iso(t.created);
        jm["tasks"].push_back(jt);
    }
    jm["birthdays"] = json::array();
    for (const auto& b : state.master.birthdays) {
        json jb;
        jb["name"] = b.name;
        jb["date"] = b.date;
        jm["birthdays"].push_back(jb);
    }
    jm["next_task_id"] = state.master.next_task_id;
    jm["coins"] = state.master.coins;
    if (state.master.daily_reset > 0) {
        jm["daily_reset"] = time_to_iso(state.master.daily_reset);
    }
    jm["entropy"] = state.master.entropy;
    jm["team_sync"] = state.master.team_sync;
    if (state.master.entropy_updated > 0) {
        jm["entropy_updated"] = time_to_iso(state.master.entropy_updated);
    }
    jm["xp_boost"] = state.master.xp_boost;
    jm["coins_double"] = state.master.coins_double;
    jm["onboarding_seen"] = state.master.onboarding_seen;
    jm["theme"] = state.master.theme;

    /* Daily todos */
    jm["daily_todos"] = json::array();
    for (const auto& dt : state.master.daily_todos) {
        json jd;
        jd["text"] = dt.text;
        jd["done"] = dt.done;
        jm["daily_todos"].push_back(jd);
    }

    /* Calendar */
    jm["calendar"] = json::array();
    for (const auto& cs : state.master.calendar) {
        json jc;
        jc["day"] = cs.day_of_week;
        jc["hour"] = cs.hour;
        jc["minute"] = cs.minute;
        jc["person"] = cs.person_id;
        jc["skill"] = cs.skill_name;
        jc["label"] = cs.label;
        jm["calendar"].push_back(jc);
    }

    /* Challenges */
    jm["challenges"] = json::array();
    for (const auto& c : state.master.challenges) {
        json jch;
        jch["id"] = c.id;
        jch["description"] = c.description;
        jch["person"] = c.person_id;
        jch["skill"] = c.skill_name;
        jch["target_count"] = c.target_count;
        jch["target_days"] = c.target_days;
        jch["coin_reward"] = c.coin_reward;
        jch["current_count"] = c.current_count;
        jch["completed"] = c.completed;
        if (c.started > 0) jch["started"] = time_to_iso(c.started);
        jm["challenges"].push_back(jch);
    }

    /* Achievements */
    jm["achievements"] = json::array();
    for (const auto& a : state.master.achievements) {
        json ja;
        ja["id"] = a.id;
        ja["name"] = a.name;
        ja["description"] = a.description;
        ja["icon"] = a.icon;
        ja["unlocked"] = a.unlocked;
        if (a.unlocked_at > 0) ja["unlocked_at"] = time_to_iso(a.unlocked_at);
        jm["achievements"].push_back(ja);
    }

    /* Skill todos */
    jm["skill_todos"] = json::array();
    for (const auto& st : state.master.skill_todos) {
        json js;
        js["person"] = st.person_id;
        js["skill"] = st.skill_name;
        js["text"] = st.text;
        js["done"] = st.done;
        jm["skill_todos"].push_back(js);
    }

    /* Mood log */
    jm["mood_log"] = json::array();
    for (const auto& me : state.master.mood_log) {
        json jmood;
        jmood["word"] = me.word;
        jmood["time"] = time_to_iso(me.timestamp);
        jm["mood_log"].push_back(jmood);
    }

    /* Diary log */
    jm["diary_log"] = json::array();
    for (const auto& de : state.master.diary_log) {
        json jd;
        jd["text"] = de.text;
        jd["time"] = time_to_iso(de.timestamp);
        jm["diary_log"].push_back(jd);
    }

    j["master"] = jm;

    /* Practice log */
    j["practice_log"] = json::array();
    for (const auto& pe : state.practice_log) {
        json jl;
        jl["person"] = pe.person_id;
        jl["skill"] = pe.skill_name;
        jl["minutes"] = pe.minutes;
        jl["time"] = time_to_iso(pe.timestamp);
        switch (pe.effect) {
            case EFFECT_ENERGIZED: jl["effect"] = "energized"; break;
            case EFFECT_TIRED:     jl["effect"] = "tired"; break;
            case EFFECT_DRAINED:   jl["effect"] = "drained"; break;
            default:               jl["effect"] = "neutral"; break;
        }
        if (!pe.notes.empty()) {
            jl["notes"] = pe.notes;
        }
        j["practice_log"].push_back(jl);
    }

    /* Atomic write: tmp file, then rename */
    std::string tmp_path = path + ".tmp";
    std::ofstream f(tmp_path);
    if (!f.is_open()) return false;

    f << j.dump(2) << std::endl;
    f.close();

    /* Backup old */
    std::string bak_path = path + ".bak";
    std::rename(path.c_str(), bak_path.c_str());

    /* Move tmp to real */
    if (std::rename(tmp_path.c_str(), path.c_str()) != 0) {
        /* Restore backup */
        std::rename(bak_path.c_str(), path.c_str());
        return false;
    }

    return true;
}

/* --- Default State --- */

GameState default_state() {
    GameState state;
    state.persons = { make_riff(), make_reef(), make_pitch(), make_rain() };
    state.master.next_task_id = 1;
    state.master.coins = 0;
    state.master.daily_reset = 0;
    state.master.theme = 0;    /* 0=system, 1=light, 2=dark */
    return state;
}

/* --- Practice Logging --- */

void log_practice(GameState& state, const std::string& person_id,
                  const std::string& skill_name, int minutes,
                  Effect effect,
                  const std::string& notes) {
    PracticeEntry pe;
    pe.person_id = person_id;
    pe.skill_name = skill_name;
    pe.minutes = minutes;
    pe.timestamp = time(nullptr);
    pe.effect = effect;
    pe.notes = notes;
    state.practice_log.push_back(pe);
}
