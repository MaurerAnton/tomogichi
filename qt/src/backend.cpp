#include "backend.h"
#include "core/engine.h"
#include "data/storage.h"
#include <cmath>
#include <algorithm>
#include <fstream>

Backend::Backend(QObject *parent) : QObject(parent) {
    connect(&m_tickTimer, &QTimer::timeout, this, &Backend::tick);
}

Backend::~Backend() {
    saveState();
}

bool Backend::loadState(const QString &path) {
    m_statePath = path.toStdString();
    if (!::load_state(m_statePath, m_state)) {
        /* Try restoring from backup */
        std::string bakPath = m_statePath + ".bak";
        if (!::load_state(bakPath, m_state)) {
            m_state = default_state();
        }
    }

    /* Reset daily todos if new day */
    reset_daily_todos(m_state.master);

    /* Update entropy & sync for the new day */
    update_entropy_sync(m_state.master, m_state.practice_log);

    emit personsChanged();
    emit dailyTodosChanged();
    emit calendarSlotsChanged();
    emit coinsChanged();
    return true;
}

bool Backend::saveState() {
    return ::save_state(m_statePath, m_state);
}

/* --- Persons --- */

QVariantMap Backend::skillToVariant(const Skill &s) {
    QVariantMap m;
    m["name"] = QString::fromStdString(s.name);
    m["xp"] = s.xp;
    m["level"] = s.level;
    m["isMain"] = s.is_main;
    m["bonus"] = s.bonus;
    m["daysSince"] = days_since(s.last_practice);
    m["progressPct"] = xp_progress_pct(s.xp);
    Warmth w = skill_warmth(s.last_practice);
    m["warmth"] = (w == WARM) ? "warm" : (w == COOL) ? "cool" : "cold";
    m["archived"] = s.archived;
    return m;
}

QVariantMap Backend::personToVariant(const Person &p, const std::vector<Person> &all) const {
    QVariantMap m;
    m["id"] = QString::fromStdString(p.id);
    m["name"] = QString::fromStdString(p.name);
    m["role"] = QString::fromStdString(p.role);
    m["level"] = p.level;
    m["title"] = QString::fromStdString(p.title);

    /* Find latest practice for mood */
    time_t latest = 0;
    for (const auto &s : p.skills) {
        if (s.last_practice > latest) latest = s.last_practice;
    }
    CharMood mood = char_mood(latest);
    m["moodEmoji"] = QString(mood_emoji(mood));
    m["moodText"] = QString(mood_text(mood));

    /* Guild level */
    m["guildLevel"] = calc_guild_level(all);

    /* Check if practiced today */
    time_t now = time(nullptr);
    time_t today_start = now - (now % 86400);
    bool practiced_today = false;
    for (const auto &pe : m_state.practice_log) {
        if (pe.person_id == p.id && pe.timestamp >= today_start) {
            practiced_today = true; break;
        }
    }
    m["practicedToday"] = practiced_today;

    /* Skills */
    QVariantList skills;
    for (const auto &s : p.skills) {
        skills.append(skillToVariant(s));
    }
    m["skills"] = skills;
    return m;
}

QVariantList Backend::persons() const {
    QVariantList list;
    for (const auto &p : m_state.persons) {
        list.append(personToVariant(p, m_state.persons));
    }
    return list;
}

/* --- Timer --- */

void Backend::startTimer(const QString &personId, const QString &skillName, int plannedMin) {
    /* Auto-stop previous timer if running */
    if (m_timerRunning) {
        stopTimer();
    }

    Person *p = find_person(m_state.persons, personId.toStdString());
    if (!p) return;
    if (!find_skill(*p, skillName.toStdString())) return;

    m_timerRunning = true;
    m_timerPersonId = personId;
    m_timerSkillName = skillName;
    m_timerStart = QDateTime::currentDateTime();
    m_timerPlannedMin = plannedMin;
    m_tickTimer.start(1000);
    emit timerChanged();
}

