import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.Page {
    id: personPage
    title: personName

    property string personId: ""
    property string personName: ""
    property string personRole: ""
    property int personLevel: 0
    property string personTitle: ""
    property string personMood: ""

    ListModel { id: skillModel }

    Component.onCompleted: refreshSkills()

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

    Connections {
        target: Backend
        function onPersonsChanged() { refreshSkills() }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        RowLayout {
            Label { text: personMood; font.pixelSize: 36 }
            ColumnLayout {
                spacing: 2
                Kirigami.Heading { text: personName; level: 2 }
                Label {
                    text: personRole + " · Lvl " + personLevel + " " + personTitle + " · " + getTotalXP() + " XP"
                    font.pixelSize: 14
                    color: Kirigami.Theme.disabledTextColor
                }
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
                    color: Kirigami.Theme.backgroundColor
                    border.width: 1
                    border.color: model.isMain ? Kirigami.Theme.highlightColor : Qt.rgba(0.5, 0.5, 0.5, 0.3)

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
                                Label { text: "lvl " + model.level; font.pixelSize: 13; color: Kirigami.Theme.positiveTextColor }
                            }

                            Rectangle {
                                Layout.fillWidth: true; height: 6; radius: 3
                                color: Qt.rgba(0.5, 0.5, 0.5, 0.3)
                                Rectangle {
                                    width: parent.width * model.progressPct / 100; height: parent.height; radius: 3
                                    color: model.warmth === "warm" ? "#4CAF50" : model.warmth === "cool" ? "#FFC107" : "#9E9E9E"
                                }
                            }

                            Label {
                                text: model.daysSince === 0 ? "today" : model.daysSince === 1 ? "1 day ago" : model.daysSince + " days ago"
                                font.pixelSize: 11
                                color: model.warmth === "cold" ? "#EF5350" : Kirigami.Theme.disabledTextColor
                            }
                        }

                        ColumnLayout {
                            spacing: 2
                            Layout.alignment: Qt.AlignVCenter

                            // Start timer (▶)
                            Label {
                                text: "▶"
                                font.pixelSize: 22; color: Kirigami.Theme.highlightColor
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
                                font.pixelSize: 14; color: Kirigami.Theme.disabledTextColor
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
                                font.pixelSize: 14; color: "#EF5350"
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
                color: Kirigami.Theme.backgroundColor; opacity: 0.6
                RowLayout {
                    anchors.fill: parent; anchors.margins: 6
                    Label { text: modelData.name; font.pixelSize: 13; color: Kirigami.Theme.disabledTextColor; Layout.fillWidth: true }
                    Label { text: "lvl " + modelData.level + " · " + modelData.xp + " XP"; font.pixelSize: 11; color: Kirigami.Theme.disabledTextColor }
                    Label {
                        text: "↩"; font.pixelSize: 16; color: Kirigami.Theme.highlightColor
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

        Component.onCompleted: { refreshSkills(); refreshArchive() }
    }

    Component { id: timerComponent; TimerPage {} }

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
