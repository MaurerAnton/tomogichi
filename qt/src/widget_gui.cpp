#include "widget_gui.h"
#include <QApplication>
#include <QScrollArea>
#include <QFrame>
#include <QMessageBox>
#include <QInputDialog>
#include <QProgressBar>
#include <QGroupBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QTableWidget>
#include <sstream>
#include <iomanip>

static QString fmtTime(int64_t ms) {
    auto t = static_cast<time_t>(ms / 1000);
    char buf[16]; strftime(buf, sizeof(buf), "%H:%M", localtime(&t));
    return buf;
}

TomoWindow::TomoWindow(Backend* b, QWidget* p) : QMainWindow(p), be(b) {
    setWindowTitle("Tomogichi");
    resize(420, 680);
    setMinimumSize(360, 500);
    
    auto* central = new QWidget;
    setCentralWidget(central);
    auto* layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    
    timerLabel = new QLabel;
    timerLabel->setVisible(false);
    timerLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(timerLabel);
    
    tabs = new QTabWidget;
    layout->addWidget(tabs);
    
    tick = new QTimer(this);
    connect(tick, &QTimer::timeout, this, [this]() {
        if (!be->timerRunning()) { tick->stop(); timerLabel->setVisible(false); return; }
        int s = be->timerElapsedSec();
        timerLabel->setText(QString("⏱ %1:%2  %3 — %4")
            .arg(s/60, 2, 10, QChar('0')).arg(s%60, 2, 10, QChar('0'))
            .arg(be->timerPersonId(), be->timerSkillName()));
    });
    
    rebuild();
}

void TomoWindow::rebuild() {
    bool d = isDark();
    QString bg = c("bg").name(), fg = c("text").name(), sub = c("subText").name();
    QString hl = c("highlight").name(), acc = c("accent").name(), danger = c("danger").name();
    QString border = c("border").name(), dim = c("dim").name();
    
    setStyleSheet(QString(
        "QMainWindow, QWidget { background-color: %1; color: %2; }"
        "QTabWidget::pane { border: 1px solid %3; background: %1; }"
        "QTabBar::tab { background: %4; color: %2; padding: 8px 12px; border: 1px solid %3; }"
        "QTabBar::tab:selected { background: %5; color: #fff; }"
        "QPushButton { background: %5; color: #fff; border: none; padding: 6px 12px; border-radius: 4px; }"
        "QPushButton:hover { background: %6; }"
        "QPushButton#danger { background: %7; }"
        "QLineEdit, QTextEdit, QComboBox { background: %4; color: %2; border: 1px solid %3; padding: 4px; }"
        "QListWidget { background: %4; color: %2; border: 1px solid %3; }"
        "QGroupBox { border: 1px solid %3; margin-top: 8px; padding-top: 16px; color: %2; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 8px; color: %5; }"
        "QProgressBar { background: %4; border: 1px solid %3; text-align: center; }"
        "QProgressBar::chunk { background: %5; }"
        "QScrollArea { border: none; background: transparent; }"
        "QTableWidget { background: %4; gridline-color: %3; color: %2; }"
        "QHeaderView::section { background: %4; color: %2; border: 1px solid %3; padding: 4px; }"
    ).arg(bg, fg, border, dim, hl, acc, danger));
    
    int cur = tabs->currentIndex();
    tabs->clear();
    tabs->addTab(buildGuildTab(), "  🏠 Guild  ");
    tabs->addTab(buildTodayTab(), "  📅 Today  ");
    tabs->addTab(buildStatsTab(), "  📊 Stats  ");
    tabs->addTab(buildSettingsTab(), "  ⚙  ");
    if (cur >= 0 && cur < 4) tabs->setCurrentIndex(cur);
}

// ── Guild Tab ──