int Backend::stopTimer() {
    if (!m_timerRunning) return -1;
    m_tickTimer.stop();
    m_timerRunning = false;

    int elapsedSec = m_timerStart.secsTo(QDateTime::currentDateTime());
    int minutes = elapsedSec / 60;
    if (minutes < 1) minutes = 1;

    /* Find person+skill for practice */
    Person *p = find_person(m_state.persons, m_timerPersonId.toStdString());
    if (!p) { emit timerChanged(); return minutes; }

    Skill *s = find_skill(*p, m_timerSkillName.toStdString());
    if (!s) { emit timerChanged(); return minutes; }

    int old_level = s->level;
    int base_xp = minutes_to_xp(minutes);
    int bonus_xp = (int)(base_xp * (s->bonus - 1.0f));
    int total_xp = base_xp + bonus_xp;

    /* Apply entropy/sync multiplier */
    float mult = xp_multiplier(m_state.master);

    /* Apply XP boost from shop */
    if (m_state.master.xp_boost) {
        mult *= 1.5f;
        m_state.master.xp_boost = false;
        emit boostChanged();
    }

    total_xp = (int)(total_xp * mult);

    s->xp += total_xp;
    s->level = xp_to_level(s->xp);
    s->last_practice = time(nullptr);

    int old_person_level = p->level;
    p->level = calc_person_level(p->skills);
    p->title = title_for_level(p->level);

    /* Log practice (effect to be set later) */
    log_practice(m_state, m_timerPersonId.toStdString(),
                 m_timerSkillName.toStdString(), minutes, EFFECT_NONE);

    /* Challenges */
    int completed = update_challenges(m_state.master, m_timerPersonId.toStdString(),
                      m_timerSkillName.toStdString());

    /* Award coins for completed challenges */
    for (int i = 0; i < completed; i++) {
        for (auto &c : m_state.master.challenges) {
            if (c.completed && c.current_count >= c.target_count) {
                int reward = c.coin_reward;
                if (m_state.master.coins_double) {
                    reward *= 2;
                    m_state.master.coins_double = false;
                }
                m_state.master.coins += reward;
                emit coinsEarned(reward);
                break;
            }
        }
    }
    if (completed > 0) emit coinsChanged();

    /* Achievements */
    auto new_ach = check_achievements(m_state.persons, m_state.master, m_state.practice_log);
    for (auto &a : new_ach) {
        m_state.master.achievements.push_back(a);
    }

    /* Cross-bonuses */
    auto new_bonuses = check_cross_bonuses(m_timerPersonId.toStdString(),
                                            m_timerSkillName.toStdString(),
                                            old_level, s->level);
    for (const auto &cb : new_bonuses) {
        apply_cross_bonus(m_state.persons, cb);
    }

    emit timerChanged();
    emit personsChanged();
    emit statsChanged();
    emit historyChanged();

    /* Tell QML to show effect picker */
    Person *pp = find_person(m_state.persons, m_timerPersonId.toStdString());
    if (pp) {
        emit effectPrompt(QString::fromStdString(pp->name),
                          m_timerSkillName, minutes);
    }

    return minutes;
}

void Backend::cancelTimer() {
    if (!m_timerRunning) return;
    m_tickTimer.stop();
    m_timerRunning = false;
    emit timerChanged();
}

void Backend::logEffect(const QString &effect) {
    if (m_state.practice_log.empty()) return;
    PracticeEntry &pe = m_state.practice_log.back();
    if (effect == "energized") pe.effect = EFFECT_ENERGIZED;
    else if (effect == "tired") pe.effect = EFFECT_TIRED;
    else if (effect == "drained") pe.effect = EFFECT_DRAINED;
    else pe.effect = EFFECT_NEUTRAL;
    saveState();
    emit statsChanged();
}

void Backend::logEffectNote(const QString &effect, const QString &note) {
    if (m_state.practice_log.empty()) return;
    PracticeEntry &pe = m_state.practice_log.back();
    if (effect == "energized") pe.effect = EFFECT_ENERGIZED;
    else if (effect == "tired") pe.effect = EFFECT_TIRED;
    else if (effect == "drained") pe.effect = EFFECT_DRAINED;
    else pe.effect = EFFECT_NEUTRAL;
    pe.notes = note.toStdString();
    saveState();
    emit statsChanged();
}

void Backend::redoEffect(const QString &effect) {
    logEffect(effect); /* same thing */
}

bool Backend::timerRunning() const { return m_timerRunning; }
int Backend::timerElapsedSec() const {
    if (!m_timerRunning) return 0;
    return m_timerStart.secsTo(QDateTime::currentDateTime());
}
QString Backend::timerPersonId() const { return m_timerPersonId; }
QString Backend::timerSkillName() const { return m_timerSkillName; }
int Backend::timerPlannedMin() const { return m_timerPlannedMin; }

void Backend::tick() {
    emit timerChanged();
}

/* --- Stats --- */

