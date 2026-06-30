#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QPalette>
#include <QColor>
#include "backend.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    app.setOrganizationName("tomogichi");
    app.setApplicationName("Tomogichi");

    Backend backend;
    backend.loadState();

    // Force dark palette if theme==2 (Dark)
    if (backend.theme() == 2) {
        QPalette darkPalette;
        darkPalette.setColor(QPalette::Window, QColor(30,30,30));
        darkPalette.setColor(QPalette::WindowText, QColor(220,220,220));
        darkPalette.setColor(QPalette::Base, QColor(45,45,45));
        darkPalette.setColor(QPalette::AlternateBase, QColor(35,35,35));
        darkPalette.setColor(QPalette::Text, QColor(220,220,220));
        darkPalette.setColor(QPalette::Button, QColor(45,45,45));
        darkPalette.setColor(QPalette::ButtonText, QColor(220,220,220));
        darkPalette.setColor(QPalette::BrightText, QColor(255,100,100));
        darkPalette.setColor(QPalette::Highlight, QColor(60,100,180));
        darkPalette.setColor(QPalette::HighlightedText, QColor(240,240,240));
        app.setPalette(darkPalette);
    }

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("Backend", &backend);
    engine.load(QUrl(QStringLiteral("qrc:///org/tomogichi/qt/qml/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
