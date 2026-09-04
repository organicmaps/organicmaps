// Source - https://stackoverflow.com/a/65082990
// Posted by Leo
// Retrieved 2026-06-30, License - CC BY-SA 4.0

import QtQuick 2.0
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material
import Map
import Location

Item {
    id: root
    required property MainWindow map

    Canvas {
        id: cloud

        anchors.right: parent.right
        y: parent.height / 2
        width: 400
        height: 300
        rotation: 30
        property var ctx: null
        property var fillStyle: "#8ED6FF"

        onPaint: {
            if (!ctx)
                ctx = getContext("2d");
            var x = 100;
            var y = 170;
            ctx.beginPath();
            ctx.arc(x, y, 60, Math.PI * 0.5, Math.PI * 1.5);
            ctx.arc(x + 70, y - 60, 70, Math.PI * 1, Math.PI * 1.85);
            ctx.arc(x + 152, y - 45, 50, Math.PI * 1.37, Math.PI * 1.91);
            ctx.arc(x + 200, y, 60, Math.PI * 1.5, Math.PI * 0.5);
            ctx.moveTo(x + 200, y + 60);
            ctx.lineTo(x, y + 60);
            ctx.strokeStyle = "#797874";
            ctx.stroke();
            ctx.fillStyle = fillStyle;
            ctx.fill();
        }

        MouseArea {
            anchors.fill: parent

            onPressed: mouse => {
                const inside = cloud.ctx.isPointInPath(mouse.x, mouse.y);
                mouse.accepted = inside;
                if (inside)
                    console.log("Cloud pressed");
                else
                    console.log("MouseArea pressed");
            }
            onReleased: mouse => {
                const inside = cloud.ctx.isPointInPath(mouse.x, mouse.y);
                mouse.accepted = inside;
                if (inside)
                    console.log("Cloud released");
                else
                    console.log("MouseArea released");
            }
            onClicked: {
                root.map.getMyPositionAction().trigger();
                console.log("Cloud clicked");
            }
        }

        Connections {
            target: root.map
            function onPositionModeChanged(mode) {
                cloud.state = mode;
                cloud.requestPaint();
                console.log("Position Mode changed to " + mode);
            }
        }

        states: [
            State {
                name: PositionMode.PendingPosition
                PropertyChanges {
                    cloud.fillStyle: undefined
                    text.text: "Pending Position..."
                }
            },
            State {
                name: PositionMode.NotFollowNoPosition
                extend: PositionMode.PendingPosition
                PropertyChanges {
                    text.text: "No Position"
                }
            },
            State {
                name: PositionMode.NotFollow
                PropertyChanges {
                    cloud.fillStyle: cloud.context.strokeStyle
                    text.text: "Not Follow"
                }
            },
            State {
                name: PositionMode.Follow
                PropertyChanges {
                    restoreEntryValues: false
                    cloud.rotation: 30
                    text.text: "Follow"
                }
            },
            State {
                name: PositionMode.FollowAndRotate
                PropertyChanges {
                    restoreEntryValues: false
                    cloud.rotation: 0
                    text.text: "Follow and Rotate"
                }
            }
        ]

        Text {
            id: text

            font.family: "Helvetica"
            font.pointSize: 24
            color: "white"

            anchors.centerIn: parent
        }
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
        visible: innerColumn.implicitHeight
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
                    implicitWidth: 1
                    implicitHeight: close.implicitHeight

                    Item {
                        implicitWidth: column.implicitWidth - innerColumn.implicitWidth

                        ToolButton {
                            id: close

                            anchors.right: parent.right
                            text: "\u2715"
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
                    text: qsTr("Route From")
                }
                ToolButton {
                    text: qsTr("Add Stop")
                }
                ToolButton {
                    text: qsTr("Route To")
                }
                Button {
                    text: qsTr("Edit Place")
                }
            }
        }
    }
}