QVariantList Backend::skillEffectStats() const {
    QVariantList list;
    auto stats = ::skill_effect_stats(m_state.practice_log, m_state.persons);
    for (const auto &ss : stats) {
        QVariantMap m;
        m["personName"] = QString::fromStdString(ss.person_name);
        m["skillName"] = QString::fromStdString(ss.skill_name);
        m["energized"] = ss.counts[0];
        m["neutral"] = ss.counts[1];
        m["tired"] = ss.counts[2];
        m["drained"] = ss.counts[3];
        list.append(m);
    }
    return list;
}

QVariantList Backend::timeEffectStats() const {
    QVariantList list;
    auto stats = ::time_effect_stats(m_state.practice_log);
    for (const auto &ts : stats) {
        QVariantMap m;
        m["bucket"] = ts.hour_bucket;
        m["total"] = ts.total;
        m["energized"] = ts.counts[0];
        m["neutral"] = ts.counts[1];
        m["tired"] = ts.counts[2];
        m["drained"] = ts.counts[3];
        list.append(m);
    }
    return list;
}

QVariantList Backend::activityChains() const {
    QVariantList list;
    auto chains = ::activity_chains(m_state.practice_log, m_state.persons);
    for (const auto &ac : chains) {
        QVariantMap m;
        m["personId"] = QString::fromStdString(ac.person_id);
        m["beforeSkill"] = QString::fromStdString(ac.before_skill);
        m["afterSkill"] = QString::fromStdString(ac.after_skill);
        m["energizedPct"] = ac.energized_pct;
        m["sampleCount"] = ac.sample_count;
        list.append(m);
    }
    return list;
}

/* --- Daily Todos --- */

QVariantList Backend::dailyTodos() const {
    QVariantList list;
    for (const auto &dt : m_state.master.daily_todos) {
        QVariantMap m;
        m["text"] = QString::fromStdString(dt.text);
        m["done"] = dt.done;
        list.append(m);
    }
    return list;
}

void Backend::dailyTodoToggle(int index) {
    if (index < 0 || index >= (int)m_state.master.daily_todos.size()) return;
    m_state.master.daily_todos[index].done = !m_state.master.daily_todos[index].done;
    saveState();
    emit dailyTodosChanged();
}

void Backend::dailyTodoAdd(const QString &text) {
    DailyTodo dt;
    dt.text = text.toStdString();
    dt.done = false;
    m_state.master.daily_todos.push_back(dt);
    saveState();
    emit dailyTodosChanged();
}

void Backend::dailyTodoDelete(int index) {
    if (index < 0 || index >= (int)m_state.master.daily_todos.size()) return;
    m_state.master.daily_todos.erase(m_state.master.daily_todos.begin() + index);
    saveState();
    emit dailyTodosChanged();
}

/* --- Calendar --- */

