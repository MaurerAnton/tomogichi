import QtQuick
import QtQuick.Controls

ApplicationWindow {
    visible: true
    width: 400
    height: 300
    title: "Test"

    Label {
        anchors.centerIn: parent
        text: "Hello Qt Quick!"
        font.pixelSize: 24
    }
}
