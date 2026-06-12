import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.Page {
    id: todayPage
    title: "Today"
    property int todayDow: new Date().getDay()
    property int viewMonth: new Date().getMonth() + 1
    property int viewYear: new Date().getFullYear()
    property int todayDay: new Date().getDate()
    property int todayMonth: viewMonth
    property int todayYear: viewYear

    function refreshHeatmap() {
        heatmapRepeater.model = Backend.monthlyHeatmapFor(viewYear, viewMonth)
    }

    Component.onCompleted: refreshHeatmap()

    ScrollView {
        anchors.fill: parent

        ColumnLayout {
            width: parent.width
            anchors.margins: 16
            spacing: 12

            // Month navigation
            RowLayout {
                Layout.fillWidth: true
                Button {
                    text: "←"
                    flat: true
                    font.pixelSize: 18
                    onClicked: {
                        if (viewMonth === 1) { viewMonth = 12; viewYear-- }
                        else viewMonth--
                        refreshHeatmap()
                    }
                }
                Kirigami.Heading {
                    text: monthName() + " " + viewYear
                    level: 3
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                }
                Button {
                    text: "→"
                    flat: true
                    font.pixelSize: 18
                    onClicked: {
                        if (viewMonth === 12) { viewMonth = 1; viewYear++ }
                        else viewMonth++
                        refreshHeatmap()
                    }
                }
                Button {
                    text: "Today"
                    flat: true
                    font.pixelSize: 12
                    visible: !(viewMonth === todayMonth && viewYear === todayYear)
                    onClicked: {
                        viewMonth = todayMonth
                        viewYear = todayYear
                        refreshHeatmap()
                    }
                }
            }

            // Calendar heatmap grid
            GridLayout {
                columns: 7
                Layout.fillWidth: true

                Repeater {
                    model: ["M","T","W","T","F","S","S"]
                    Label {
                        text: modelData; font.pixelSize: 11; font.bold: true
                        color: Kirigami.Theme.disabledTextColor
                        Layout.preferredWidth: 28; horizontalAlignment: Text.AlignHCenter
                    }
                }

                // Offset cells for first day (Monday=0)
                Repeater {
                    id: offsetRepeater
                    model: {
                        var first = heatmapRepeater.count > 0 ? heatmapRepeater.model[0].dow : 0
                        // Convert Sunday=0 to Monday=0 system
                        return first === 0 ? 6 : first - 1
                    }
                    delegate: Item {
                        Layout.preferredWidth: 28
                        Layout.preferredHeight: 1
                    }
                }

                Repeater {
                    id: heatmapRepeater
                    model: []
                    delegate: Rectangle {
                        Layout.preferredWidth: 28; Layout.preferredHeight: 28; radius: 4
                        color: modelData.isToday ? Kirigami.Theme.highlightColor :
                               modelData.level === 3 ? "#2E7D32" :
                               modelData.level === 2 ? "#4CAF50" :
                               modelData.level === 1 ? "#81C784" : Qt.rgba(0.5,0.5,0.5,0.1)

                        Label {
                            anchors.centerIn: parent; text: modelData.day; font.pixelSize: 10
                            color: modelData.level > 0 || modelData.isToday ? "white" : Kirigami.Theme.disabledTextColor
                            font.bold: modelData.isToday
                        }
                        // Diary dot (orange, bottom-center)
                        Rectangle {
                            anchors.bottom: parent.bottom; anchors.bottomMargin: 2
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: 7; height: 7; radius: 3
                            color: "#FF9800"
                            visible: modelData.hasDiary
                        }
                        // Schedule dot (blue, bottom-left)
                        Rectangle {
                            anchors.bottom: parent.bottom; anchors.bottomMargin: 2
                            anchors.left: parent.left; anchors.leftMargin: 2
                            width: 7; height: 7; radius: 3
                            color: "#2196F3"
                            visible: modelData.hasSchedule
                        }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                var tip = modelData.day + " " + monthName() + ": " + modelData.minutes + " min"
                                if (modelData.hasDiary) tip += "\n📓 " + modelData.diaryText.substring(0, 40) + "..."
                                if (modelData.hasSchedule) tip += "\n📋 Schedules on this day"
                                dayTooltip.text = tip
                                dayTooltip.visible = true
                                calDayDialog.dayNum = modelData.day
                                calDayDialog.dayLabel = modelData.day + " " + monthName()
                                calDayDialog.open()
                            }
                        }
                    }
                }
            }

            Label {
                id: dayTooltip
                visible: false
                font.pixelSize: 11
                color: Kirigami.Theme.disabledTextColor
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            RowLayout {
                spacing: 4
                Repeater {
                    model: [{c:Qt.rgba(0.5,0.5,0.5,0.1),t:"0"},{c:"#81C784",t:"<30m"},{c:"#4CAF50",t:"<90m"},{c:"#2E7D32",t:"90m+"},{c:"#FF9800",t:"diary"},{c:"#2196F3",t:"sched"}]
                    delegate: RowLayout { spacing: 2
                        Rectangle { width:12; height:12; radius:2; color: modelData.c }
                        Label { text: modelData.t; font.pixelSize: 9; color: Kirigami.Theme.disabledTextColor }
                    }
                }
            }

            Kirigami.Separator { Layout.fillWidth: true }
            Kirigami.Heading { text: "📋 Schedules"; level: 3 }

            // Schedule list
            Repeater {
                model: Backend.calendarSlots
                delegate: RowLayout {
                    spacing: 6
                    Layout.fillWidth: true
                    Rectangle {
                        width: 6; height: 24; radius: 3
                        color: modelData.date ? "#2196F3" :
                               modelData.dayOfWeek === todayDow ? Kirigami.Theme.highlightColor : Qt.rgba(0.5,0.5,0.5,0.3)
                    }
                    Label {
                        text: modelData.desc
                        font.pixelSize: 13
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                    Label {
                        text: "✎"
                        font.pixelSize: 14
                        color: Kirigami.Theme.highlightColor
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                editDialog.editIndex = modelData.index
                                editDialog.oldLabel = modelData.label
                                editDialog.oldHour = modelData.hour
                                editDialog.oldMin = modelData.minute
                                editDialog.oldDate = modelData.date || ""
                                editDayCombo.currentIndex = modelData.dayOfWeek
                                editDialog.open()
                            }
                        }
                    }
                    Label {
                        text: "✕"
                        font.pixelSize: 14
                        color: "#EF5350"
                        MouseArea {
                            anchors.fill: parent
                            onClicked: Backend.calendarDelete(modelData.index)
                        }
                    }
                }
            }

            // Add schedule button
            Button {
                text: "+ Add Schedule"
                Layout.fillWidth: true
                flat: true
                onClicked: calDialog.open()
            }

            Kirigami.Separator { Layout.fillWidth: true }
            Kirigami.Heading { text: "☑ Daily Tasks"; level: 3 }

            Label {
                text: getTodoDone() + "/" + Backend.dailyTodos.length + " done"
                font.pixelSize: 12
                color: Kirigami.Theme.disabledTextColor
                visible: Backend.dailyTodos.length > 0
            }

            Repeater {
                model: Backend.dailyTodos
                delegate: RowLayout {
                    spacing: 10
                    CheckBox { checked: modelData.done; onClicked: Backend.dailyTodoToggle(index) }
                    Label { text: modelData.text; font.pixelSize: 14; color: modelData.done ? Kirigami.Theme.disabledTextColor : Kirigami.Theme.textColor }
                    Label { text: "✕"; color: "#EF5350"; font.pixelSize: 14; MouseArea { anchors.fill: parent; onClicked: Backend.dailyTodoDelete(index) } }
                }
            }
            RowLayout { spacing: 8
                TextField { id: newTodoInput; placeholderText: "New daily task..."; Layout.fillWidth: true; onAccepted: addTodo() }
                Button { text: "Add"; onClicked: addTodo() }
            }

            Kirigami.Separator { Layout.fillWidth: true }

            // Upcoming birthdays
            Kirigami.Heading {
                text: "🎂 Upcoming Birthdays"
                level: 3
                visible: hasUpcomingBirthdays()
            }

            Repeater {
                model: Backend.birthdays
                delegate: Label {
                    visible: modelData.daysAway <= 14
                    text: (modelData.isToday ? "🎂 TODAY: " : "🔔 ") + modelData.name + " — " + modelData.date +
                          (modelData.isToday ? "" : " (in " + modelData.daysAway + " days)")
                    font.pixelSize: 13
                    color: modelData.isToday ? "#EF5350" :
                           modelData.daysAway <= 3 ? "#FFC107" : Kirigami.Theme.disabledTextColor
                    font.bold: modelData.isToday
                }
            }

            Kirigami.Separator { Layout.fillWidth: true }
            Kirigami.Heading { text: "💡 Recommendation"; level: 3 }
            Label { text: getRec(); font.pixelSize: 14; wrapMode: Text.WordWrap; Layout.fillWidth: true }
            Item { Layout.fillHeight: true }
        }
    }

    function addTodo() {
        if (newTodoInput.text.trim()) { Backend.dailyTodoAdd(newTodoInput.text.trim()); newTodoInput.text = "" }
    }
    function pad2(n) { return ("0" + n).slice(-2) }
    function monthName() {
        var m = ["January","February","March","April","May","June","July","August","September","October","November","December"]
        return m[viewMonth - 1]
    }
    function getRec() {
        var s = Backend.todaySummary()
        if (s.neglectedName) return "Focus: " + s.neglectedName + " " + (s.neglectedSkill||"") + " (" + (s.neglectedDays||0) + "d idle)"
        return "All good. Keep the rhythm!"
    }

    function hasUpcomingBirthdays() {
        var bdays = Backend.birthdays
        for (var i = 0; i < bdays.length; i++) {
            if (bdays[i].daysAway <= 14) return true
        }
        return false
    }

    function getTodoDone() {
        var todos = Backend.dailyTodos
        var done = 0
        for (var i = 0; i < todos.length; i++) {
            if (todos[i].done) done++
        }
        return done
    }

    // Dialog: Add Schedule (generic, day-of-week recurring)
    Dialog {
        id: calDialog
        title: "Add Schedule"
        ColumnLayout {
            spacing: 8
            width: 280
            RowLayout {
                Label { text: "Day:" }
                ComboBox {
                    id: calDay
                    model: ["Sun","Mon","Tue","Wed","Thu","Fri","Sat"]
                    currentIndex: todayDow
                }
            }
            RowLayout {
                Label { text: "Time:" }
                TextField { id: calHour; placeholderText: "HH"; Layout.preferredWidth: 50; inputMethodHints: Qt.ImhDigitsOnly }
                Label { text: ":" }
                TextField { id: calMin; placeholderText: "MM"; Layout.preferredWidth: 50; inputMethodHints: Qt.ImhDigitsOnly }
            }
            TextField { id: calLabel; placeholderText: "Label"; Layout.fillWidth: true }
        }
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: {
            var h = parseInt(calHour.text) || 9
            var m = parseInt(calMin.text) || 0
            Backend.calendarAdd(calDay.currentIndex, h, m, calLabel.text || "Event")
            calHour.text = ""
            calMin.text = ""
            calLabel.text = ""
        }
    }

    // Dialog: Add Schedule for specific day (from calendar tap)
    Dialog {
        id: calDayDialog
        title: "Add Schedule"
        property int dayNum: 1
        property string dayLabel: ""
        ColumnLayout {
            spacing: 8
            width: 280
            Label {
                text: calDayDialog.dayLabel
                font.pixelSize: 16
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
            }
            RowLayout {
                Label { text: "Time:" }
                TextField { id: calDayHour; placeholderText: "HH"; Layout.preferredWidth: 50; inputMethodHints: Qt.ImhDigitsOnly }
                Label { text: ":" }
                TextField { id: calDayMin; placeholderText: "MM"; Layout.preferredWidth: 50; inputMethodHints: Qt.ImhDigitsOnly }
            }
            TextField { id: calDayLabel; placeholderText: "Label"; Layout.fillWidth: true }
        }
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: {
            var h = parseInt(calDayHour.text) || 9
            var m = parseInt(calDayMin.text) || 0
            Backend.calendarAddDate(calDayDialog.dayNum, viewMonth, h, m, calDayLabel.text || "Event")
            calDayHour.text = ""
            calDayMin.text = ""
            calDayLabel.text = ""
        }
    }

    // Dialog: Edit Schedule
    Dialog {
        id: editDialog
        title: "Edit Schedule"
        property int editIndex: 0
        property string oldLabel: ""
        property int oldHour: 9
        property int oldMin: 0
        property string oldDate: ""
        ColumnLayout {
            spacing: 8
            width: 280
            RowLayout {
                Label { text: "Day:" }
                ComboBox {
                    id: editDayCombo
                    model: ["Sun","Mon","Tue","Wed","Thu","Fri","Sat"]
                }
            }
            RowLayout {
                Label { text: "Time:" }
                TextField { id: editHour; placeholderText: "HH"; Layout.preferredWidth: 50; text: editDialog.oldHour; inputMethodHints: Qt.ImhDigitsOnly }
                Label { text: ":" }
                TextField { id: editMin; placeholderText: "MM"; Layout.preferredWidth: 50; text: editDialog.oldMin; inputMethodHints: Qt.ImhDigitsOnly }
            }
            TextField { id: editLabel; placeholderText: "Label"; Layout.fillWidth: true; text: editDialog.oldLabel }
        }
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: {
            var h = parseInt(editHour.text) || 9
            var m = parseInt(editMin.text) || 0
            Backend.calendarEdit(editDialog.editIndex, editDayCombo.currentIndex, h, m, editLabel.text || "Event", editDialog.oldDate)
        }
    }
}