QVariantList Backend::calendarSlots() const {
    QVariantList list;
    for (int i = 0; i < (int)m_state.master.calendar.size(); i++) {
        const auto &cs = m_state.master.calendar[i];
        QVariantMap m;
        m["index"] = i;
        m["dayOfWeek"] = cs.day_of_week;
        m["hour"] = cs.hour;
        m["minute"] = cs.minute;
        m["label"] = QString::fromStdString(cs.label);
        m["date"] = QString::fromStdString(cs.date);
        QString desc;
        QString qdate = QString::fromStdString(cs.date);
        if (!cs.date.empty()) {
            desc = qdate + " " + QString::number(cs.hour).rightJustified(2,'0') + ":" + QString::number(cs.minute).rightJustified(2,'0') + " " + QString::fromStdString(cs.label);
        } else {
            const char* days[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
            desc = QString(days[cs.day_of_week]) + " " + QString::number(cs.hour).rightJustified(2,'0') + ":" + QString::number(cs.minute).rightJustified(2,'0') + " " + QString::fromStdString(cs.label);
        }
        m["desc"] = desc;
        list.append(m);
    }
    return list;
}

void Backend::calendarAdd(int dayOfWeek, int hour, int minute, const QString &label) {
    CalendarSlot cs;
    cs.day_of_week = dayOfWeek;
    cs.hour = hour;
    cs.minute = minute;
    cs.person_id = "master";
    cs.label = label.toStdString();
    m_state.master.calendar.push_back(cs);
    saveState();
    emit calendarSlotsChanged();
}

void Backend::calendarAddDate(int day, int month, int hour, int minute, const QString &label) {
    CalendarSlot cs;
    cs.day_of_week = 0;
    cs.hour = hour;
    cs.minute = minute;
    cs.person_id = "master";
    cs.label = label.toStdString();
    char buf[8];
    snprintf(buf, sizeof(buf), "%02d.%02d", day, month);
    cs.date = buf;
    m_state.master.calendar.push_back(cs);
    saveState();
    emit calendarSlotsChanged();
}

void Backend::calendarDelete(int index) {
    if (index < 0 || index >= (int)m_state.master.calendar.size()) return;
    m_state.master.calendar.erase(m_state.master.calendar.begin() + index);
    saveState();
    emit calendarSlotsChanged();
}

void Backend::calendarEdit(int index, int dayOfWeek, int hour, int minute, const QString &label, const QString &date) {
    if (index < 0 || index >= (int)m_state.master.calendar.size()) return;
    auto &cs = m_state.master.calendar[index];
    cs.day_of_week = dayOfWeek;
    cs.hour = hour;
    cs.minute = minute;
    cs.label = label.toStdString();
    cs.date = date.toStdString();
    saveState();
    emit calendarSlotsChanged();
}

int Backend::coins() const { return m_state.master.coins; }

/* --- Today Summary --- */

QVariantMap Backend::todaySummary() const {
    QVariantMap m;
    time_t now = time(nullptr);
    struct tm tm;
    localtime_r(&now, &tm);
    int dow = tm.tm_wday;

    /* Calendar slots for today */
    QVariantList todaySlots;
    for (const auto &cs : m_state.master.calendar) {
        if (cs.day_of_week == dow) {
            QVariantMap s;
            s["hour"] = cs.hour;
            s["minute"] = cs.minute;
            s["label"] = QString::fromStdString(cs.label);
            todaySlots.append(s);
        }
    }
    m["slots"] = todaySlots;

    /* Open tasks count */
    int openTasks = 0;
    for (const auto &t : m_state.master.tasks) {
        if (!t.done) openTasks++;
    }
    m["openTasks"] = openTasks;

    /* Most neglected character (skip if never practiced — they're new, not neglected) */
    const Person *neglected = most_neglected(m_state.persons);
    if (neglected) {
        time_t latest = 0;
        for (const auto &s : neglected->skills) {
            if (s.last_practice > latest) latest = s.last_practice;
        }
        if (latest > 0) {
            m["neglectedName"] = QString::fromStdString(neglected->name);
            m["neglectedSkill"] = QString::fromStdString(default_skill(neglected->id));
            m["neglectedDays"] = days_since(latest);
        }
    }

    /* This week total */
    auto pie = ::pie_data(m_state.persons, m_state.practice_log, 7);
    m["pie"] = QVariant::fromValue(pie.size()); /* simplified for now */

    return m;
}

/* --- Skill Management --- */

void Backend::skillAdd(const QString &personId, const QString &skillName, bool isMain) {
    Person *p = find_person(m_state.persons, personId.toStdString());
    if (!p) return;
    if (find_skill(*p, skillName.toStdString())) return;

    Skill s;
    s.name = skillName.toStdString();
    s.xp = 0;
    s.level = 0;
    s.last_practice = 0;
    s.is_main = isMain;
    s.bonus = 1.0f;
    s.archived = false;
    p->skills.push_back(s);
    p->level = calc_person_level(p->skills);
    p->title = title_for_level(p->level);
    saveState();
    emit personsChanged();
}

void Backend::skillArchive(const QString &personId, const QString &skillName) {
    Person *p = find_person(m_state.persons, personId.toStdString());
    if (!p) return;
    if (!archive_skill(*p, skillName.toStdString())) return;
    saveState();
    emit personsChanged();
}

void Backend::skillReactivate(const QString &personId, const QString &skillName) {
    Person *p = find_person(m_state.persons, personId.toStdString());
    if (!p) return;
    if (!reactivate_skill(*p, skillName.toStdString())) return;
    saveState();
    emit personsChanged();
}

void Backend::skillDelete(const QString &personId, const QString &skillName) {
    Person *p = find_person(m_state.persons, personId.toStdString());
    if (!p) return;
    if (!delete_skill(*p, skillName.toStdString())) return;
    saveState();
    emit personsChanged();
}

void Backend::skillRename(const QString &personId, const QString &oldName, const QString &newName) {
    Person *p = find_person(m_state.persons, personId.toStdString());
    if (!p) return;
    if (!rename_skill(*p, oldName.toStdString(), newName.toStdString())) return;
    saveState();
    emit personsChanged();
}

/* --- Task Management --- */

QVariantList Backend::tasks() const {
    QVariantList list;
    for (const auto &t : m_state.master.tasks) {
        QVariantMap m;
        m["id"] = t.id;
        m["text"] = QString::fromStdString(t.text);
        m["done"] = t.done;
        list.append(m);
    }
    return list;
}

void Backend::taskAdd(const QString &text) {
    Task t;
    t.id = m_state.master.next_task_id++;
    t.text = text.toStdString();
    t.done = false;
    t.created = time(nullptr);
    m_state.master.tasks.push_back(t);
    saveState();
    emit tasksChanged();
}

void Backend::taskToggle(int id) {
    for (auto &t : m_state.master.tasks) {
        if (t.id == id) { t.done = !t.done; break; }
    }
    saveState();
    emit tasksChanged();
}

void Backend::taskDelete(int id) {
    auto &tasks = m_state.master.tasks;
    tasks.erase(std::remove_if(tasks.begin(), tasks.end(),
                [id](const Task &t) { return t.id == id; }), tasks.end());
    saveState();
    emit tasksChanged();
}

void Backend::taskRename(int id, const QString &newText) {
    for (auto &t : m_state.master.tasks) {
        if (t.id == id) { t.text = newText.toStdString(); break; }
    }
    saveState();
    emit tasksChanged();
}

/* --- Challenge Management --- */

QVariantList Backend::challenges() const {
    QVariantList list;
    for (const auto &c : m_state.master.challenges) {
        QVariantMap m;
        m["description"] = QString::fromStdString(c.description);
        m["personId"] = QString::fromStdString(c.person_id);
        m["skillName"] = QString::fromStdString(c.skill_name);
        m["targetCount"] = c.target_count;
        m["targetDays"] = c.target_days;
        m["coinReward"] = c.coin_reward;
        m["currentCount"] = c.current_count;
        m["completed"] = c.completed;
        list.append(m);
    }
    return list;
}

int Backend::challengeAdd(const QString &description, const QString &personId,
                           const QString &skillName, int targetCount,
                           int targetDays, int coinReward) {
    Challenge c;
    c.id = "chal_" + std::to_string(m_state.master.challenges.size() + 1);
    c.description = description.toStdString();
    c.person_id = personId.toStdString();
    c.skill_name = skillName.toStdString();
    c.target_count = targetCount;
    c.target_days = targetDays;
    c.coin_reward = coinReward;
    c.current_count = 0;
    c.completed = false;
    c.started = time(nullptr);
    m_state.master.challenges.push_back(c);
    saveState();
    emit challengesChanged();
    return (int)m_state.master.challenges.size() - 1;
}

void Backend::challengeDelete(int index) {
    if (index < 0 || index >= (int)m_state.master.challenges.size()) return;
    m_state.master.challenges.erase(m_state.master.challenges.begin() + index);
    saveState();
    emit challengesChanged();
}

/* --- Mood --- */

void Backend::moodLog(const QString &word) {
    MoodEntry me;
    me.word = word.toStdString();
    me.timestamp = time(nullptr);
    m_state.master.mood_log.push_back(me);
    saveState();
    emit moodChanged();
}

bool Backend::xpBoostActive() const { return m_state.master.xp_boost; }
bool Backend::coinsDoubleActive() const { return m_state.master.coins_double; }

int Backend::totalPracticeXP() const {
    int total = 0;
    for (const auto &p : m_state.persons) {
        for (const auto &s : p.skills) total += s.xp;
    }
    return total;
}

int Backend::totalSessions() const {
    return (int)m_state.practice_log.size();
}

bool Backend::onboardingSeen() const { return m_state.master.onboarding_seen; }
void Backend::markOnboardingSeen() {
    m_state.master.onboarding_seen = true;
    saveState();
    emit boostChanged();
}

int Backend::theme() const { return m_state.master.theme; }
void Backend::setTheme(int t) {
    if (t != m_state.master.theme) {
        m_state.master.theme = t;
        saveState();
        emit boostChanged();
    }
}

QVariantList Backend::moodHistory() const {
    QVariantList list;
    int count = 0;
    for (auto it = m_state.master.mood_log.rbegin();
         it != m_state.master.mood_log.rend() && count < 30; ++it, ++count) {
        QVariantMap m;
        m["word"] = QString::fromStdString(it->word);
        char dbuf[32];
        struct tm tm;
        time_t t = it->timestamp;
        localtime_r(&t, &tm);
        strftime(dbuf, sizeof(dbuf), "%d %b %H:%M", &tm);
        m["dateStr"] = QString(dbuf);
        list.append(m);
    }
    return list;
}

/* --- Diary --- */

QVariantList Backend::diaryLog() const {
    QVariantList list;
    int count = 0;
    for (auto it = m_state.master.diary_log.rbegin();
         it != m_state.master.diary_log.rend() && count < 20; ++it, ++count) {
        QVariantMap m;
        m["text"] = QString::fromStdString(it->text);
        char dbuf[32];
        struct tm tm;
        time_t t = it->timestamp;
        localtime_r(&t, &tm);
        strftime(dbuf, sizeof(dbuf), "%d %b %H:%M", &tm);
        m["dateStr"] = QString(dbuf);
        list.append(m);
    }
    return list;
}

void Backend::diaryAdd(const QString &text) {
    DiaryEntry de;
    de.text = text.toStdString();
    de.timestamp = time(nullptr);
    m_state.master.diary_log.push_back(de);
    saveState();
    emit diaryChanged();
}

QVariantList Backend::allDiaryEntries() const {
    QVariantList list;
    for (int i = (int)m_state.master.diary_log.size() - 1; i >= 0; i--) {
        const auto& de = m_state.master.diary_log[i];
        QVariantMap m;
        m["index"] = i;
        m["text"] = QString::fromStdString(de.text);
        char dbuf[32];
        struct tm tm;
        time_t t = de.timestamp;
        localtime_r(&t, &tm);
        strftime(dbuf, sizeof(dbuf), "%d %b %Y %H:%M", &tm);
        m["dateStr"] = QString(dbuf);
        // Group key for section headers (date only)
        char dateKey[16];
        strftime(dateKey, sizeof(dateKey), "%d %b %Y", &tm);
        m["dateKey"] = QString(dateKey);
        // Comments
        QVariantList comments;
        for (const auto& dc : de.comments) {
            QVariantMap cm;
            cm["text"] = QString::fromStdString(dc.text);
            time_t ct = dc.timestamp;
            struct tm ctm;
            localtime_r(&ct, &ctm);
            strftime(dbuf, sizeof(dbuf), "%H:%M", &ctm);
            cm["timeStr"] = QString(dbuf);
            comments.append(cm);
        }
        m["comments"] = comments;
        list.append(m);
    }
    return list;
}

void Backend::diaryAddComment(int index, const QString &text) {
    if (index < 0 || index >= (int)m_state.master.diary_log.size()) return;
    DiaryComment dc;
    dc.text = text.toStdString();
    dc.timestamp = time(nullptr);
    m_state.master.diary_log[index].comments.push_back(dc);
    saveState();
    emit diaryChanged();
}

QVariantList Backend::monthlyHeatmap() const {
    time_t now = time(nullptr);
    struct tm tm;
    localtime_r(&now, &tm);
    return monthlyHeatmapFor(tm.tm_year + 1900, tm.tm_mon + 1);
}

QVariantList Backend::monthlyHeatmapFor(int year, int month) const {
    QVariantList list;

    /* Days in month */
    int dim = 31;
    if (month == 2) {
        dim = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) ? 29 : 28;
    } else if (month == 4 || month == 6 || month == 9 || month == 11) {
        dim = 30;
    }

    /* Count minutes per day */
    int dayMins[32] = {};
    for (const auto &pe : m_state.practice_log) {
        struct tm petm;
        time_t t = pe.timestamp;
        localtime_r(&t, &petm);
        if (petm.tm_year + 1900 == year && petm.tm_mon + 1 == month) {
            dayMins[petm.tm_mday] += pe.minutes;
        }
    }

    /* Find diary entries per day */
    int dayDiaryIdx[32] = {};
    for (int i = 0; i < 32; i++) dayDiaryIdx[i] = -1;
    for (int i = 0; i < (int)m_state.master.diary_log.size(); i++) {
        struct tm dtm;
        time_t t = m_state.master.diary_log[i].timestamp;
        localtime_r(&t, &dtm);
        if (dtm.tm_year + 1900 == year && dtm.tm_mon + 1 == month) {
            if (dayDiaryIdx[dtm.tm_mday] == -1) dayDiaryIdx[dtm.tm_mday] = i;
        }
    }

    /* Find schedule entries per day */
    bool dayHasSchedule[32] = {};
    for (const auto &cs : m_state.master.calendar) {
        if (!cs.date.empty()) {
            int d = 0, m = 0;
            if (sscanf(cs.date.c_str(), "%d.%d", &d, &m) == 2 && m == month && d >= 1 && d <= dim) {
                dayHasSchedule[d] = true;
            }
        }
        // Recurring day-of-week: check each day of month
        if (cs.date.empty()) {
            struct tm first = {};
            first.tm_year = year - 1900;
            first.tm_mon = month - 1;
            first.tm_mday = 1;
            mktime(&first);
            for (int d = 1; d <= dim; d++) {
                if ((first.tm_wday + d - 1) % 7 == cs.day_of_week) {
                    dayHasSchedule[d] = true;
                }
            }
        }
    }

    /* Find first weekday */
    struct tm first_tm = {};
    first_tm.tm_year = year - 1900;
    first_tm.tm_mon = month - 1;
    first_tm.tm_mday = 1;
    mktime(&first_tm);
    int firstDow = first_tm.tm_wday;

    time_t now = time(nullptr);
    struct tm now_tm;
    localtime_r(&now, &now_tm);
    bool isCurrentMonth = (now_tm.tm_year + 1900 == year && now_tm.tm_mon + 1 == month);

    for (int d = 1; d <= dim; d++) {
        QVariantMap m;
        m["day"] = d;
        m["dow"] = (firstDow + d - 1) % 7;
        int mins = dayMins[d];
        if (mins == 0) m["level"] = 0;
        else if (mins < 30) m["level"] = 1;
        else if (mins < 90) m["level"] = 2;
        else m["level"] = 3;
        m["minutes"] = mins;
        int di = dayDiaryIdx[d];
        m["hasDiary"] = (di >= 0);
        if (di >= 0) {
            m["diaryText"] = QString::fromStdString(m_state.master.diary_log[di].text);
        }
        m["hasSchedule"] = dayHasSchedule[d];
        m["isToday"] = (isCurrentMonth && d == now_tm.tm_mday);
        list.append(m);
    }
    return list;
}

