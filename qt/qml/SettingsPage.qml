import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.Page {
    id: settingsPage
    title: "Settings"
    property bool tasksOpen: false
    property bool challengesOpen: false
    property bool journalOpen: false
    property bool archiveOpen: false
    property bool shopOpen: false
    property bool bdaysOpen: false
    property bool systemOpen: false

    ScrollView {
        anchors.fill: parent
        anchors.margins: 12
        contentWidth: availableWidth

        ColumnLayout {
            width: parent ? parent.width - 24 : 360
            spacing: 6

            // Section helper component
            Component {
                id: sectionHeader
                Rectangle {
                    property alias text: headerLabel.text
                    property string emoji: ""
                    property bool isOpen: false
                    signal toggle()
                    Layout.fillWidth: true; height: 44; radius: 10
                    color: isOpen ? Kirigami.Theme.backgroundColor : Qt.rgba(0.5,0.5,0.5,0.08)
                    border.width: 1; border.color: Qt.rgba(0.5,0.5,0.5,0.2)
                    RowLayout {
                        anchors.fill: parent; anchors.margins: 12
                        Label { text: emoji; font.pixelSize: 18 }
                        Label {
                            id: headerLabel
                            text: ""
                            font.pixelSize: 15; font.bold: true
                            Layout.fillWidth: true
                        }
                        Label {
                            text: isOpen ? "▲" : "▼"
                            font.pixelSize: 12
                            color: Kirigami.Theme.disabledTextColor
                        }
                    }
                    MouseArea { anchors.fill: parent; onClicked: parent.toggle() }
                }
            }

            // ── Tasks ──
            Rectangle {
                Layout.fillWidth: true; height: 44; radius: 10
                color: tasksOpen ? Kirigami.Theme.backgroundColor : Qt.rgba(0.5,0.5,0.5,0.08)
                border.width: 1; border.color: Qt.rgba(0.5,0.5,0.5,0.2)
                RowLayout {
                    anchors.fill: parent; anchors.margins: 12
                    Label { text: "📋"; font.pixelSize: 18 }
                    Label { text: "Tasks"; font.pixelSize: 15; font.bold: true; Layout.fillWidth: true }
                    Label { text: tasksOpen ? "▲" : "▼"; font.pixelSize: 12; color: Kirigami.Theme.disabledTextColor }
                }
                MouseArea { anchors.fill: parent; onClicked: tasksOpen = !tasksOpen }
            }
            ColumnLayout {
                spacing: 6; visible: tasksOpen
                Layout.fillWidth: true
                Repeater {
                    model: Backend.tasks
                    delegate: RowLayout {
                        width: parent ? parent.width : 300; spacing: 8
                        CheckBox { checked: modelData.done; onClicked: Backend.taskToggle(modelData.id) }
                        Label {
                            text: modelData.text; Layout.fillWidth: true; font.pixelSize: 14
                            color: modelData.done ? Kirigami.Theme.disabledTextColor : Kirigami.Theme.textColor
                        }
                        Label {
                            text: "✕"; color: "#EF5350"; font.pixelSize: 14
                            MouseArea {
                                anchors.fill: parent
                                onClicked: { delTarget.type = "task"; delTarget.id = modelData.id; delConfirm.open() }
                            }
                        }
                    }
                }
                RowLayout { spacing: 8
                    TextField { id: newTaskInput; placeholderText: "New task..."; Layout.fillWidth: true; onAccepted: addTask() }
                    Button { text: "Add"; onClicked: addTask() }
                }
            }

            Kirigami.Separator { Layout.fillWidth: true }

            // ── Challenges ──
            Rectangle {
                Layout.fillWidth: true; height: 44; radius: 10
                color: challengesOpen ? Kirigami.Theme.backgroundColor : Qt.rgba(0.5,0.5,0.5,0.08)
                border.width: 1; border.color: Qt.rgba(0.5,0.5,0.5,0.2)
                RowLayout {
                    anchors.fill: parent; anchors.margins: 12
                    Label { text: "🏆"; font.pixelSize: 18 }
                    Label { text: "Challenges"; font.pixelSize: 15; font.bold: true; Layout.fillWidth: true }
                    Label { text: challengesOpen ? "▲" : "▼"; font.pixelSize: 12; color: Kirigami.Theme.disabledTextColor }
                }
                MouseArea { anchors.fill: parent; onClicked: challengesOpen = !challengesOpen }
            }
            ColumnLayout {
                spacing: 6; visible: challengesOpen
                Layout.fillWidth: true
                Repeater {
                    model: Backend.challenges
                    delegate: Rectangle {
                        width: parent ? parent.width : 300; height: 60; radius: 8
                        color: Kirigami.Theme.backgroundColor
                        border.width: 1; border.color: Qt.rgba(0.5,0.5,0.5,0.2)
                        opacity: modelData.completed ? 0.5 : 1.0
                        ColumnLayout {
                            anchors.fill: parent; anchors.margins: 8; spacing: 2
                            Label { text: modelData.description; font.pixelSize: 13; font.bold: true }
                            RowLayout {
                                spacing: 6
                                Label { text: modelData.currentCount + "/" + modelData.targetCount; font.pixelSize: 11; color: Kirigami.Theme.highlightColor }
                                Label { text: modelData.personId || "any"; font.pixelSize: 11; color: Kirigami.Theme.disabledTextColor }
                                Label { text: modelData.skillName || ""; font.pixelSize: 11 }
                                Item { Layout.fillWidth: true }
                                Label { text: "+" + modelData.coinReward + "💰"; font.pixelSize: 11; color: "#FFC107" }
                                Label {
                                    text: "✕"; color: "#EF5350"; font.pixelSize: 14
                                    MouseArea { anchors.fill: parent; onClicked: { delTarget.type = "challenge"; delTarget.index = index; delConfirm.open() } }
                                }
                            }
                        }
                    }
                }
                Button { text: "+ Add Challenge"; Layout.fillWidth: true; onClicked: addChalDialog.open() }
            }

            Kirigami.Separator { Layout.fillWidth: true }

            // ── Journal ──
            Rectangle {
                Layout.fillWidth: true; height: 44; radius: 10
                color: journalOpen ? Kirigami.Theme.backgroundColor : Qt.rgba(0.5,0.5,0.5,0.08)
                border.width: 1; border.color: Qt.rgba(0.5,0.5,0.5,0.2)
                RowLayout {
                    anchors.fill: parent; anchors.margins: 12
                    Label { text: "📓"; font.pixelSize: 18 }
                    Label { text: "Journal"; font.pixelSize: 15; font.bold: true; Layout.fillWidth: true }
                    Label { text: journalOpen ? "▲" : "▼"; font.pixelSize: 12; color: Kirigami.Theme.disabledTextColor }
                }
                MouseArea { anchors.fill: parent; onClicked: journalOpen = !journalOpen }
            }
            ColumnLayout {
                spacing: 6; visible: journalOpen
                Layout.fillWidth: true

                Label { text: "Mood"; font.pixelSize: 13; font.bold: true; color: Kirigami.Theme.highlightColor }
                Flow {
                    Layout.fillWidth: true; spacing: 6
                    Repeater {
                        model: [
                            { e: "😊", w: "happy" },     { e: "😢", w: "sad" },
                            { e: "😴", w: "tired" },     { e: "⚡", w: "energetic" },
                            { e: "😰", w: "anxious" },   { e: "😌", w: "calm" },
                            { e: "🎯", w: "focused" },   { e: "😵", w: "distracted" },
                            { e: "💪", w: "motivated" }, { e: "🦥", w: "lazy" },
                            { e: "🎨", w: "creative" },  { e: "🚧", w: "blocked" },
                            { e: "🙏", w: "grateful" },  { e: "😤", w: "frustrated" },
                            { e: "🕊️", w: "peaceful" }, { e: "🔄", w: "restless" },
                            { e: "🤩", w: "excited" },   { e: "🥱", w: "bored" },
                            { e: "🌟", w: "hopeful" },   { e: "🪫", w: "drained" },
                            { e: "🤔", w: "curious" },   { e: "🌊", w: "overwhelmed" },
                            { e: "😌", w: "content" },   { e: "🏆", w: "proud" },
                            { e: "💔", w: "lonely" }
                        ]
                        delegate: Rectangle {
                            width: label2.implicitWidth + 16; height: 32; radius: 16
                            color: Kirigami.Theme.backgroundColor
                            border.width: 1; border.color: Qt.rgba(0.5,0.5,0.5,0.3)
                            RowLayout { anchors.centerIn: parent; spacing: 4
                                Label { text: modelData.e; font.pixelSize: 16 }
                                Label { id: label2; text: modelData.w; font.pixelSize: 11 }
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    Backend.moodLog(modelData.w)
                                    moodToast.text = "Mood: " + modelData.e + " " + modelData.w
                                    moodToast.visible = true; moodToastTimer.restart()
                                }
                            }
                        }
                    }
                }
                Label { id: moodToast; visible: false; text: ""; font.pixelSize: 13; color: Kirigami.Theme.positiveTextColor }
                Timer { id: moodToastTimer; interval: 2000; onTriggered: moodToast.visible = false }

                Repeater {
                    model: Backend.moodHistory
                    delegate: Label {
                        visible: index < 8
                        text: modelData.dateStr + "  " + modelData.word
                        font.pixelSize: 12; color: Kirigami.Theme.disabledTextColor
                    }
                }

                Kirigami.Separator { Layout.fillWidth: true }

                RowLayout {
                    Label { text: "Diary"; font.pixelSize: 13; font.bold: true; color: Kirigami.Theme.highlightColor; Layout.fillWidth: true }
                    Label {
                        text: "View All →"; font.pixelSize: 12; color: Kirigami.Theme.highlightColor
                        MouseArea {
                            anchors.fill: parent
                            onClicked: applicationWindow().pageStack.push("qrc:/org/tomogichi/qt/qml/DiaryPage.qml")
                        }
                    }
                }
                Repeater {
                    model: Backend.diaryLog
                    delegate: ColumnLayout {
                        visible: index < 5
                        width: parent ? parent.width : 300; spacing: 2
                        Label { text: modelData.dateStr; font.pixelSize: 11; color: Kirigami.Theme.disabledTextColor }
                        Label { text: modelData.text; font.pixelSize: 13; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                    }
                }
                TextArea {
                    id: diaryInput
                    Layout.fillWidth: true
                    Layout.preferredHeight: 100
                    placeholderText: "What happened today?"
                    wrapMode: TextArea.Wrap
                }
                Button { text: "Save"; Layout.alignment: Qt.AlignRight; onClicked: addDiary() }
            }

            Kirigami.Separator { Layout.fillWidth: true }

            // ── Archive ──
            Rectangle {
                Layout.fillWidth: true; height: 44; radius: 10
                color: archiveOpen ? Kirigami.Theme.backgroundColor : Qt.rgba(0.5,0.5,0.5,0.08)
                border.width: 1; border.color: Qt.rgba(0.5,0.5,0.5,0.2)
                RowLayout {
                    anchors.fill: parent; anchors.margins: 12
                    Label { text: "📦"; font.pixelSize: 18 }
                    Label { text: "Skill Archive"; font.pixelSize: 15; font.bold: true; Layout.fillWidth: true }
                    Label { text: archiveOpen ? "▲" : "▼"; font.pixelSize: 12; color: Kirigami.Theme.disabledTextColor }
                }
                MouseArea { anchors.fill: parent; onClicked: archiveOpen = !archiveOpen }
            }
            ColumnLayout {
                spacing: 6; visible: archiveOpen
                Layout.fillWidth: true
                Label { text: "Select person then tap archived skill to restore."; color: Kirigami.Theme.disabledTextColor }
                ComboBox {
                    id: archivePerson
                    model: ["riff", "reef", "pitch", "rain"]
                    Layout.fillWidth: true
                    onCurrentTextChanged: refreshArchive()
                }
                Repeater {
                    id: archiveList
                    model: ListModel { id: archiveModel }
                    delegate: Rectangle {
                        width: parent ? parent.width : 300; height: 40; radius: 8
                        color: Kirigami.Theme.backgroundColor
                        RowLayout {
                            anchors.fill: parent; anchors.margins: 8
                            Label { text: modelData.name; font.pixelSize: 14; font.bold: true; Layout.fillWidth: true }
                            Label { text: "lvl " + modelData.level + " · " + modelData.xp + " XP"; font.pixelSize: 12; color: Kirigami.Theme.disabledTextColor }
                            Label {
                                text: "↩ Restore"; font.pixelSize: 12; color: Kirigami.Theme.highlightColor
                                MouseArea { anchors.fill: parent; onClicked: { Backend.skillReactivate(archivePerson.currentText, modelData.name); refreshArchive() } }
                            }
                        }
                    }
                }
            }

            Kirigami.Separator { Layout.fillWidth: true }

            // ── Shop ──
            Rectangle {
                Layout.fillWidth: true; height: 44; radius: 10
                color: shopOpen ? Kirigami.Theme.backgroundColor : Qt.rgba(0.5,0.5,0.5,0.08)
                border.width: 1; border.color: Qt.rgba(0.5,0.5,0.5,0.2)
                RowLayout {
                    anchors.fill: parent; anchors.margins: 12
                    Label { text: "🛒"; font.pixelSize: 18 }
                    Label { text: "Shop"; font.pixelSize: 15; font.bold: true; Layout.fillWidth: true }
                    Label { text: "💰 " + Backend.coins; font.pixelSize: 13; color: "#FFC107" }
                    Label { text: shopOpen ? "▲" : "▼"; font.pixelSize: 12; color: Kirigami.Theme.disabledTextColor }
                }
                MouseArea { anchors.fill: parent; onClicked: shopOpen = !shopOpen }
            }
            ColumnLayout {
                spacing: 6; visible: shopOpen
                Layout.fillWidth: true
                Label { id: shopMsg; visible: false; text: ""; font.pixelSize: 12; color: "#EF5350" }
                Repeater {
                    model: Backend.shopItems
                    delegate: Rectangle {
                        width: parent ? parent.width : 300; height: 70; radius: 10
                        color: Kirigami.Theme.backgroundColor
                        border.width: 1; border.color: Backend.coins >= modelData.cost ? Kirigami.Theme.highlightColor : Qt.rgba(0.5,0.5,0.5,0.2)
                        opacity: Backend.coins >= modelData.cost ? 1.0 : 0.5
                        RowLayout {
                            anchors.fill: parent; anchors.margins: 10; spacing: 10
                            ColumnLayout { Layout.fillWidth: true; spacing: 2
                                Label { text: modelData.name; font.pixelSize: 15; font.bold: true }
                                Label { text: modelData.description; font.pixelSize: 12; color: Kirigami.Theme.disabledTextColor }
                            }
                            ColumnLayout { spacing: 4; Layout.alignment: Qt.AlignVCenter
                                Label { text: modelData.cost + " 💰"; font.pixelSize: 16; font.bold: true; color: Backend.coins >= modelData.cost ? "#FFC107" : Kirigami.Theme.disabledTextColor }
                                Button {
                                    text: "Buy"; Layout.minimumWidth: 70; enabled: Backend.coins >= modelData.cost
                                    onClicked: {
                                        if (modelData.id === "custom_title") { buyDialog.itemId = modelData.id; buyDialog.open() }
                                        else if (!Backend.buyItem(modelData.id)) { shopMsg.text = "Not enough coins!"; shopMsg.visible = true }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Kirigami.Separator { Layout.fillWidth: true }

            // ── Birthdays ──
            Rectangle {
                Layout.fillWidth: true; height: 44; radius: 10
                color: bdaysOpen ? Kirigami.Theme.backgroundColor : Qt.rgba(0.5,0.5,0.5,0.08)
                border.width: 1; border.color: Qt.rgba(0.5,0.5,0.5,0.2)
                RowLayout {
                    anchors.fill: parent; anchors.margins: 12
                    Label { text: "🎂"; font.pixelSize: 18 }
                    Label { text: "Birthdays"; font.pixelSize: 15; font.bold: true; Layout.fillWidth: true }
                    Label { text: bdaysOpen ? "▲" : "▼"; font.pixelSize: 12; color: Kirigami.Theme.disabledTextColor }
                }
                MouseArea { anchors.fill: parent; onClicked: bdaysOpen = !bdaysOpen }
            }
            ColumnLayout {
                spacing: 6; visible: bdaysOpen
                Layout.fillWidth: true
                Repeater {
                    model: Backend.birthdays
                    delegate: Rectangle {
                        width: parent ? parent.width : 300; height: 44; radius: 8
                        color: modelData.isToday ? Qt.rgba(0.94, 0.2, 0.2, 0.15) : Kirigami.Theme.backgroundColor
                        border.width: modelData.isToday ? 2 : 1; border.color: modelData.isToday ? "#EF5350" : Qt.rgba(0.5,0.5,0.5,0.2)
                        RowLayout {
                            anchors.fill: parent; anchors.margins: 10; spacing: 8
                            Label {
                                text: modelData.isToday ? "🎂 TODAY!" : modelData.daysAway <= 3 ? "🔔 " + modelData.daysAway + "d" : modelData.daysAway <= 14 ? modelData.daysAway + "d" : ""
                                font.pixelSize: 13; font.bold: modelData.isToday
                                color: modelData.isToday ? "#EF5350" : modelData.daysAway <= 3 ? "#FFC107" : Kirigami.Theme.disabledTextColor
                            }
                            Label { text: modelData.name; font.pixelSize: 14; font.bold: true; Layout.fillWidth: true }
                            Label { text: modelData.date; font.pixelSize: 13; color: Kirigami.Theme.disabledTextColor }
                            Label {
                                text: "✕"; color: "#EF5350"; font.pixelSize: 14
                                MouseArea { anchors.fill: parent; onClicked: { delTarget.type = "birthday"; delTarget.index = index; delConfirm.open() } }
                            }
                        }
                    }
                }
                RowLayout { spacing: 8
                    TextField { id: bdayName; placeholderText: "Name"; Layout.fillWidth: true }
                    TextField {
                        id: bdayDate; placeholderText: "DD.MM"; Layout.preferredWidth: 90
                        onTextChanged: {
                            var raw = text.replace(/[^0-9]/g, "")
                            if (raw.length >= 4) { var f = raw.substring(0,2) + "." + raw.substring(2,4); if (text !== f) { text = f; cursorPosition = 5 } }
                        }
                        onAccepted: addBday()
                    }
                    Button { text: "Add"; onClicked: addBday() }
                }
            }

            Kirigami.Separator { Layout.fillWidth: true }

            // ── System ──
            Rectangle {
                Layout.fillWidth: true; height: 44; radius: 10
                color: systemOpen ? Kirigami.Theme.backgroundColor : Qt.rgba(0.5,0.5,0.5,0.08)
                border.width: 1; border.color: Qt.rgba(0.5,0.5,0.5,0.2)
                RowLayout {
                    anchors.fill: parent; anchors.margins: 12
                    Label { text: "💾"; font.pixelSize: 18 }
                    Label { text: "System"; font.pixelSize: 15; font.bold: true; Layout.fillWidth: true }
                    Label { text: systemOpen ? "▲" : "▼"; font.pixelSize: 12; color: Kirigami.Theme.disabledTextColor }
                }
                MouseArea { anchors.fill: parent; onClicked: systemOpen = !systemOpen }
            }
            ColumnLayout {
                spacing: 8; visible: systemOpen
                Layout.fillWidth: true
                Rectangle {
                    Layout.fillWidth: true; height: 48; radius: 10
                    color: Kirigami.Theme.backgroundColor
                    border.width: 1; border.color: Qt.rgba(0.5,0.5,0.5,0.3)
                    RowLayout { anchors.fill: parent; anchors.margins: 12
                        Label { text: "Save State"; font.pixelSize: 15; Layout.fillWidth: true }
                        Label { text: "Save"; color: Kirigami.Theme.highlightColor }
                    }
                    MouseArea { anchors.fill: parent; onClicked: Backend.saveState() }
                }

                RowLayout {
                    Label { text: "Theme:"; font.pixelSize: 14 }
                    ComboBox {
                        id: themePicker
                        model: ["System", "Light", "Dark"]
                        currentIndex: Backend.theme
                        onActivated: Backend.setTheme(currentIndex)
                    }
                }

                RowLayout {
                    Label { text: "Wallpaper:"; font.pixelSize: 14 }
                    TextField { id: wallpaperPath; placeholderText: "Path..."; Layout.fillWidth: true; text: Backend.wallpaper; font.pixelSize: 12 }
                    Button { text: "Set"; flat: true; onClicked: Backend.setWallpaper(wallpaperPath.text) }
                    Button { text: "Clear"; flat: true; visible: Backend.wallpaper.length > 0; onClicked: { Backend.clearWallpaper(); wallpaperPath.text = "" } }
                }

                Rectangle {
                    Layout.fillWidth: true; height: 48; radius: 10
                    color: Kirigami.Theme.backgroundColor
                    border.width: 1; border.color: Qt.rgba(0.5,0.5,0.5,0.3)
                    RowLayout { anchors.fill: parent; anchors.margins: 12
                        Label { text: "📊 Export CSV"; font.pixelSize: 15; Layout.fillWidth: true }
                        Label { text: "Export"; color: Kirigami.Theme.highlightColor }
                    }
                    MouseArea { anchors.fill: parent; onClicked: { var p = Backend.exportCsv(); exportLabel.text = "Exported to " + p; exportLabel.visible = true } }
                }
                Label { id: exportLabel; visible: false; text: ""; font.pixelSize: 11; color: Kirigami.Theme.positiveTextColor }
                Label { text: "Tomogichi v0.5.0 — Qt Quick + Kirigami"; font.pixelSize: 11; color: Kirigami.Theme.disabledTextColor; Layout.alignment: Qt.AlignHCenter }
            }

            Item { Layout.fillHeight: true }
        }
    }

    Component.onCompleted: refreshArchive()

    property var delTarget: ({ type: "", id: 0, index: 0 })

    function addTask() {
        if (newTaskInput.text.trim()) { Backend.taskAdd(newTaskInput.text.trim()); newTaskInput.text = "" }
    }
    function addDiary() {
        if (diaryInput.text.trim()) { Backend.diaryAdd(diaryInput.text.trim()); diaryInput.text = "" }
    }
    function refreshArchive() {
        archiveModel.clear()
        var skills = Backend.archivedSkills(archivePerson.currentText)
        for (var i = 0; i < skills.length; i++) {
            archiveModel.append(skills[i])
        }
    }

    function addBday() {
        if (bdayName.text.trim() && bdayDate.text.trim()) {
            Backend.birthdayAdd(bdayName.text.trim(), bdayDate.text.trim())
            bdayName.text = ""
            bdayDate.text = ""
        }
    }

    // Challenge skill models — filtered by person
    ListModel {
        id: chalSkillAll
        Component.onCompleted: {
            append({n:"any"}); append({n:"drawing"}); append({n:"sewing"}); append({n:"lefthand"})
            append({n:"3d-model"}); append({n:"linux"}); append({n:"anatomy"})
            append({n:"dance"}); append({n:"driving"}); append({n:"kendama"})
            append({n:"meditation"}); append({n:"ferment"}); append({n:"massage"})
        }
    }
    ListModel { id: chalSkillRiff;  Component.onCompleted: { append({n:"any"}); append({n:"drawing"}); append({n:"sewing"}); append({n:"lefthand"}) } }
    ListModel { id: chalSkillReef;  Component.onCompleted: { append({n:"any"}); append({n:"3d-model"}); append({n:"linux"}); append({n:"anatomy"}) } }
    ListModel { id: chalSkillPitch; Component.onCompleted: { append({n:"any"}); append({n:"dance"}); append({n:"driving"}); append({n:"kendama"}) } }
    ListModel { id: chalSkillRain;  Component.onCompleted: { append({n:"any"}); append({n:"meditation"}); append({n:"ferment"}); append({n:"massage"}) } }

    function updateChalSkills() {
        var p = chalPerson.currentText
        var m
        if (p === "riff")  m = chalSkillRiff
        else if (p === "reef")  m = chalSkillReef
        else if (p === "pitch") m = chalSkillPitch
        else if (p === "rain")  m = chalSkillRain
        else m = chalSkillAll
        chalSkill.model = m
        chalSkill.currentIndex = 0
    }

    Dialog {
        id: addChalDialog
        title: "Add Challenge"
        ColumnLayout { spacing: 8; width: 300
            TextField { id: chalDesc; placeholderText: "Description"; Layout.fillWidth: true }
            ComboBox {
                id: chalPerson
                model: ["any", "riff", "reef", "pitch", "rain"]
                Layout.fillWidth: true
                onCurrentTextChanged: updateChalSkills()
            }
            ComboBox {
                id: chalSkill
                model: chalSkillAll
                textRole: "n"
                Layout.fillWidth: true
            }
            RowLayout {
                TextField { id: chalCount; placeholderText: "count"; Layout.preferredWidth: 70; inputMethodHints: Qt.ImhDigitsOnly }
                TextField { id: chalDays; placeholderText: "days"; Layout.preferredWidth: 70; inputMethodHints: Qt.ImhDigitsOnly }
                TextField { id: chalReward; placeholderText: "coins"; Layout.preferredWidth: 70; inputMethodHints: Qt.ImhDigitsOnly }
            }
        }
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: {
            var pid = chalPerson.currentText === "any" ? "" : chalPerson.currentText
            var sk = chalSkill.currentText === "any" ? "" : chalSkill.currentText
            Backend.challengeAdd(chalDesc.text.trim(), pid, sk,
                                 parseInt(chalCount.text) || 7, parseInt(chalDays.text) || 7, parseInt(chalReward.text) || 50)
            chalDesc.text = ""
            chalCount.text = ""; chalDays.text = ""; chalReward.text = ""
        }
    }

    Dialog {
        id: buyDialog
        title: "Custom Title"
        property string itemId: ""
        ColumnLayout { spacing: 8; width: 280
            Label { text: "Choose character:" }
            ComboBox {
                id: buyPerson
                model: ["riff", "reef", "pitch", "rain"]
                Layout.fillWidth: true
            }
            Label { text: "Title:" }
            TextField {
                id: buyTitleInput
                placeholderText: "e.g. Grandmaster"
                Layout.fillWidth: true
            }
        }
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: {
            Backend.buyCustomTitle(buyPerson.currentText, buyTitleInput.text || "Custom")
            buyTitleInput.text = ""
        }
    }

    Dialog {
        id: delConfirm
        title: "Delete?"
        Label { text: "Are you sure?"; font.pixelSize: 14 }
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: {
            if (delTarget.type === "task") Backend.taskDelete(delTarget.id)
            if (delTarget.type === "challenge") Backend.challengeDelete(delTarget.index)
            if (delTarget.type === "birthday") Backend.birthdayDelete(delTarget.index)
        }
    }
}
// force update
