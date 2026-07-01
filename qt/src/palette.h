#include <QtQml>
#pragma once
#include <QObject>
#include <QColor>
#include <QGuiApplication>

class TomoPalette : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool dark READ dark WRITE setDark NOTIFY darkChanged FINAL)
    Q_PROPERTY(QColor bg READ bg NOTIFY darkChanged FINAL)
    Q_PROPERTY(QColor text READ text NOTIFY darkChanged FINAL)
    Q_PROPERTY(QColor subText READ subText NOTIFY darkChanged FINAL)
    Q_PROPERTY(QColor highlight READ highlight NOTIFY darkChanged FINAL)
    Q_PROPERTY(QColor accent READ accent NOTIFY darkChanged FINAL)
    Q_PROPERTY(QColor danger READ danger NOTIFY darkChanged FINAL)
    Q_PROPERTY(QColor border READ border NOTIFY darkChanged FINAL)
    Q_PROPERTY(QColor dim READ dim NOTIFY darkChanged FINAL)
public:
    TomoPalette(QObject* p = nullptr) : QObject(p) {}
    
    bool dark() const { return m_dark; }
    void setDark(bool d) { if (d != m_dark) { m_dark = d; emit darkChanged(); } }
    
    QColor bg() const      { return m_dark ? QColor("#1e1e1e") : QColor("#fcfcfc"); }
    QColor text() const    { return m_dark ? QColor("#e0e0e0") : QColor("#333333"); }
    QColor subText() const { return m_dark ? QColor("#909090") : QColor("#777777"); }
    QColor highlight() const { return m_dark ? QColor("#5294e2") : QColor("#3d8ce8"); }
    QColor accent() const  { return m_dark ? QColor("#4caf50") : QColor("#388e3c"); }
    QColor danger() const  { return m_dark ? QColor("#f44336") : QColor("#d32f2f"); }
    QColor border() const  { return m_dark ? QColor("#404040") : QColor("#e0e0e0"); }
    QColor dim() const     { return m_dark ? QColor("#333333") : QColor("#eeeeee"); }

signals:
    void darkChanged();

private:
    bool m_dark = false;
};

