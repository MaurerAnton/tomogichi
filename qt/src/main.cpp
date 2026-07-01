#include <QApplication>
#include "backend.h"
#include "widget_gui.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    Backend backend;
    backend.loadState();
    
    TomoWindow window(&backend);
    window.show();
    
    return app.exec();
}