/* --- Shop --- */

QVariantList Backend::shopItems() const {
    QVariantList items;

    auto add = [&](const char *id, const char *name, const char *desc, int cost) {
        QVariantMap m;
        m["id"] = QString(id);
        m["name"] = QString(name);
        m["description"] = QString(desc);
        m["cost"] = cost;
        items.append(m);
    };

    add("entropy_reset", "Entropy Reset", "Reset team entropy to 0", 50);
    add("xp_boost", "XP Boost", "+50% XP for next practice", 20);
    add("skip_day", "Skip Day", "No entropy penalty today", 30);
    add("custom_title", "Custom Title", "Set a custom title for one character", 100);
    add("coins_double", "Coin Doubler", "×2 coins from next completed challenge", 40);
    return items;
}

bool Backend::buyItem(const QString &itemId, const QString &personId) {
    /* Find item cost */
    auto items = shopItems();
    int cost = -1;
    for (const auto &item : items) {
        if (item.toMap()["id"].toString() == itemId) {
            cost = item.toMap()["cost"].toInt();
            break;
        }
    }
    if (cost < 0) return false;
    if (m_state.master.coins < cost) return false;

    m_state.master.coins -= cost;

    if (itemId == "entropy_reset") {
        m_state.master.entropy = 0;
        emit entropyChanged();
    } else if (itemId == "xp_boost") {
        m_state.master.xp_boost = true;
        emit boostChanged();
    } else if (itemId == "skip_day") {
        m_state.master.entropy = m_state.master.entropy > 0 ? m_state.master.entropy - 5 : 0;
        m_state.master.entropy_updated = time(nullptr); /* mark today as updated */
        emit entropyChanged();
    } else if (itemId == "custom_title") {
        /* Custom title handled via buyCustomTitle — skip here */
    } else if (itemId == "coins_double") {
        m_state.master.coins_double = true;
    }

    saveState();
    emit coinsChanged();
    return true;
}