QWidget* TomoWindow::buildGuildTab() {
    auto* page = new QWidget;
    auto* lay = new QVBoxLayout(page);
    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    lay->addWidget(scroll);
    auto* w = new QWidget;
    auto* l = new QVBoxLayout(w);
    
    // Guild stats
    auto* gb = new QGroupBox(QString("Guild — Entropy: %1/100 | Coins: %2 | Sync: %3%")
        .arg(be->entropy()).arg(be->coins()).arg(be->teamSync()));
    auto* gl = new QVBoxLayout(gb);
    auto* ep = new QProgressBar;
    ep->setRange(0, 100); ep->setValue(be->entropy());
    ep->setFormat(QString("Entropy %1%").arg(be->entropy()));
    gl->addWidget(ep);
    l->addWidget(gb);
    
    // Emergency button
    auto* emBtn = new QPushButton(" Emergency — Notify AI ");
    emBtn->setObjectName("danger");
    emBtn->setStyleSheet(QString("QPushButton#danger { background: %1; color: white; font-weight: bold; padding: 10px; }")
        .arg(c("danger").name()));
    connect(emBtn, &QPushButton::clicked, this, [this]() {
        system("~/tomogichi-agora-emergency on");
        QMessageBox::information(this, "Emergency", "AI will see emergency in next Agora chat.");
    });
    l->addWidget(emBtn);
    
    // Characters
    auto persons = be->persons();
    for (auto& pv : persons) {
        auto p = pv.toMap();
        auto* card = new QGroupBox(QString("%1 — %2 (Lvl %3)")
            .arg(p["name"].toString(), p["role"].toString()).arg(p["level"].toInt()));
        auto* cl = new QVBoxLayout(card);
        
        QString mood = p["moodEmoji"].toString();
        int idle = p["daysIdle"].toInt();
        if (idle >= 3) mood += QString(" ⚠️ idle %1d").arg(idle);
        cl->addWidget(new QLabel(QString("Mood: %1 | XP: %2").arg(mood).arg(p["xp"].toInt())));
        
        auto skills = p["skills"].toList();
        for (auto& sv : skills) {
            auto s = sv.toMap();
            QString star = s["isMain"].toBool() ? "★" : " ";
            auto* row = new QHBoxLayout;
            row->addWidget(new QLabel(QString("[%1] %2 — Lvl %3 (%4 XP)")
                .arg(star, s["name"].toString()).arg(s["level"].toInt()).arg(s["xp"].toInt())));
            auto* btn = new QPushButton("▶");
            btn->setFixedWidth(36);
            connect(btn, &QPushButton::clicked, this, [this, p, s]() {
                startTimer(p["id"].toString(), s["name"].toString(), 25);
            });
            row->addWidget(btn);
            cl->addLayout(row);
        }
        l->addWidget(card);
    }
    
    l->addStretch();
    scroll->setWidget(w);
    return page;
}

// ── Today Tab ──

QWidget* TomoWindow::buildTodayTab() {
    auto* page = new QWidget;
    auto* lay = new QVBoxLayout(page);
    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    lay->addWidget(scroll);
    auto* w = new QWidget;
    auto* l = new QVBoxLayout(w);
    
    // Calendar
    auto cal = be->calendarSlots();
    if (!cal.isEmpty()) {
        auto* gb = new QGroupBox("📅 Today's Calendar");
        auto* gl = new QVBoxLayout(gb);
        for (auto& cv : cal) {
            auto c = cv.toMap();
            gl->addWidget(new QLabel(QString("%1 — %2").arg(c["time"].toString(), c["label"].toString())));
        }
        l->addWidget(gb);
    }
    
    // Tasks
    auto tasks = be->tasks();
    if (!tasks.isEmpty()) {
        auto* gb = new QGroupBox("☐ Tasks");
        auto* gl = new QVBoxLayout(gb);
        for (auto& tv : tasks) {
            auto t = tv.toMap();
            auto* row = new QHBoxLayout;
            row->addWidget(new QLabel((t["done"].toBool() ? "☑ " : "☐ ") + t["text"].toString()));
            if (!t["done"].toBool()) {
                auto* doneBtn = new QPushButton("✓");
                doneBtn->setFixedWidth(32);
                connect(doneBtn, &QPushButton::clicked, this, [this, t]() {
                    be->dailyTodoToggle(t["index"].toInt());
                    rebuild();
                });
                row->addWidget(doneBtn);
            }
            gl->addLayout(row);
        }
        l->addWidget(gb);
    }
    
    // Today's todos
    auto todos = be->dailyTodos();
    if (!todos.isEmpty()) {
        auto* gb = new QGroupBox("☑ Daily");
        auto* gl = new QVBoxLayout(gb);
        for (auto& tv : todos) {
            auto t = tv.toMap();
            gl->addWidget(new QLabel((t["done"].toBool() ? "✅ " : "☐ ") + t["text"].toString()));
        }
        l->addWidget(gb);
    }
    
    // Challenges
    auto challenges = be->challenges();
    if (!challenges.isEmpty()) {
        auto* gb = new QGroupBox("🏆 Challenges");
        auto* gl = new QVBoxLayout(gb);
        for (auto& cv : challenges) {
            auto c = cv.toMap();
            int cur = c["currentCount"].toInt(), tgt = c["targetCount"].toInt();
            auto* pb = new QProgressBar;
            pb->setRange(0, tgt); pb->setValue(cur);
            pb->setFormat(QString("%1 — %2/%3").arg(c["description"].toString()).arg(cur).arg(tgt));
            gl->addWidget(pb);
        }
        l->addWidget(gb);
    }
    
    // Diary entries
    auto diary = be->diaryLog();
    if (!diary.isEmpty()) {
        auto* gb = new QGroupBox("📝 Diary");
        auto* gl = new QVBoxLayout(gb);
        for (auto& dv : diary) {
            auto d = dv.toMap();
            gl->addWidget(new QLabel(QString("[%1] %2")
                .arg(d["dateStr"].toString().mid(0, 5), d["text"].toString())));
        }
        l->addWidget(gb);
    }
    
    if (cal.isEmpty() && tasks.isEmpty() && todos.isEmpty() && challenges.isEmpty() && diary.isEmpty()) {
        l->addWidget(new QLabel("No items for today. Start practicing!"));
    }
    
    l->addStretch();
    scroll->setWidget(w);
    return page;
}

