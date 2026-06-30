#ifndef TOMOGICHI_BACKEND_H
#define TOMOGICHI_BACKEND_H

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QString>
#include <QTimer>
#include <QDateTime>
#include <memory>

#include "core/types.h"
#include "data/storage.h"

class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList persons READ persons NOTIFY personsChanged)
    Q_PROPERTY(QVariantList skillEffectStats READ skillEffectStats NOTIFY statsChanged)
    Q_PROPERTY(QVariantList timeEffectStats READ timeEffectStats NOTIFY statsChanged)
    Q_PROPERTY(QVariantList activityChains READ activityChains NOTIFY statsChanged)
    Q_PROPERTY(bool timerRunning READ timerRunning NOTIFY timerChanged)
    Q_PROPERTY(int timerElapsedSec READ timerElapsedSec NOTIFY timerChanged)
    Q_PROPERTY(QString timerPersonId READ timerPersonId NOTIFY timerChanged)
    Q_PROPERTY(QString timerSkillName READ timerSkillName NOTIFY timerChanged)
    Q_PROPERTY(int timerPlannedMin READ timerPlannedMin NOTIFY timerChanged)
    Q_PROPERTY(QVariantList dailyTodos READ dailyTodos NOTIFY dailyTodosChanged)
    Q_PROPERTY(QVariantList calendarSlots READ calendarSlots NOTIFY calendarSlotsChanged)
    Q_PROPERTY(int coins READ coins NOTIFY coinsChanged)
    Q_PROPERTY(QVariantList tasks READ tasks NOTIFY tasksChanged)
    Q_PROPERTY(QVariantList challenges READ challenges NOTIFY challengesChanged)
    Q_PROPERTY(int entropy READ entropy NOTIFY entropyChanged)
    Q_PROPERTY(int teamSync READ teamSync NOTIFY entropyChanged)
    Q_PROPERTY(QVariantList practiceHistory READ practiceHistory NOTIFY historyChanged)
    Q_PROPERTY(QVariantList diaryLog READ diaryLog NOTIFY diaryChanged)
    Q_PROPERTY(QVariantList allDiaryEntries READ allDiaryEntries NOTIFY diaryChanged)
    Q_PROPERTY(QVariantList monthlyHeatmap READ monthlyHeatmap NOTIFY historyChanged)
    Q_PROPERTY(QVariantList shopItems READ shopItems CONSTANT)
    Q_PROPERTY(QVariantList birthdays READ birthdays NOTIFY birthdaysChanged)
    Q_PROPERTY(QVariantList weeklyPie READ weeklyPie NOTIFY historyChanged)
    Q_PROPERTY(bool xpBoostActive READ xpBoostActive NOTIFY boostChanged)
    Q_PROPERTY(bool coinsDoubleActive READ coinsDoubleActive NOTIFY boostChanged)
    Q_PROPERTY(int totalPracticeXP READ totalPracticeXP NOTIFY historyChanged)
    Q_PROPERTY(int totalSessions READ totalSessions NOTIFY historyChanged)
    Q_PROPERTY(bool onboardingSeen READ onboardingSeen NOTIFY boostChanged)
    Q_PROPERTY(int theme READ theme NOTIFY boostChanged)
    Q_PROPERTY(QString wallpaper READ wallpaper NOTIFY boostChanged)
    Q_PROPERTY(QVariantList moodHistory READ moodHistory NOTIFY moodChanged)
    Q_PROPERTY(QVariantList charChecklists READ charChecklists NOTIFY charChecklistsChanged)

public:
    explicit Backend(QObject *parent = nullptr);
    ~Backend();

    /* Persons for QML */
    QVariantList persons() const;

    /* Effect stats for QML */
    QVariantList skillEffectStats() const;
    QVariantList timeEffectStats() const;
    QVariantList activityChains() const;

    /* Timer state */
    bool timerRunning() const;
    int timerElapsedSec() const;
    QString timerPersonId() const;
    QString timerSkillName() const;
    int timerPlannedMin() const;

    /* Daily todos & calendar */
    QVariantList dailyTodos() const;
    QVariantList calendarSlots() const;
    int coins() const;
    QVariantList tasks() const;
    QVariantList challenges() const;
    int entropy() const;
    int teamSync() const;
    QVariantList practiceHistory() const;
    QVariantList diaryLog() const;
    QVariantList allDiaryEntries() const;
    Q_INVOKABLE QVariantList monthlyHeatmap() const;
    Q_INVOKABLE QVariantList monthlyHeatmapFor(int year, int month) const;
    QVariantList shopItems() const;
    QVariantList birthdays() const;
    QVariantList weeklyPie() const;
    QVariantList moodHistory() const;
    bool xpBoostActive() const;
    bool coinsDoubleActive() const;
    int totalPracticeXP() const;
    int totalSessions() const;
    bool onboardingSeen() const;
    int theme() const;
    Q_INVOKABLE void setTheme(int t);
    QString wallpaper() const;
    Q_INVOKABLE void setWallpaper(const QString &path);
    Q_INVOKABLE void clearWallpaper();
    Q_INVOKABLE void markOnboardingSeen();

    /* Compute today's summary */
    Q_INVOKABLE QVariantMap todaySummary() const;

