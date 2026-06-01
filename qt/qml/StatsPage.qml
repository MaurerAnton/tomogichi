import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.Page {
    id: statsPage
    title: "Stats"

    property int activeTab: 0

    Component.onCompleted: console.log("Stats loaded")

    Connections {
        target: Backend
        function onStatsChanged() { console.log("stats changed") }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        RowLayout {
            Repeater {
                model: ["Skills", "Time", "Chains", "History", "Week"]
                delegate: Rectangle {
                    Layout.fillWidth: true
                    height: 36
                    radius: 8
                    color: index === activeTab ? Kirigami.Theme.highlightColor : "transparent"
                    opacity: index === activeTab ? 0.2 : 0.05
                    Label {
                        anchors.centerIn: parent
                        text: modelData
                        font.pixelSize: 13
                        font.bold: index === activeTab
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: activeTab = index
                    }
                }
            }
        }

        Kirigami.Separator { Layout.fillWidth: true }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: activeTab

            // Tab 0: Skills
            ScrollView {
                ColumnLayout {
                    width: parent.width
                    spacing: 8
                    Repeater {
                        model: Backend.skillEffectStats
                        delegate: Rectangle {
                            width: parent ? parent.width : 300
                            height: 44
                            radius: 8
                            color: Kirigami.Theme.backgroundColor
                            border.width: 1
                            border.color: Qt.rgba(0,0,0,0.1)
                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 8
                                Label { text: modelData.personName; font.bold: true }
                                Label { text: modelData.skillName; color: Kirigami.Theme.disabledTextColor }
                                Item { Layout.fillWidth: true }
                                Label { text: "🔋" + modelData.energized; color: "#4CAF50"; font.pixelSize: 11 }
                                Label { text: "😐" + modelData.neutral; color: "#FFC107"; font.pixelSize: 11 }
                                Label { text: "😮‍💨" + modelData.tired; color: "#FF5722"; font.pixelSize: 11 }
                                Label { text: "🪫" + modelData.drained; color: "#9E9E9E"; font.pixelSize: 11 }
                            }
                        }
                    }
                    Label {
                        visible: Backend.skillEffectStats.length === 0
                        text: "No practice data yet. Start practicing to see effects."
                        color: Kirigami.Theme.disabledTextColor
                        font.pixelSize: 13
                    }
                }
            }

            // Tab 1: Time of day
            ScrollView {
                ColumnLayout {
                    width: parent.width
                    spacing: 8
                    Repeater {
                        model: Backend.timeEffectStats
                        delegate: Rectangle {
                            width: parent ? parent.width : 300
                            height: 44
                            radius: 8
                            color: Kirigami.Theme.backgroundColor
                            border.width: 1
                            border.color: Qt.rgba(0,0,0,0.1)
                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 4
                                Label {
                                    text: ["Morning","Afternoon","Evening","Night"][modelData.bucket] + " (" + modelData.total + ")"
                                    font.bold: true
                                }
                                RowLayout {
                                    spacing: 6
                                    Label { text: "🔋" + modelData.energized; color: "#4CAF50"; font.pixelSize: 11 }
                                    Label { text: "😐" + modelData.neutral; color: "#FFC107"; font.pixelSize: 11 }
                                    Label { text: "😮‍💨" + modelData.tired; color: "#FF5722"; font.pixelSize: 11 }
                                    Label { text: "🪫" + modelData.drained; color: "#9E9E9E"; font.pixelSize: 11 }
                                }
                            }
                        }
                    }
                }

                Label {
                    visible: Backend.timeEffectStats.length === 0 || !hasTimeData()
                    text: "No time-of-day data yet."
                    color: Kirigami.Theme.disabledTextColor
                    font.pixelSize: 13
                }
            }

            // Tab 2: Chains
            ScrollView {
                ColumnLayout {
                    width: parent.width
                    spacing: 6
                    Repeater {
                        model: Backend.activityChains
                        delegate: Rectangle {
                            width: parent ? parent.width : 300
                            height: 40
                            radius: 8
                            color: Kirigami.Theme.backgroundColor
                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 8
                                Label {
                                    text: modelData.beforeSkill + " → " + modelData.afterSkill
                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                }
                                Rectangle {
                                    width: modelData.energizedPct
                                    height: 20; radius: 4
                                    color: "#4CAF50"
                                    Label {
                                        anchors.centerIn: parent
                                        text: modelData.energizedPct + "%"
                                        font.pixelSize: 10; color: "white"
                                    }
                                }
                                Label {
                                    text: "(" + modelData.sampleCount + "×)"
                                    font.pixelSize: 11
                                    color: Kirigami.Theme.disabledTextColor
                                }
                            }
                        }
                    }
                    Label {
                        visible: Backend.activityChains.length === 0
                        text: "Practice multiple skills in one day to see patterns.\nE.g. 'drawing → meditation' shows how you felt."
                        color: Kirigami.Theme.disabledTextColor
                    }
                }
            }

            // Tab 3: History
            ScrollView {
                ListView {
                    model: Backend.practiceHistory
                    spacing: 4
                    delegate: Rectangle {
                        width: ListView.view ? ListView.view.width : 300
                        height: 36
                        radius: 6
                        color: Kirigami.Theme.backgroundColor
                        opacity: 0.8

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 6
                            spacing: 8

                            Label {
                                text: modelData.dateStr
                                font.pixelSize: 11
                                font.family: "monospace"
                                color: Kirigami.Theme.disabledTextColor
                            }
                            Label {
                                text: modelData.personId
                                font.pixelSize: 13
                                font.bold: true
                            }
                            Label {
                                text: modelData.skillName
                                font.pixelSize: 12
                                color: Kirigami.Theme.disabledTextColor
                            }
                            Item { Layout.fillWidth: true }
                            Label {
                                text: modelData.minutes + "m"
                                font.pixelSize: 12
                                font.bold: true
                            }
                            Label {
                                text: modelData.effect === "energized" ? "🔋" :
                                      modelData.effect === "tired" ? "😮‍💨" :
                                      modelData.effect === "drained" ? "🪫" : "😐"
                                font.pixelSize: 14
                            }
                            Label {
                                visible: modelData.notes.length > 0
                                text: "📝"
                                font.pixelSize: 12
                                color: Kirigami.Theme.disabledTextColor
                            }
                        }
                    }
                    Label {
                        visible: Backend.practiceHistory.length === 0
                        text: "No practice sessions yet."
                        color: Kirigami.Theme.disabledTextColor
                    }
                }
            }

            // Tab 4: Week — time distribution
            ScrollView {
                ColumnLayout {
                    width: parent ? parent.width : 300
                    spacing: 12

                    Kirigami.Heading { text: "This Week — Time Distribution"; level: 3 }

                    // Horizontal bar chart
                    Repeater {
                        model: Backend.weeklyPie
                        delegate: ColumnLayout {
                            width: parent ? parent.width : 300
                            spacing: 4

                            RowLayout {
                                Label { text: modelData.personName; font.pixelSize: 14; font.bold: true; Layout.preferredWidth: 60 }
                                Rectangle {
                                    Layout.fillWidth: true; height: 22; radius: 4
                                    color: Qt.rgba(0.5,0.5,0.5,0.15)
                                    Rectangle {
                                        width: parent.width * modelData.percent / 100
                                        height: parent.height; radius: 4
                                        color: modelData.color
                                    }
                                }
                                Label {
                                    text: modelData.percent.toFixed(0) + "%"
                                    font.pixelSize: 13; font.bold: true
                                    Layout.preferredWidth: 40
                                    color: modelData.color
                                }
                            }

                            Label {
                                text: Math.floor(modelData.minutes / 60) + "h " + (modelData.minutes % 60) + "m"
                                font.pixelSize: 12
                                color: Kirigami.Theme.disabledTextColor
                                Layout.leftMargin: 60
                            }
                        }
                    }

                    Label {
                        visible: Backend.weeklyPie.length === 0
                        text: "No practice this week."
                        color: Kirigami.Theme.disabledTextColor
                    }

                    Kirigami.Separator { Layout.fillWidth: true }

                    // Totals
                    RowLayout {
                        Label { text: "Total this week:"; font.pixelSize: 14; Layout.fillWidth: true }
                        Label {
                            text: totalWeekHours() + "h"
                            font.pixelSize: 20; font.bold: true
                            color: Kirigami.Theme.highlightColor
                        }
                    }
                }
            }
        }
    }

    function totalWeekHours() {
        var pie = Backend.weeklyPie
        var total = 0
        for (var i = 0; i < pie.length; i++) total += pie[i].minutes
        return (total / 60).toFixed(1)
    }

    function hasTimeData() {
        var stats = Backend.timeEffectStats
        for (var i = 0; i < stats.length; i++) {
            if (stats[i].total > 0) return true
        }
        return false
    }
}
