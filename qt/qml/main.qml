import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ApplicationWindow {
    id: root
    title: "Tomogichi"
    width: 400
    height: 720

    pageStack.initialPage: guildPage

    // Theme: applied on load and whenever changed in Settings
    Component.onCompleted: {
        if (Backend.theme === 1)
            Kirigami.Theme.colorSet = Kirigami.Theme.Light
        else if (Backend.theme === 2)
            Kirigami.Theme.colorSet = Kirigami.Theme.Dark
        else
            Kirigami.Theme.colorSet = Kirigami.Theme.Window
    }
    Connections {
        target: Backend
        function onBoostChanged() {
            if (Backend.theme === 1)
                Kirigami.Theme.colorSet = Kirigami.Theme.Light
            else if (Backend.theme === 2)
                Kirigami.Theme.colorSet = Kirigami.Theme.Dark
            else
                Kirigami.Theme.colorSet = Kirigami.Theme.Window
        }
    }

    // Shared timer chip formatter
    function formatTimerChip() {
        var sec = Backend.timerElapsedSec
        var m = Math.floor(sec / 60)
        var s = sec % 60
        return (m < 10 ? "0" : "") + m + ":" + (s < 10 ? "0" : "") + s
    }

    // Timer indicator chip
    Rectangle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 8
        width: timerChipText.implicitWidth + 20
        height: 28
        radius: 14
        color: Kirigami.Theme.highlightColor
        visible: Backend.timerRunning
        z: 250

        Label {
            id: timerChipText
            anchors.centerIn: parent
            text: "⏱ " + formatTimerChip()
            font.pixelSize: 12
            color: "white"
        }

        Timer {
            interval: 1000
            running: Backend.timerRunning
            repeat: true
            onTriggered: timerChipText.text = "⏱ " + formatTimerChip()
        }
    }

    // Onboarding overlay for first launch
    Loader {
        id: onboardingLoader
        anchors.fill: parent
        active: !Backend.onboardingSeen && Backend.totalSessions === 0
        sourceComponent: onboardingOverlay
        z: 300
    }

    Component {
        id: onboardingOverlay
        Rectangle {
            anchors.fill: parent
            color: Kirigami.Theme.backgroundColor
            ColumnLayout {
                anchors.centerIn: parent
                spacing: 20
                width: Math.min(parent.width * 0.85, 400)

                Kirigami.Heading {
                    text: "Welcome to Tomogichi!"
                    level: 1
                    Layout.alignment: Qt.AlignHCenter
                }

                Label {
                    text: "Your personal RPG system for skill tracking.\n\n👥 4 characters: Riff, Reef, Pitch, Rain\n🎯 Practice skills, earn XP, level up\n💀 Watch entropy — neglect punishes\n🏆 Complete challenges, earn coins\n🛒 Spend coins in the Shop\n\nTap a character card to see their skills,\nor tap ▶ to start practicing immediately."
                    font.pixelSize: 14
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                Button {
                    text: "Let's Go!"
                    Layout.fillWidth: true
                    Layout.preferredHeight: 48
                    font.pixelSize: 16
                    onClicked: {
                        Backend.markOnboardingSeen()
                        onboardingLoader.active = false
                    }
                }
            }
        }
    }

    // Pages
    Component { id: guildPage; GuildPage {} }
    Component { id: todayPage; TodayPage {} }
    Component { id: statsPage; StatsPage {} }
    Component { id: settingsPage; SettingsPage {} }
    Component { id: diaryFullPage; DiaryPage {} }

    // Effect picker — shown after timer stop
    Loader {
        id: effectLoader
        anchors.fill: parent
        active: false
        sourceComponent: effectPicker
    }

    Component {
        id: effectPicker
        Rectangle {
            anchors.fill: parent
            color: Qt.rgba(0, 0, 0, 0.7)
            z: 100
            MouseArea { anchors.fill: parent }

            Rectangle {
                anchors.centerIn: parent
                width: Math.min(parent.width * 0.85, 380)
                height: col.implicitHeight + 40
                radius: 16
                color: Kirigami.Theme.backgroundColor

                ColumnLayout {
                    id: col
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: 20
                    spacing: 12

                    Label {
                        text: "How do you feel?"
                        font.pixelSize: 20
                        font.bold: true
                        Layout.alignment: Qt.AlignHCenter
                    }
                    Label {
                        id: effectTitle
                        text: ""
                        font.pixelSize: 14
                        color: Kirigami.Theme.disabledTextColor
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Label {
                        text: "(tap to log, long-press character to change)"
                        font.pixelSize: 10
                        color: Qt.rgba(0.5, 0.5, 0.5, 0.5)
                        Layout.alignment: Qt.AlignHCenter
                    }

                    RowLayout {
                        spacing: 8
                        Layout.alignment: Qt.AlignHCenter

                        Repeater {
                            model: [
                                { t: "Energized", i: "🔋", e: "energized", c: "#4CAF50" },
                                { t: "Neutral",   i: "😐", e: "neutral",   c: "#FFC107" },
                                { t: "Tired",     i: "😮‍💨", e: "tired",    c: "#FF5722" },
                                { t: "Drained",   i: "🪫", e: "drained",   c: "#9E9E9E" }
                            ]
                            delegate: Rectangle {
                                width: 72; height: 90; radius: 12
                                color: modelData.c
                                opacity: 0.2
                                ColumnLayout {
                                    anchors.centerIn: parent
                                    Label { text: modelData.i; font.pixelSize: 28; Layout.alignment: Qt.AlignHCenter }
                                    Label { text: modelData.t; font.pixelSize: 11; Layout.alignment: Qt.AlignHCenter }
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        var n = noteInput.text.trim()
                                        if (n.length > 0) {
                                            Backend.logEffectNote(modelData.e, n)
                                        } else {
                                            Backend.logEffect(modelData.e)
                                        }
                                        effectLoader.active = false
                                        while (root.pageStack.depth > 1 &&
                                               root.pageStack.currentItem &&
                                               root.pageStack.currentItem.objectName === "timerPage")
                                            root.pageStack.pop()
                                    }
                                }
                            }
                        }
                    }

                    TextField {
                        id: noteInput
                        Layout.fillWidth: true
                        placeholderText: "Note? (optional)"
                        font.pixelSize: 13
                    }
                }
            }
        }
    }

    Connections {
        target: Backend
        function onEffectPrompt(personName, skillName, minutes) {
            effectTitle.text = personName + " - " + skillName + " (" + minutes + " min)"
            effectLoader.active = true
        }
        function onCoinsEarned(amount) {
            coinToastText.text = "🏆 +" + amount + " 💰"
            coinToast.visible = true
            coinToastTimer.restart()
        }
    }

    // Coin toast
    Rectangle {
        id: coinToast
        anchors.horizontalCenter: parent.horizontalCenter
        y: 60
        width: 160; height: 40; radius: 20
        color: "#FFC107"
        visible: false; z: 200
        Label {
            id: coinToastText
            anchors.centerIn: parent
            text: ""
            font.pixelSize: 16; font.bold: true
            color: "#333"
        }
    }
    Timer {
        id: coinToastTimer
        interval: 2500
        onTriggered: coinToast.visible = false
    }

    // Bottom tab bar
    footer: TabBar {
        id: tabBar
        TabButton { text: "🏠 Guild"; onClicked: root.pageStack.replace(guildPage) }
        TabButton { text: "📅 Today"; onClicked: root.pageStack.replace(todayPage) }
        TabButton { text: "📊 Stats"; onClicked: root.pageStack.replace(statsPage) }
        TabButton { text: "⚙"; onClicked: root.pageStack.replace(settingsPage) }
    }
}