public slots:
    /* Load state on startup */
    bool loadState(const QString &path = "data/state.json");

    /* Start timer for a person+skill with planned minutes */
    void startTimer(const QString &personId, const QString &skillName, int plannedMin);

    /* Stop timer and log practice. Returns minutes elapsed. */
    int stopTimer();

    /* Cancel timer without logging */
    void cancelTimer();

    /* Log effect for the last practice (called after stopTimer) */
    void logEffect(const QString &effect); /* "energized","neutral","tired","drained" */
    void logEffectNote(const QString &effect, const QString &note); /* with notes */

    /* Daily todo actions */
    void dailyTodoToggle(int index);
    void dailyTodoAdd(const QString &text);
    void dailyTodoDelete(int index);

    /* Calendar actions */
    void calendarAdd(int dayOfWeek, int hour, int minute, const QString &label);
    void calendarAddDate(int day, int month, int hour, int minute, const QString &label);
    void calendarDelete(int index);
    void calendarEdit(int index, int dayOfWeek, int hour, int minute, const QString &label, const QString &date);

    /* Save state */
    bool saveState();

    /* Skill management */
    void skillAdd(const QString &personId, const QString &skillName, bool isMain);
    void skillArchive(const QString &personId, const QString &skillName);
    void skillReactivate(const QString &personId, const QString &skillName);
    void skillDelete(const QString &personId, const QString &skillName);
    void skillRename(const QString &personId, const QString &oldName, const QString &newName);

    /* Task management */
    void taskAdd(const QString &text);
    void taskToggle(int id);
    void taskDelete(int id);
    void taskRename(int id, const QString &newText);

    /* Challenge management */
    int challengeAdd(const QString &description, const QString &personId,
                     const QString &skillName, int targetCount, int targetDays, int coinReward);
    void challengeDelete(int index);

    /* Mood */
    void moodLog(const QString &word);

    /* Re-log effect for last practice */
    Q_INVOKABLE void redoEffect(const QString &effect);

    /* Diary */
    void diaryAdd(const QString &text);
    void diaryAddComment(int index, const QString &text);

    /* Character checklists */
    QVariantList charChecklists() const;

    /* Character rename */
    Q_INVOKABLE void renamePerson(const QString &personId, const QString &newName);

    /* Character checklist actions */
    Q_INVOKABLE void charChecklistAdd(const QString &personId, const QString &text);
    Q_INVOKABLE void charChecklistToggle(int index);
    Q_INVOKABLE void charChecklistDelete(int index);
    Q_INVOKABLE void charChecklistRepeat(int index, int hours);

    /* Emergency SOS → Agora */
    Q_INVOKABLE void triggerEmergency();

    /* Archived skills for a person */
    Q_INVOKABLE QVariantList archivedSkills(const QString &personId);

    /* Shop */
    Q_INVOKABLE bool buyItem(const QString &itemId, const QString &personId = "");
    Q_INVOKABLE bool buyCustomTitle(const QString &personId, const QString &title);
    Q_INVOKABLE QString exportCsv();

    /* Birthdays */
    Q_INVOKABLE void birthdayAdd(const QString &name, const QString &date);
    Q_INVOKABLE void birthdayDelete(int index);

signals:
    void personsChanged();
    void statsChanged();
    void timerChanged();
    void dailyTodosChanged();
    void calendarSlotsChanged();
    void coinsChanged();
    void tasksChanged();
    void challengesChanged();
    void entropyChanged();
    void historyChanged();
    void diaryChanged();
    void birthdaysChanged();
    void moodChanged();
    void boostChanged();
    void charChecklistsChanged();
    void effectPrompt(const QString &personName, const QString &skillName, int minutes);
    void coinsEarned(int amount);

private slots:
    void tick(); /* called every second when timer runs */

private:
    GameState m_state;
    QTimer m_tickTimer;
    bool m_timerRunning = false;
    QString m_timerPersonId;
    QString m_timerSkillName;
    QDateTime m_timerStart;
    int m_timerPlannedMin = 0;
    std::string m_statePath = "data/state.json";

    /* Convert Person to QVariantMap for QML */
    QVariantMap personToVariant(const Person &p, const std::vector<Person> &all) const;
    /* Convert skill to QVariantMap */
    static QVariantMap skillToVariant(const Skill &s);
};

#endif /* TOMOGICHI_BACKEND_H */