bool Backend::buyCustomTitle(const QString &personId, const QString &title) {
    if (m_state.master.coins < 100) return false;
    Person *p = find_person(m_state.persons, personId.toStdString());
    if (!p) return false;
    m_state.master.coins -= 100;
    p->title = title.toStdString();
    saveState();
    emit personsChanged();
    emit coinsChanged();
    return true;
}

QString Backend::exportCsv() {
    std::string path = m_statePath.substr(0, m_statePath.rfind('/')) + "/practice_log.csv";
    std::ofstream f(path);
    f << "date,time,person,skill,minutes,effect\n";
    for (const auto &pe : m_state.practice_log) {
        char dbuf[32], tbuf[32];
        struct tm tm;
        time_t t = pe.timestamp;
        localtime_r(&t, &tm);
        strftime(dbuf, sizeof(dbuf), "%Y-%m-%d", &tm);
        strftime(tbuf, sizeof(tbuf), "%H:%M:%S", &tm);
        const char *eff = "neutral";
        switch (pe.effect) {
            case EFFECT_ENERGIZED: eff = "energized"; break;
            case EFFECT_TIRED: eff = "tired"; break;
            case EFFECT_DRAINED: eff = "drained"; break;
            default: break;
        }
        f << dbuf << "," << tbuf << "," << pe.person_id << ","
          << pe.skill_name << "," << pe.minutes << ","
          << eff << "\n";
    }
    f.close();
    return QString::fromStdString(path);
}

