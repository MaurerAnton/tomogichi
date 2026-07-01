import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.Page {
    background: Rectangle { color: Backend.getColor("bg") }
    leftPadding: 12
    rightPadding: 12
    topPadding: 8
    id: guildPage
    title: "Guild"

    ListModel { id: personModel }

    Component.onCompleted: refreshPersons()

    function refreshPersons() {
        personModel.clear()
        var persons = Backend.persons
        for (var i = 0; i < persons.length; i++) {
            var p = persons[i]
            var mainSkill = ""
            if (p.skills && p.skills.length > 0) {
                for (var j = 0; j < p.skills.length; j++) {
                    if (p.skills[j].isMain) { mainSkill = p.skills[j].name; break }
                }
                if (!mainSkill) mainSkill = p.skills[0].name
            }
            personModel.append({
                pid: p.id, name: p.name, role: p.role,
                level: p.level, title: p.title, mood: p.moodEmoji,
                mainSkill: mainSkill, guildLvl: p.guildLevel || 0,
                practicedToday: p.practicedToday || false
            })
        }
    }

    Connections {
        target: Backend
        function onPersonsChanged() { refreshPersons() }
    }

    // Background wallpaper
    Image {
        id: bgImage
        anchors.fill: parent
        source: Backend.wallpaper.length > 0 ? Backend.wallpaper : ""
        fillMode: Image.PreserveAspectCrop
        opacity: 0.2
        visible: Backend.wallpaper.length > 0 && bgImage.status === Image.Ready
        onStatusChanged: {
            if (status === Image.Error && Backend.wallpaper.length > 0) {
                bgError.visible = true
                bgError.text = "Wallpaper not found: " + Backend.wallpaper
            }
        }
    }
    Label {
        id: bgError
        anchors.centerIn: parent
        visible: false
        text: ""
        color: Backend.getColor("danger")
        font.pixelSize: 12
        wrapMode: Text.WordWrap
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        Kirigami.Heading {
            text: "Guild Level " + (personModel.count > 0 ? personModel.get(0).guildLvl : 0)
            level: 2
        }

        // Coins
        Label {
            text: "💰 " + Backend.coins + " coins"
            font.pixelSize: 13
            color: Backend.getColor("highlight")
            visible: Backend.coins > 0
        }

        // Emergency SOS button
        Rectangle {
            Layout.fillWidth: true
            height: 44
            radius: 10
            color: Qt.rgba(0.94, 0.2, 0.2, 0.15)
            border.width: 2
            border.color: Backend.getColor("danger")
            visible: true
            RowLayout {
                anchors.centerIn: parent
                spacing: 8
                Label { text: "🚨"; font.pixelSize: 22 }
                Label { text: "EMERGENCY — Notify AI (bad state)"; font.pixelSize: 14; color: Backend.getColor("danger"); font.bold: true }
            }
            MouseArea {
                anchors.fill: parent
                onClicked: emergencyDialog.open()
            }
        }

        // Active boosts
        Row {
            spacing: 8
            visible: Backend.xpBoostActive || Backend.coinsDoubleActive
            Label {
                text: "⚡ XP Boost active"
                font.pixelSize: 11; color: Backend.getColor("accent")
                visible: Backend.xpBoostActive
            }
            Label {
                text: "🪙 2× Coins active"
                font.pixelSize: 11; color: Backend.getColor("highlight")
                visible: Backend.coinsDoubleActive
            }
        }

        // Neglect warning
        Rectangle {
            Layout.fillWidth: true
            height: 28
            radius: 8
            color: Qt.rgba(0.94, 0.2, 0.2, 0.12)
            visible: getNeglectName() !== ""

            function getNeglectName() {
                var s = Backend.todaySummary()
                return s.neglectedName || ""
            }
            function getNeglectDays() {
                var s = Backend.todaySummary()
                return s.neglectedDays || 0
            }

            Connections {
                target: Backend
                function onPersonsChanged() {} // triggers repaint
                function onHistoryChanged() {}  // triggers repaint
            }

            Label {
                anchors.centerIn: parent
                text: "💔 " + parent.getNeglectName() + " needs practice! (" + parent.getNeglectDays() + "d idle)"
                font.pixelSize: 12
                color: Backend.getColor("danger")
            }
        }

        // Entropy + Sync bar
        Rectangle {
            Layout.fillWidth: true
            height: 36
            radius: 8
            visible: true

            RowLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 12

                Label {
                    text: "⚡ Sync: " + Backend.teamSync + "%"
                    font.pixelSize: 13
                    color: Backend.teamSync >= 75 ? Backend.getColor("accent") :
                           Backend.teamSync >= 50 ? Backend.getColor("highlight") : Backend.getColor("subText")
                }
                Rectangle {
                    Layout.fillWidth: true
                    height: 8; radius: 4
                    color: Backend.getColor("border")
                    Rectangle {
                        width: parent.width * Backend.teamSync / 100
                        height: parent.height; radius: 4
                        color: Backend.getColor("accent")
                    }
                }

                Label {
                    text: "💀 Entropy: " + Backend.entropy
                    font.pixelSize: 13
                    color: Backend.entropy > 50 ? Backend.getColor("danger") :
                           Backend.entropy > 25 ? Backend.getColor("highlight") : Backend.getColor("subText")
                }
                Rectangle {
                    Layout.preferredWidth: 60
                    height: 8; radius: 4
                    color: Backend.getColor("border")
                    Rectangle {
                        width: parent.width * Backend.entropy / 100
                        height: parent.height; radius: 4
                        color: Backend.entropy > 50 ? Backend.getColor("danger") : Backend.getColor("highlight")
                    }
                }
            }

            color: Backend.entropy > 50 ? Qt.rgba(0.94, 0.2, 0.2, 0.1) : Backend.getColor("dim")
        }

        Connections {
            target: Backend
            function onEntropyChanged() {}  // triggers repaint
        }

        // Quick weekly stats
        RowLayout {
            spacing: 16
            Label {
                text: "This week: " + getWeekTotal() + "h"
                font.pixelSize: 13
                color: Backend.getColor("subText")
            }
            Label {
                text: "Today: " + getTodayMins() + "m"
                font.pixelSize: 13
                color: Backend.getColor("highlight")
            }
            Label {
                text: "Total: " + Backend.totalPracticeXP + " XP"
                font.pixelSize: 13
                color: Backend.getColor("accent")
            }
        }

        // Today's practice
        Repeater {
            model: Backend.practiceHistory
            delegate: Label {
                visible: index < 3 && isToday(modelData.timestamp)
                text: "  " + modelData.personId + " " + modelData.skillName + " " + modelData.minutes + "m  " +
                      (modelData.effect === "energized" ? "🔋" : modelData.effect === "tired" ? "😮‍💨" : modelData.effect === "drained" ? "🪫" : "😐")
                font.pixelSize: 12
                color: Backend.getColor("subText")
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true

            GridLayout {
                width: parent.width
                columns: 1
                columnSpacing: 10
                rowSpacing: 10

                Repeater {
                    model: personModel
                    delegate: Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 160
                        radius: 12
                        color: Backend.getColor("bg")
                        border.width: 1
                        border.color: Backend.getColor("border")

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                var p = personModel.get(index)
                                applicationWindow().pageStack.push(personPageComponent, {
                                    personId: p.pid,
                                    personName: p.name,
                                    personRole: p.role,
                                    personLevel: p.level,
                                    personTitle: p.title,
                                    personMood: p.mood
                                })
                            }
                        }

                        // Quick-start timer button
                        Rectangle {
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            anchors.margins: 8
                            width: 32; height: 32; radius: 16
                            color: Backend.getColor("highlight")
                            opacity: 0.8
                            Label {
                                anchors.centerIn: parent
                                text: "▶"
                                font.pixelSize: 14
                                color: Backend.getColor("text")
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    var p = personModel.get(index)
                                    Backend.startTimer(p.pid, p.mainSkill, 30)
                                    root.pageStack.push(timerComponent, {
                                        personId: p.pid,
                                        personName: p.name,
                                        skillName: p.mainSkill
                                    })
                                }
                            }
                        }

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 4

                            RowLayout {
                                Label { text: model.mood; font.pixelSize: 28 }
                            ColumnLayout {
                                spacing: 0
                                Label { text: model.name; font.pixelSize: 18; font.bold: true }
                                RowLayout {
                                    Label { text: model.role; font.pixelSize: 14; color: Backend.getColor("text") }
                                    Label {
                                        text: "✓"
                                        font.pixelSize: 12; color: Backend.getColor("accent")
                                        visible: model.practicedToday
                                    }
                                }
                            }
                            }
                            Label {
                                text: "Lvl " + model.level + " · " + model.title
                                font.pixelSize: 13
                                color: Backend.getColor("accent")
                            }
                            Item { Layout.fillHeight: true }
                            Label {
                                text: "▶ " + model.mainSkill
                                font.pixelSize: 12
                                color: Backend.getColor("highlight")
                            }
                        }
                    }
                }
            }
        }
    }

    // Person page component
    Component {
        id: personPageComponent
        PersonPage {}
    }

    function getWeekTotal() {
        var pie = Backend.weeklyPie
        var total = 0
        for (var i = 0; i < pie.length; i++) total += pie[i].minutes
        return (total / 60).toFixed(1)
    }
    function getTodayMins() {
        var hm = Backend.monthlyHeatmap
        var today = new Date().getDate()
        for (var i = 0; i < hm.length; i++) {
            if (hm[i].isToday) return hm[i].minutes
        }
        return 0
    }

    function isToday(ts) {
        var d = new Date(ts * 1000)
        var now = new Date()
        return d.getDate() === now.getDate() &&
               d.getMonth() === now.getMonth() &&
               d.getFullYear() === now.getFullYear()
    }

    // Timer component for quick-start
    Component {
        id: timerComponent
        TimerPage {}
    }

    // Emergency confirmation dialog
    Dialog {
        id: emergencyDialog
        title: "🚨 Trigger Emergency Protocol?"
        ColumnLayout { spacing: 10; width: 280
            Label {
                text: "This will notify the AI in Agora that you're in a bad state."
                wrapMode: Text.WordWrap; font.pixelSize: 13
            }
            Label {
                text: "The AI will analyze last week's activity, check neglected characters, and create a recovery plan."
                wrapMode: Text.WordWrap; font.pixelSize: 12; color: Backend.getColor("subText")
            }
        }
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: {
            Backend.triggerEmergency()
            root.pageStack.replace(todayPage)
        }
    }
}