// ── Stats Tab ──

QWidget* TomoWindow::buildStatsTab() {
    auto* page = new QWidget;
    auto* lay = new QVBoxLayout(page);
    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    lay->addWidget(scroll);
    auto* w = new QWidget;
    auto* l = new QVBoxLayout(w);
    
    auto pie = be->weeklyPie();
    if (!pie.isEmpty()) {
        auto* gb = new QGroupBox("This Week — Minutes per Character");
        auto* gl = new QVBoxLayout(gb);
        int total = 0;
        for (auto& pv : pie) { auto p = pv.toMap(); total += p["minutes"].toInt(); }
        for (auto& pv : pie) {
            auto p = pv.toMap();
            int min = p["minutes"].toInt();
            auto* pb = new QProgressBar;
            pb->setRange(0, total > 0 ? total : 1); pb->setValue(min);
            pb->setFormat(QString("%1: %2 min").arg(p["personId"].toString()).arg(min));
            gl->addWidget(pb);
        }
        gl->addWidget(new QLabel(QString("Total: %1 min").arg(total)));
        l->addWidget(gb);
    }
    
    auto stats = be->skillEffectStats();
    if (!stats.isEmpty()) {
        auto* gb = new QGroupBox("Effect Analytics");
        auto* gl = new QVBoxLayout(gb);
        for (auto& sv : stats) {
            auto s = sv.toMap();
            gl->addWidget(new QLabel(QString("%1 — %2: 🔋%3  😐%4  😮‍💨%5  🪫%6")
                .arg(s["personId"].toString(), s["skillName"].toString())
                .arg(s["energized"].toInt()).arg(s["neutral"].toInt())
                .arg(s["tired"].toInt()).arg(s["drained"].toInt())));
        }
        l->addWidget(gb);
    }
    
    if (pie.isEmpty() && stats.isEmpty())
        l->addWidget(new QLabel("No stats yet. Practice to see data!"));
    
    l->addStretch();
    scroll->setWidget(w);
    return page;
}

// ── Settings Tab ──

QWidget* TomoWindow::buildSettingsTab() {
    auto* page = new QWidget;
    auto* l = new QVBoxLayout(page);
    
    // Theme
    auto* tg = new QGroupBox("Theme");
    auto* tl = new QHBoxLayout(tg);
    auto* combo = new QComboBox;
    combo->addItems({"System", "Light", "Dark"});
    combo->setCurrentIndex(be->theme());
    connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int i) {
        be->setTheme(i);
        rebuild();
    });
    tl->addWidget(new QLabel("Theme:"));
    tl->addWidget(combo);
    tl->addStretch();
    l->addWidget(tg);
    
    // Export
    auto* eg = new QGroupBox("Data");
    auto* el = new QHBoxLayout(eg);
    auto* expBtn = new QPushButton("Export CSV");
    connect(expBtn, &QPushButton::clicked, this, [this]() {
        QString path = be->exportCsv();
        QMessageBox::information(this, "Export", "Saved to: " + path);
    });
    el->addWidget(expBtn);
    
    auto* saveBtn = new QPushButton("Save State");
    connect(saveBtn, &QPushButton::clicked, this, [this]() { be->saveState(); });
    el->addWidget(saveBtn);
    el->addStretch();
    l->addWidget(eg);
    
    // Version
    auto* vg = new QGroupBox("About");
    auto* vl = new QVBoxLayout(vg);
    vl->addWidget(new QLabel("Tomogichi v0.7.0 — Qt Widgets"));
    vl->addWidget(new QLabel("Personal RPG practice tracker"));
    l->addWidget(vg);
    
    l->addStretch();
    return page;
}

// ── Timer ──

void TomoWindow::startTimer(const QString& pid, const QString& skill, int min) {
    be->startTimer(pid, skill, min);
    timerLabel->setVisible(true);
    tick->start(1000);
}

void TomoWindow::stopTimer() {
    if (be->timerRunning()) {
        be->stopTimer();
        tick->stop();
        timerLabel->setVisible(false);
        rebuild();
    }
}
