import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.Page {
    id: todayPage
    title: "Today"
    property int todayDow: new Date().getDay()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        Kirigami.Heading { text: "📅 " + monthName(); level: 3 }

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

            // Offset cells for first day
            Repeater {
                model: Backend.monthlyHeatmap.length > 0 ? Backend.monthlyHeatmap[0].dow : 0
                delegate: Item {
                    Layout.preferredWidth: 28
                    Layout.preferredHeight: 1
                }
            }

            Repeater {
                model: Backend.monthlyHeatmap
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
                    MouseArea { anchors.fill: parent; onClicked: { dayTooltip.text = modelData.day + ": " + modelData.minutes + " min"; dayTooltip.visible = true } }
                }
            }
        }

        Label { id: dayTooltip; visible: false; font.pixelSize: 11; color: Kirigami.Theme.disabledTextColor }

        RowLayout {
            spacing: 4
            Repeater {
                model: [{c:Qt.rgba(0.5,0.5,0.5,0.1),t:"0"},{c:"#81C784",t:"<30m"},{c:"#4CAF50",t:"<90m"},{c:"#2E7D32",t:"90m+"}]
                delegate: RowLayout { spacing: 2
                    Rectangle { width:12; height:12; radius:2; color: modelData.c }
                    Label { text: modelData.t; font.pixelSize: 9; color: Kirigami.Theme.disabledTextColor }
                }
            }
        }

        Kirigami.Separator { Layout.fillWidth: true }
        Kirigami.Heading { text: "Schedule"; level: 3 }

        Repeater {
            model: Backend.calendarSlots
            delegate: RowLayout {
                visible: modelData.dayOfWeek === todayDow
                spacing: 8
                Rectangle { width: 4; height: 24; radius: 2; color: Kirigami.Theme.highlightColor }
                Label { text: pad2(modelData.hour) + ":" + pad2(modelData.minute); font.pixelSize: 14; font.bold: true; font.family: "monospace" }
                Label { text: modelData.label; font.pixelSize: 14 }
            }
        }

        Button { text: "+ Add Schedule"; Layout.fillWidth: true; flat: true; onClicked: calDialog.open() }

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

    function addTodo() {
        if (newTodoInput.text.trim()) { Backend.dailyTodoAdd(newTodoInput.text.trim()); newTodoInput.text = "" }
    }
    function pad2(n) { return ("0" + n).slice(-2) }
    function monthName() {
        var m = ["January","February","March","April","May","June","July","August","September","October","November","December"]
        return m[new Date().getMonth()]
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
}