/* --- Weekly Pie --- */

QVariantList Backend::weeklyPie() const {
    QVariantList list;
    auto slices = ::pie_data(m_state.persons, m_state.practice_log, 7);
    int total = 0;
    for (const auto &s : slices) total += s.minutes;
    const char *colors[] = {"#EF5350","#42A5F5","#66BB6A","#AB47BC"};

    for (size_t i = 0; i < slices.size(); i++) {
        QVariantMap m;
        m["personName"] = QString::fromStdString(slices[i].person_name);
        m["minutes"] = slices[i].minutes;
        m["percent"] = total > 0 ? (slices[i].minutes * 100.0f / total) : 0.0f;
        m["color"] = QString(colors[i % 4]);
        list.append(m);
    }
    return list;
}

/* --- Birthdays --- */

QVariantList Backend::birthdays() const {
    QVariantList list;
    time_t now = time(nullptr);
    struct tm tm_now;
    localtime_r(&now, &tm_now);
    int todayMonth = tm_now.tm_mon + 1;
    int todayDay = tm_now.tm_mday;

    for (const auto &b : m_state.master.birthdays) {
        QVariantMap m;
        m["name"] = QString::fromStdString(b.name);
        m["date"] = QString::fromStdString(b.date);
        int bm, bd;
        sscanf(b.date.c_str(), "%d-%d", &bm, &bd);
        int da = (bm - todayMonth) * 30 + (bd - todayDay);
        if (da < 0) da += 365;
        m["daysAway"] = da;
        m["isToday"] = (da == 0);
        list.append(m);
    }
    std::sort(list.begin(), list.end(), [](const QVariant &a, const QVariant &b) {
        return a.toMap()["daysAway"].toInt() < b.toMap()["daysAway"].toInt();
    });
    return list;
}

