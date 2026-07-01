#pragma once
#include <QObject>
#include <QColor>

class AppTheme : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isDark READ isDark WRITE setIsDark NOTIFY isDarkChanged FINAL)
    Q_PROPERTY(QColor bg READ bg NOTIFY colorsChanged FINAL)
    Q_PROPERTY(QColor text READ text NOTIFY colorsChanged FINAL)
    Q_PROPERTY(QColor subText READ subText NOTIFY colorsChanged FINAL)
    Q_PROPERTY(QColor highlight READ highlight NOTIFY colorsChanged FINAL)
    Q_PROPERTY(QColor accent READ accent NOTIFY colorsChanged FINAL)
    Q_PROPERTY(QColor danger READ danger NOTIFY colorsChanged FINAL)
    Q_PROPERTY(QColor border READ border NOTIFY colorsChanged FINAL)
    Q_PROPERTY(QColor dim READ dim NOTIFY colorsChanged FINAL)
public:
    explicit AppTheme(QObject* p = nullptr) : QObject(p) { update(); }
    bool isDark() const { return m_dark; }
    void setIsDark(bool d) { if (d != m_dark) { m_dark = d; update(); emit isDarkChanged(); } }
    QColor bg() const { return m_bg; }
    QColor text() const { return m_text; }
    QColor subText() const { return m_subText; }
    QColor highlight() const { return m_highlight; }
    QColor accent() const { return m_accent; }
    QColor danger() const { return m_danger; }
    QColor border() const { return m_border; }
    QColor dim() const { return m_dim; }
signals:
    void isDarkChanged();
    void colorsChanged();
private:
    void update() {
        if (m_dark) {
            m_bg="#1e1e1e"; m_text="#e0e0e0"; m_subText="#909090"; m_highlight="#5294e2";
            m_accent="#4caf50"; m_danger="#f44336"; m_border="#404040"; m_dim="#333333";
        } else {
            m_bg="#fcfcfc"; m_text="#333333"; m_subText="#777777"; m_highlight="#3d8ce8";
            m_accent="#388e3c"; m_danger="#d32f2f"; m_border="#e0e0e0"; m_dim="#eeeeee";
        }
        emit colorsChanged();
    }
    bool m_dark=false;
    QColor m_bg, m_text, m_subText, m_highlight, m_accent, m_danger, m_border, m_dim;
};
