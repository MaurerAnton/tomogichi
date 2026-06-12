import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.Page {
    id: diaryPage
    title: "Diary"

    ListView {
        id: diaryList
        anchors.fill: parent
        anchors.margins: 12
        spacing: 4
        model: Backend.allDiaryEntries

        section.property: "dateKey"
        section.criteria: ViewSection.FullString
        section.delegate: Rectangle {
            width: diaryList.width
            height: 32
            color: Kirigami.Theme.backgroundColor
            Label {
                anchors.centerIn: parent
                text: section
                font.pixelSize: 13
                font.bold: true
                color: Kirigami.Theme.highlightColor
            }
        }

        delegate: ColumnLayout {
            width: diaryList.width
            spacing: 4

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: entryCol.implicitHeight + 16
                radius: 10
                color: Kirigami.Theme.backgroundColor
                border.width: 1
                border.color: Qt.rgba(0.5, 0.5, 0.5, 0.15)

                ColumnLayout {
                    id: entryCol
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 4

                    Label {
                        text: modelData.text
                        font.pixelSize: 14
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                    Label {
                        text: modelData.dateStr
                        font.pixelSize: 11
                        color: Kirigami.Theme.disabledTextColor
                    }

                    // Show comments
                    Repeater {
                        model: modelData.comments
                        delegate: RowLayout {
                            spacing: 4
                            Label {
                                text: "↳ " + modelData.text
                                font.pixelSize: 12
                                color: Qt.rgba(0.6, 0.6, 0.6, 0.9)
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                            Label {
                                text: modelData.timeStr
                                font.pixelSize: 10
                                color: Kirigami.Theme.disabledTextColor
                            }
                        }
                    }

                    // Comment input
                    Rectangle {
                        Layout.fillWidth: true
                        height: commentInput.active ? 36 : 0
                        visible: commentInput.active
                        color: Qt.rgba(0.3, 0.5, 1.0, 0.08)
                        radius: 8
                        TextField {
                            id: commentInput
                            anchors.fill: parent
                            anchors.margins: 4
                            placeholderText: "Add comment..."
                            font.pixelSize: 12
                            onAccepted: {
                                if (text.trim()) {
                                    Backend.diaryAddComment(modelData.index, text.trim())
                                    text = ""
                                    active = false
                                }
                            }
                            property bool active: false
                        }
                    }

                    Label {
                        text: modelData.comments.length > 0 ? "💬 " + modelData.comments.length : "💬"
                        font.pixelSize: 12
                        color: Kirigami.Theme.highlightColor
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                commentInput.active = !commentInput.active
                                if (commentInput.active) commentInput.forceActiveFocus()
                            }
                        }
                    }
                }
            }
        }
    }
}