void Backend::birthdayAdd(const QString &name, const QString &date) {
    Birthday b;
    b.name = name.toStdString();
    b.date = date.toStdString();
    m_state.master.birthdays.push_back(b);
    saveState();
    emit birthdaysChanged();
}

void Backend::birthdayDelete(int index) {
    if (index < 0 || index >= (int)m_state.master.birthdays.size()) return;
    m_state.master.birthdays.erase(m_state.master.birthdays.begin() + index);
    saveState();
    emit birthdaysChanged();
}

QVariantList Backend::archivedSkills(const QString &personId) {
    QVariantList list;
    for (const auto &p : m_state.persons) {
        if (p.id != personId.toStdString()) continue;
        for (const auto &s : p.archived_skills) {
            QVariantMap m;
            m["name"] = QString::fromStdString(s.name);
            m["xp"] = s.xp;
            m["level"] = s.level;
            list.append(m);
        }
        break;
    }
    return list;
}

int Backend::entropy() const { return m_state.master.entropy; }
int Backend::teamSync() const { return m_state.master.team_sync; }

QVariantList Backend::practiceHistory() const {
    QVariantList list;
    int count = 0;
    for (auto it = m_state.practice_log.rbegin();
         it != m_state.practice_log.rend() && count < 50; ++it, ++count) {
        QVariantMap m;
        m["personId"] = QString::fromStdString(it->person_id);
        m["skillName"] = QString::fromStdString(it->skill_name);
        m["minutes"] = it->minutes;
        m["timestamp"] = (qlonglong)it->timestamp;
        const char *eff = "neutral";
        if (it->effect == EFFECT_ENERGIZED) eff = "energized";
        else if (it->effect == EFFECT_TIRED) eff = "tired";
        else if (it->effect == EFFECT_DRAINED) eff = "drained";
        m["effect"] = QString(eff);
        m["notes"] = QString::fromStdString(it->notes);

        /* Date string */
        char dbuf[32];
        struct tm tm;
        time_t t = it->timestamp;
        localtime_r(&t, &tm);
        strftime(dbuf, sizeof(dbuf), "%d %b %H:%M", &tm);
        m["dateStr"] = QString(dbuf);
        list.append(m);
    }
    return list;
}
