import QtQuick 2.6
import Sailfish.Silica 1.0
import app.organicmaps 1.0
import "notices.js" as Toast

Dialog {
    id: dialog

    // The opening_hours value, changed when accepted.
    property string value
    property bool advanced
    // Days in the order of the week in the locale; osmoh numbers them from Sunday as 1.
    readonly property var weekDays: {
        var days = []
        for (var i = 0; i < 7; ++i)
            days.push((Qt.locale().firstDayOfWeek + i) % 7 + 1)
        return days
    }

    function formatTime(minutes) {
        return Format.formatDate(new Date(2000, 0, 1, Math.floor(minutes / 60) % 24, minutes % 60), Formatter.TimeValue)
    }
    function pickTime(minutes, done) {
        var picker = pageStack.push("Sailfish.Silica.TimePickerDialog",
                                    { hour: Math.floor(minutes / 60) % 24, minute: minutes % 60 })
        picker.accepted.connect(function() { done(picker.hour * 60 + picker.minute) })
    }

    allowedOrientations: Orientation.All
    canAccept: !advanced || hours.isValid(textArea.text)
    onAccepted: value = advanced ? textArea.text.trim() : hours.value
    Component.onCompleted: {
        hours.value = value
        advanced = !hours.simple
        textArea.text = value
    }

    OpeningHoursEditor {
        id: hours
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        PullDownMenu {
            MenuItem {
                visible: dialog.advanced
                text: appInfo.localized("editor_example_values")
                onClicked: pageStack.push(Qt.resolvedUrl("HtmlPage.qml"),
                                          { title: text, file: "opening_hours_how_to_edit.html" })
            }
            MenuItem {
                text: appInfo.localized(dialog.advanced ? "editor_time_simple" : "editor_time_advanced")
                onClicked: {
                    if (dialog.advanced) {
                        hours.value = textArea.text
                        if (hours.simple)
                            dialog.advanced = false
                        else
                            Toast.show(appInfo.localized("editor_correct_mistake"))
                    } else {
                        textArea.text = hours.value
                        dialog.advanced = true
                    }
                }
            }
        }

        Column {
            id: column
            width: parent.width

            DialogHeader {
                title: appInfo.localized("editor_time_title")
            }

            Column {
                width: parent.width
                visible: dialog.advanced

                TextArea {
                    id: textArea
                    width: parent.width
                    label: hours.isValid(text) ? appInfo.localized("editor_time_title")
                                               : appInfo.localized("editor_correct_mistake")
                    placeholderText: "Mo-Fr 09:00-18:00"
                    errorHighlight: !hours.isValid(text)
                    inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
                }
                SectionHeader {
                    text: appInfo.localized("editor_example_values")
                }
                PageLabel {
                    color: Theme.secondaryHighlightColor
                    font.pixelSize: Theme.fontSizeSmall
                    text: "24/7\nMo-Fr 08:00-20:00; Sa 10:00-16:00\n"
                          + "Mo-Fr 09:00-13:00,14:00-18:00\nMo-Su 10:00-22:00; Tu off"
                }
            }

            Repeater {
                model: dialog.advanced ? [] : hours.timetables

                Column {
                    id: card

                    readonly property int timetableIndex: index
                    readonly property var timetable: modelData

                    width: column.width

                    SectionHeader {
                        visible: hours.timetables.length > 1
                        text: appInfo.localized("editor_time_title") + " " + (index + 1)
                    }
                    // A day taken here leaves the other schedules.
                    Row {
                        x: Theme.horizontalPageMargin
                        width: parent.width - 2 * x
                        height: Theme.itemSizeSmall

                        Repeater {
                            model: dialog.weekDays

                            BackgroundItem {
                                readonly property bool selected: card.timetable.days.indexOf(modelData) >= 0

                                width: parent.width / 7
                                height: parent.height
                                highlighted: down || selected
                                onClicked: hours.setDay(card.timetableIndex, modelData, !selected)

                                Label {
                                    anchors.centerIn: parent
                                    text: hours.dayName(modelData)
                                    color: parent.selected ? Theme.highlightColor : Theme.primaryColor
                                    font.bold: parent.selected
                                }
                            }
                        }
                    }
                    TextSwitch {
                        text: appInfo.localized("editor_time_allday")
                        automaticCheck: false
                        checked: card.timetable.allDay
                        onClicked: hours.setAllDay(card.timetableIndex, !checked)
                    }
                    Row {
                        width: parent.width
                        visible: !card.timetable.allDay

                        ValueButton {
                            width: parent.width / 2
                            label: appInfo.localized("editor_time_from")
                            value: dialog.formatTime(card.timetable.open)
                            onClicked: dialog.pickTime(card.timetable.open, function(minutes) {
                                hours.setOpeningTime(card.timetableIndex, minutes, card.timetable.close)
                            })
                        }
                        ValueButton {
                            width: parent.width / 2
                            label: appInfo.localized("editor_time_to")
                            value: dialog.formatTime(card.timetable.close)
                            onClicked: dialog.pickTime(card.timetable.close, function(minutes) {
                                hours.setOpeningTime(card.timetableIndex, card.timetable.open, minutes)
                            })
                        }
                    }
                    // Non-business hours, e.g. a lunch break.
                    Repeater {
                        model: card.timetable.allDay ? [] : card.timetable.closed

                        Row {
                            width: card.width

                            ValueButton {
                                width: (parent.width - removeClosed.width) / 2
                                label: appInfo.localized("editor_hours_closed")
                                value: dialog.formatTime(modelData.start)
                                onClicked: {
                                    var closedIndex = index
                                    var end = modelData.end
                                    dialog.pickTime(modelData.start, function(minutes) {
                                        hours.setClosed(card.timetableIndex, closedIndex, minutes, end)
                                    })
                                }
                            }
                            ValueButton {
                                width: (parent.width - removeClosed.width) / 2
                                label: appInfo.localized("editor_time_to")
                                value: dialog.formatTime(modelData.end)
                                onClicked: {
                                    var closedIndex = index
                                    var start = modelData.start
                                    dialog.pickTime(modelData.end, function(minutes) {
                                        hours.setClosed(card.timetableIndex, closedIndex, start, minutes)
                                    })
                                }
                            }
                            IconButton {
                                id: removeClosed
                                anchors.verticalCenter: parent.verticalCenter
                                icon.source: "image://theme/icon-m-remove"
                                onClicked: hours.removeClosed(card.timetableIndex, index)
                            }
                        }
                    }
                    Button {
                        anchors.horizontalCenter: parent.horizontalCenter
                        visible: card.timetable.canAddClosed
                        text: appInfo.localized("editor_time_add_closed")
                        onClicked: hours.addClosed(card.timetableIndex)
                    }
                    Item {
                        width: 1
                        height: Theme.paddingMedium
                    }
                    Button {
                        anchors.horizontalCenter: parent.horizontalCenter
                        visible: hours.timetables.length > 1
                        text: appInfo.localized("editor_time_delete")
                        onClicked: hours.removeTimetable(card.timetableIndex)
                    }
                }
            }

            Item {
                width: 1
                height: Theme.paddingLarge
            }
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                visible: !dialog.advanced && hours.canAddTimetable
                text: appInfo.localized("editor_time_add")
                onClicked: hours.addTimetable()
            }
        }

        VerticalScrollDecorator {}
    }
}
