import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.Page {
    background: Rectangle { color: Backend.getColor("bg") }
    leftPadding: 12
    rightPadding: 12
    topPadding: 8
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
            color: Backend.getColor("bg")
            Label {
                anchors.centerIn: parent
                text: section
                font.pixelSize: 13
                font.bold: true
                color: Backend.getColor("highlight")
            }
        }

        delegate: ColumnLayout {
            width: diaryList.width
            spacing: 4

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: entryCol.implicitHeight + 16
                radius: 10
                color: Backend.getColor("bg")
                border.width: 1
                border.color: Backend.getColor("border")

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
                        color: Backend.getColor("subText")
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
                                color: Backend.getColor("subText")
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
                        color: Backend.getColor("highlight")
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
