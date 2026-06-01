import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import org.kde.kirigami as Kirigami

Kirigami.ApplicationWindow {
    id: root
    width: 400
    height: 600
    title: "Tomogichi"

    pageStack.initialPage: Kirigami.Page {
        title: "Guild"
        Kirigami.Heading {
            anchors.centerIn: parent
            text: "Tomogichi"
            level: 1
        }
    }
}
