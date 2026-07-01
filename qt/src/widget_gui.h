#pragma once
#include <QMainWindow>
#include <QTabWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>
#include <QListWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QTimer>
#include <QScrollArea>
#include "backend.h"

class TomoWindow : public QMainWindow {
    Q_OBJECT
public:
    TomoWindow(Backend* b, QWidget* p = nullptr);
    void rebuild();

private:
    Backend* be;
    QTabWidget* tabs;
    QLabel* timerLabel;
    QTimer* tick;

    bool isDark() const { return be->theme() == 2; }
    QColor c(const char* n) const { return be->getColor(n); }
    QString css(const QString& prop, const QColor& col) const {
        return QString("%1: %2;").arg(prop, col.name());
    }

    QWidget* buildGuildTab();
    QWidget* buildTodayTab();
    QWidget* buildStatsTab();
    QWidget* buildSettingsTab();

    void startTimer(const QString& personId, const QString& skill, int minutes);
    void stopTimer();
};
