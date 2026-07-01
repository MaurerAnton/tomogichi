import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.Page {
    background: Rectangle { color: Backend.getColor("bg") }
    leftPadding: 12
    rightPadding: 12
    topPadding: 8
    id: personPage
    title: personName

    property string personId: ""
    property string personName: ""
    property string personRole: ""
    property int personLevel: 0
    property string personTitle: ""
    property string personMood: ""

    ListModel { id: skillModel }

    function refreshSkills() {
        skillModel.clear()
        var persons = Backend.persons
        for (var i = 0; i < persons.length; i++) {
            if (persons[i].id !== personId) continue
            var skills = persons[i].skills
            for (var j = 0; j < skills.length; j++) {
                var s = skills[j]
                skillModel.append({
                    name: s.name, level: s.level, xp: s.xp,
                    progressPct: s.progressPct, isMain: s.isMain,
                    daysSince: s.daysSince, warmth: s.warmth
                })
            }
            break
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        RowLayout {
            Label { text: personMood; font.pixelSize: 36 }
            ColumnLayout {
                spacing: 2
                Kirigami.Heading {
                    text: personName; level: 2
                    MouseArea {
                        anchors.fill: parent
                        onClicked: renamePersonDialog.open()
                    }
                }
                Label {
                    text: personRole + " · Lvl " + personLevel + " " + personTitle + " · " + getTotalXP() + " XP"
                    font.pixelSize: 14
                    color: Backend.getColor("subText")
                }
            }
            Item { Layout.fillWidth: true }
            Label {
                text: "✎"; font.pixelSize: 18; color: Backend.getColor("highlight")
                MouseArea { anchors.fill: parent; onClicked: renamePersonDialog.open() }
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                model: skillModel
                spacing: 8

                delegate: Rectangle {
                    width: ListView.view ? ListView.view.width : 300
                    height: 72
                    radius: 10
                    color: Backend.getColor("bg")
                    border.width: 1
                    border.color: model.isMain ? Backend.getColor("highlight") : Backend.getColor("border")

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            Backend.startTimer(personId, model.name, 30)
                            applicationWindow().pageStack.push(timerComponent, {
                                personId: personId, personName: personName,
                                skillName: model.name
                            })
                        }
                        onPressAndHold: {
                            if (!model.isMain) {
                                archiveDialog.skillName = model.name
                                archiveDialog.open()
                            }
                        }
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 10

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            RowLayout {
                                Label { text: model.name + (model.isMain ? " ★" : ""); font.pixelSize: 15; font.bold: model.isMain }
                                Item { Layout.fillWidth: true }
                                Label { text: "lvl " + model.level; font.pixelSize: 13; color: Backend.getColor("accent") }
                            }

                            Rectangle {
                                Layout.fillWidth: true; height: 6; radius: 3
                                color: Backend.getColor("border")
                                Rectangle {
                                    width: parent.width * model.progressPct / 100; height: parent.height; radius: 3
                                    color: model.warmth === "warm" ? Backend.getColor("accent") : model.warmth === "cool" ? Backend.getColor("highlight") : Backend.getColor("subText")
                                }
                            }

                            Label {
                                text: model.daysSince === 0 ? "today" : model.daysSince === 1 ? "1 day ago" : model.daysSince + " days ago"
                                font.pixelSize: 11
                                visible: model.daysSince >= 0
                                color: model.warmth === "cold" ? Backend.getColor("danger") : Backend.getColor("subText")
                            }
                        }

                        ColumnLayout {
                            spacing: 2
                            Layout.alignment: Qt.AlignVCenter

                            // Start timer (▶)
                            Label {
                                text: "▶"
                                font.pixelSize: 22; color: Backend.getColor("highlight")
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        Backend.startTimer(personId, model.name, 30)
                                        applicationWindow().pageStack.push(timerComponent, {
                                            personId: personId, personName: personName,
                                            skillName: model.name
                                        })
                                    }
                                }
                            }

                            // Rename (✎)
                            Label {
                                text: "✎"
                                font.pixelSize: 14; color: Backend.getColor("subText")
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        renameDialog.oldName = model.name
                                        renameDialog.newName = model.name
                                        renameDialog.open()
                                    }
                                }
                            }

                            // Delete (✕)
                            Label {
                                text: "✕"
                                font.pixelSize: 14; color: Backend.getColor("danger")
                                visible: skillModel.count > 1 || !model.isMain
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        delSkillDialog.skillName = model.name
                                        delSkillDialog.isMainSkill = model.isMain
                                        delSkillDialog.open()
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // Add skill button
        Button {
            text: "+ Add Skill"
            Layout.fillWidth: true
            onClicked: addSkillDialog.open()
        }

        // Archived skills
        Kirigami.Heading {
            text: "📦 Archived"
            level: 3
            visible: archiveModel.count > 0
        }
        Repeater {
            model: archiveModel
            delegate: Rectangle {
                width: parent ? parent.width : 300; height: 36; radius: 6
                color: Backend.getColor("bg"); opacity: 0.6
                RowLayout {
                    anchors.fill: parent; anchors.margins: 6
                    Label { text: modelData.name; font.pixelSize: 13; color: Backend.getColor("subText"); Layout.fillWidth: true }
                    Label { text: "lvl " + modelData.level + " · " + modelData.xp + " XP"; font.pixelSize: 11; color: Backend.getColor("subText") }
                    Label {
                        text: "↩"; font.pixelSize: 16; color: Backend.getColor("highlight")
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                Backend.skillReactivate(personId, modelData.name)
                                refreshArchive()
                                refreshSkills()
                            }
                        }
                    }
                }
            }
        }

        // Archive model
        ListModel { id: archiveModel }

    function refreshArchive() {
        archiveModel.clear()
        var skills = Backend.archivedSkills(personId)
        for (var i = 0; i < skills.length; i++) archiveModel.append(skills[i])
    }

    function getTotalXP() {
        var total = 0
        for (var i = 0; i < skillModel.count; i++) {
            total += skillModel.get(i).xp
        }
        return total
    }

        // Character checklist section
        Kirigami.Separator { Layout.fillWidth: true; Layout.topMargin: 12 }

        RowLayout {
            Kirigami.Heading { text: "☑ Checklist"; level: 3; Layout.fillWidth: true }
            Label {
                text: "+"; font.pixelSize: 22; color: Backend.getColor("highlight")
                MouseArea { anchors.fill: parent; onClicked: addChecklistDialog.open() }
            }
        }

        Repeater {
            model: checklistModel
            delegate: RowLayout {
                width: parent ? parent.width : 300; spacing: 8
                CheckBox {
                    checked: modelData.done
                    onClicked: {
                        Backend.charChecklistToggle(modelData.index)
                        refreshChecklist()
                    }
                }
                ColumnLayout {
                    spacing: 0; Layout.fillWidth: true
                    Label {
                        text: modelData.text; font.pixelSize: 13
                        color: modelData.done ? Backend.getColor("subText") : Backend.getColor("text")
                    }
                    Label {
                        text: modelData.repeatHours > 0 ?
                              (modelData.done ? "↻ repeats in " + modelData.repeatInHours + "h" : "↻ every " + modelData.repeatHours + "h") : ""
                        font.pixelSize: 10; color: Backend.getColor("highlight")
                        visible: modelData.repeatHours > 0
                    }
                }
                Label {
                    text: "⏱"; font.pixelSize: 14; color: Backend.getColor("highlight")
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            repeatDialog.itemIndex = modelData.index
                            repeatDialog.itemText = modelData.text
                            repeatDialog.open()
                        }
                    }
                }
                Label {
                    text: "✕"; color: Backend.getColor("danger"); font.pixelSize: 14
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            Backend.charChecklistDelete(modelData.index)
                            refreshChecklist()
                        }
                    }
                }
            }
        }

        ListModel { id: checklistModel }

        function refreshChecklist() {
            checklistModel.clear()
            var items = Backend.charChecklists
            for (var i = 0; i < items.length; i++) {
                if (items[i].personId === personId) {
                    checklistModel.append(items[i])
                }
            }
        }

        Component.onCompleted: { refreshSkills(); refreshArchive(); refreshChecklist() }
    }

    Connections {
        target: Backend
        function onCharChecklistsChanged() { refreshChecklist() }
        function onPersonsChanged() { refreshSkills(); refreshArchive() }
    }

    Component { id: timerComponent; TimerPage {} }

    // Rename person dialog
    Dialog {
        id: renamePersonDialog
        title: "Rename " + personName
        ColumnLayout { spacing: 8; width: 280
            Label { text: "New name:" }
            TextField {
                id: renamePersonInput
                Layout.fillWidth: true
                text: personName
                onAccepted: doRename()
            }
        }
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: doRename()
        function doRename() {
            if (renamePersonInput.text.trim() && renamePersonInput.text.trim() !== personName) {
                Backend.renamePerson(personId, renamePersonInput.text.trim())
            }
        }
    }

    // Add checklist item dialog
    Dialog {
        id: addChecklistDialog
        title: "Add Checklist Item"
        ColumnLayout { spacing: 8; width: 280
            Label { text: "Task for " + personName + ":" }
            TextField {
                id: addChecklistInput
                Layout.fillWidth: true
                placeholderText: "e.g. Stretch for 10 min"
                onAccepted: doAdd()
            }
        }
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: doAdd()
        function doAdd() {
            if (addChecklistInput.text.trim()) {
                Backend.charChecklistAdd(personId, addChecklistInput.text.trim())
                addChecklistInput.text = ""
            }
        }
    }

    // Repeat interval dialog
    Dialog {
        id: repeatDialog
        title: "Set Repeat Interval"
        property int itemIndex: -1
        property string itemText: ""
        ColumnLayout { spacing: 8; width: 280
            Label { text: "Repeat '" + repeatDialog.itemText + "' every:"; wrapMode: Text.WordWrap }
            RowLayout {
                spacing: 8
                TextField {
                    id: repeatHoursInput
                    placeholderText: "hours"
                    Layout.fillWidth: true
                    inputMethodHints: Qt.ImhDigitsOnly
                    onAccepted: doSet()
                }
                Label { text: "hours (0 = no repeat)"; font.pixelSize: 11; color: Backend.getColor("subText") }
            }
        }
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: doSet()
        function doSet() {
            var h = parseInt(repeatHoursInput.text) || 0
            Backend.charChecklistRepeat(repeatDialog.itemIndex, h)
            repeatHoursInput.text = ""
        }
    }

    // Archive confirmation dialog
    Dialog {
        id: archiveDialog
        title: "Archive Skill"
        property string skillName: ""
        Label { text: "Archive '" + archiveDialog.skillName + "'? XP will be preserved."; wrapMode: Text.WordWrap }
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: {
            Backend.skillArchive(personId, archiveDialog.skillName)
            archiveDialog.close()
        }
    }

    // Add skill dialog
    Dialog {
        id: addSkillDialog
        title: "Add Skill for " + personName
        ColumnLayout {
            spacing: 10
            TextField {
                id: newSkillName
                placeholderText: "skill name"
                Layout.fillWidth: true
            }
            CheckBox {
                id: newSkillMain
                text: "Main skill (★)"
            }
        }
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: {
            if (newSkillName.text.trim()) {
                Backend.skillAdd(personId, newSkillName.text.trim(), newSkillMain.checked)
                newSkillName.text = ""
                newSkillMain.checked = false
            }
        }
    }

    // Rename skill dialog
    Dialog {
        id: renameDialog
        title: "Rename Skill"
        property string oldName: ""
        property alias newName: renameInput.text
        ColumnLayout { spacing: 8; width: 280
            Label { text: "New name:" }
            TextField {
                id: renameInput
                Layout.fillWidth: true
                text: renameDialog.oldName
            }
        }
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: {
            if (renameInput.text.trim() && renameInput.text.trim() !== oldName) {
                Backend.skillRename(personId, oldName, renameInput.text.trim())
            }
        }
    }

    // Delete skill confirmation dialog
    Dialog {
        id: delSkillDialog
        title: "Delete Skill"
        property string skillName: ""
        property bool isMainSkill: false
        Label {
            text: delSkillDialog.isMainSkill ?
                  "Delete main skill '" + delSkillDialog.skillName + "'?\nXP will be lost!" :
                  "Delete '" + delSkillDialog.skillName + "'?\nXP will be lost!"
            wrapMode: Text.WordWrap
        }
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: {
            Backend.skillDelete(personId, delSkillDialog.skillName)
        }
    }
}
