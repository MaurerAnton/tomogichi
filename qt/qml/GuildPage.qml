import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.Page {
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
        anchors.fill: parent
        source: Backend.wallpaper ? "file://" + Backend.wallpaper : ""
        fillMode: Image.PreserveAspectCrop
        opacity: 0.12
        visible: Backend.wallpaper.length > 0
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
            color: "#FFC107"
            visible: Backend.coins > 0
        }

        // Active boosts
        Row {
            spacing: 8
            visible: Backend.xpBoostActive || Backend.coinsDoubleActive
            Label {
                text: "⚡ XP Boost active"
                font.pixelSize: 11; color: "#4CAF50"
                visible: Backend.xpBoostActive
            }
            Label {
                text: "🪙 2× Coins active"
                font.pixelSize: 11; color: "#FFC107"
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
                color: "#EF5350"
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
                    color: Backend.teamSync >= 75 ? "#4CAF50" :
                           Backend.teamSync >= 50 ? "#FFC107" : Kirigami.Theme.disabledTextColor
                }
                Rectangle {
                    Layout.fillWidth: true
                    height: 8; radius: 4
                    color: Qt.rgba(0.5, 0.5, 0.5, 0.3)
                    Rectangle {
                        width: parent.width * Backend.teamSync / 100
                        height: parent.height; radius: 4
                        color: "#4CAF50"
                    }
                }

                Label {
                    text: "💀 Entropy: " + Backend.entropy
                    font.pixelSize: 13
                    color: Backend.entropy > 50 ? "#EF5350" :
                           Backend.entropy > 25 ? "#FFC107" : Kirigami.Theme.disabledTextColor
                }
                Rectangle {
                    Layout.preferredWidth: 60
                    height: 8; radius: 4
                    color: Qt.rgba(0.5, 0.5, 0.5, 0.3)
                    Rectangle {
                        width: parent.width * Backend.entropy / 100
                        height: parent.height; radius: 4
                        color: Backend.entropy > 50 ? "#EF5350" : "#FFC107"
                    }
                }
            }

            color: Backend.entropy > 50 ? Qt.rgba(0.94, 0.2, 0.2, 0.1) : Qt.rgba(0, 0, 0, 0.05)
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
                color: Kirigami.Theme.disabledTextColor
            }
            Label {
                text: "Today: " + getTodayMins() + "m"
                font.pixelSize: 13
                color: Kirigami.Theme.highlightColor
            }
            Label {
                text: "Total: " + Backend.totalPracticeXP + " XP"
                font.pixelSize: 13
                color: Kirigami.Theme.positiveTextColor
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
                color: Kirigami.Theme.disabledTextColor
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
                        color: Kirigami.Theme.backgroundColor
                        border.width: 1
                        border.color: Qt.rgba(0.5, 0.5, 0.5, 0.3)

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
                            color: Kirigami.Theme.highlightColor
                            opacity: 0.8
                            Label {
                                anchors.centerIn: parent
                                text: "▶"
                                font.pixelSize: 14
                                color: "white"
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
                                    Label { text: model.role; font.pixelSize: 14; color: Kirigami.Theme.textColor }
                                    Label {
                                        text: "✓"
                                        font.pixelSize: 12; color: "#4CAF50"
                                        visible: model.practicedToday
                                    }
                                }
                            }
                            }
                            Label {
                                text: "Lvl " + model.level + " · " + model.title
                                font.pixelSize: 13
                                color: "#4CAF50"
                            }
                            Item { Layout.fillHeight: true }
                            Label {
                                text: "▶ " + model.mainSkill
                                font.pixelSize: 12
                                color: Kirigami.Theme.highlightColor
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
}
