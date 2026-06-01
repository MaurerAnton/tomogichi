import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.Page {
    id: timerPage
    title: "Timer"

    property string personId: ""
    property string personName: ""
    property string skillName: ""

    property int elapsedSec: 0
    property int plannedMin: 0
    property bool running: false

    Timer {
        id: refreshTimer
        interval: 500
        running: timerPage.running
        repeat: true
        onTriggered: elapsedSec = Backend.timerElapsedSec
    }

    Component.onCompleted: {
        elapsedSec = Backend.timerElapsedSec
        plannedMin = Backend.timerPlannedMin
        running = true
        refreshTimer.start()
    }

    Connections {
        target: Backend
        function onTimerChanged() {
            elapsedSec = Backend.timerElapsedSec
            running = Backend.timerRunning
            if (!running) refreshTimer.stop()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 20

        Item { Layout.fillHeight: true }

        ColumnLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 4
            Label {
                text: personName
                font.pixelSize: 18; font.bold: true
                color: Kirigami.Theme.highlightColor
                Layout.alignment: Qt.AlignHCenter
            }
            Label {
                text: skillName
                font.pixelSize: 14
                color: Kirigami.Theme.disabledTextColor
                Layout.alignment: Qt.AlignHCenter
            }
        }

        Label {
            text: formatTime(elapsedSec)
            font.pixelSize: 64; font.bold: true; font.family: "monospace"
            Layout.alignment: Qt.AlignHCenter
            color: plannedMin > 0 && elapsedSec > plannedMin * 60 ? "#EF5350" : Kirigami.Theme.textColor
        }

        Label {
            text: plannedMin > 0 ? "Planned: " + plannedMin + " min" : "Free practice"
            font.pixelSize: 13
            Layout.alignment: Qt.AlignHCenter
            color: Kirigami.Theme.disabledTextColor
        }

        Item { Layout.fillHeight: true }

        Button {
            text: "Stop & Log"
            Layout.fillWidth: true
            Layout.preferredHeight: 52
            font.pixelSize: 16
            onClicked: Backend.stopTimer()
        }

        Button {
            text: "Cancel"
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            flat: true
            onClicked: {
                Backend.cancelTimer()
                root.pageStack.pop()
            }
        }
    }

    function formatTime(sec) {
        var m = Math.floor(sec / 60)
        var s = sec % 60
        return (m < 10 ? "0" : "") + m + ":" + (s < 10 ? "0" : "") + s
    }
}
