import QtQuick 2.0
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material
import Map
import Location

Item {
    id: root
    required property MainWindow map
    Material.theme: map.nightMode

    RoundButton {
        id: positionMode

        anchors.right: parent.right
        y: parent.height / 2
        icon.source: "qrc:/navig64/routing.png"
        icon.color: "transparent"
        Material.background: Material.theme === Material.Dark ? "#CC3C3F44" : "#CBFFFFFF"
        flat: true
        onClicked: root.map.myPositionAction.trigger()

        Connections {
            target: root.map
            function onPositionModeChanged(mode) {
                positionMode.state = mode;
                console.log("Position Mode changed to", mode);
            }
            function onInfoChanged() {
                pane.visible = root.height > root.width + 50;
            }
        }

        states: [
            State {
                name: PositionMode.PendingPosition
                PropertyChanges {
                    positionMode.icon.source: "qrc:/navig64/location-search.png"
                }
            },
            State {
                name: PositionMode.NotFollowNoPosition
                extend: PositionMode.PendingPosition
            },
            State {
                name: PositionMode.NotFollow
                PropertyChanges {
                    positionMode.icon.source: "qrc:/navig64/location.png"
                }
            },
            State {
                name: PositionMode.Follow
                PropertyChanges {
                    restoreEntryValues: false
                    positionMode.rotation: 30
                }
            },
            State {
                name: PositionMode.FollowAndRotate
                PropertyChanges {
                    restoreEntryValues: false
                    positionMode.rotation: 0
                }
            }
        ]
    }

    component GridLabel: Text {
        // Positioner.index doesn't work here
        visible: parent.children[parent.children.indexOf(this) + 1].text
    }

    component LinkedText: Text {
        onLinkActivated: link => Qt.openUrlExternally(link)
    }

    Pane {
        id: pane

        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        visible: false
        topPadding: 0

        Column {
            id: column

            width: Math.min(implicitWidth, parent.width)

            Row {
                Column {
                    id: innerColumn

                    Text {
                        text: root.map.title
                        font.bold: true
                    }

                    Text {
                        text: root.map.subTitle
                    }

                    Text {
                        text: root.map.address
                    }
                }

                Item {
                    implicitWidth: close.implicitWidth
                    implicitHeight: close.implicitHeight

                    Item {
                        implicitWidth: column.implicitWidth - innerColumn.implicitWidth + pane.rightPadding

                        ToolButton {
                            id: close

                            anchors.right: parent.right
                            text: "\u2715"

                            onClicked: {pane.visible = false; root.map.DeactivateMapSelection()}
                        }
                    }
                }
            }

            // Rectangle wrapper is needed so the separator shrinks
            Item {
                implicitWidth: 1
                implicitHeight: 1
                visible: parent.implicitWidth

                Rectangle {
                    implicitWidth: parent.parent.implicitWidth

                    implicitHeight: 1
                    color: "#1E000000"
                }
            }

            Row {
                spacing: 10

                Text {
                    text: root.map.wikipedia ? "<a href='" + root.map.wikipedia + "'>Wikipedia</a>" : ""
                    onLinkActivated: link => Qt.openUrlExternally(link)
                }

                Text {
                    text: root.map.wikimedia ? "<a href='" + root.map.wikimedia + "'>Wikimedia Commons</a>" : ""
                    onLinkActivated: link => Qt.openUrlExternally(link)
                }
            }

            Text {
                text: root.map.description
                wrapMode: Text.Wrap
                width: Math.min(implicitWidth, root.width - pane.leftPadding - pane.rightPadding)
            }

            Grid {
                columns: 2
                columnSpacing: 5

                GridLabel {
                    text: "Bookmark:"
                }
                Text {
                    text: root.map.bookmark ? "Yes" : ""
                }

                GridLabel {
                    text: "Opening hours:"
                }
                Text {
                    text: root.map.openingHours
                }

                GridLabel {
                    text: "Cuisine:"
                }
                Text {
                    text: root.map.cuisines
                }

                GridLabel {
                    text: "Phone:"
                }
                LinkedText {
                    text: root.map.phone ? "<a href='tel:" + root.map.phone + "'>" + root.map.phone + "</a>" : ""
                }

                GridLabel {
                    text: "Operator:"
                }
                Text {
                    text: root.map.operator
                }

                GridLabel {
                    text: "Wi-Fi:"
                }
                Text {
                    text: root.map.wifi ? "Yes" : ""
                }

                GridLabel {
                    text: "Website:"
                }
                LinkedText {
                    text: root.map.website ? "<a href='" + root.map.website + "'>" + root.map.website + "</a>" : ""
                }

                GridLabel {
                    text: "Email:"
                }
                LinkedText {
                    text: root.map.email ? "<a href='mailto:" + root.map.email + "'>" + root.map.email + "</a>" : ""
                }

                GridLabel {
                    text: "Facebook:"
                }
                LinkedText {
                    text: root.map.facebook
                }

                GridLabel {
                    text: "Instagram:"
                }
                LinkedText {
                    text: root.map.instagram
                }

                GridLabel {
                    text: "Twitter:"
                }
                LinkedText {
                    text: root.map.twitter
                }

                GridLabel {
                    text: "VK:"
                }
                LinkedText {
                    text: root.map.vk
                }

                GridLabel {
                    text: "Line:"
                }
                LinkedText {
                    text: root.map.line
                }

                GridLabel {
                    text: "Level:"
                }
                Text {
                    text: root.map.level
                }

                GridLabel {
                    text: "ATM:"
                }
                Text {
                    text: root.map.atm ? "Yes" : ""
                }
            }

            Row {
                anchors.horizontalCenter: parent.horizontalCenter

                ToolButton {
                    ToolTip.text: qsTr("Route From")
                    ToolTip.visible: hovered
                    onClicked: {root.map.fromAction.trigger(); pane.visible = false}
                    icon.source: "qrc:/navig64/point-start.png"
                    icon.color: "transparent"
                }
                ToolButton {
                    ToolTip.text: qsTr("Add Stop")
                    ToolTip.visible: hovered
                    onClicked: {root.map.stopAction.trigger(); pane.visible = false}
                    icon.source: "qrc:/navig64/point-intermediate.png"
                    icon.color: "transparent"
                }
                ToolButton {
                    ToolTip.text: qsTr("Route To")
                    ToolTip.visible: hovered
                    onClicked: {root.map.toAction.trigger(); pane.visible = false}
                    icon.source: "qrc:/navig64/point-finish.png"
                    icon.color: "transparent"
                }
                ToolButton {
                    text: qsTr("Route Along Track")
                    onClicked: {root.map.alongAction.trigger(); pane.visible = false}
                    visible: root.map.alongAction.visible
                }
                ToolButton {
                    text: qsTr("Edit Place")
                    onClicked: {root.map.editAction.trigger(); pane.visible = false}
                    visible: root.map.editAction.visible
                }
            }
        }
    }
}
