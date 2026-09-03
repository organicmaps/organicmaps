// Source - https://stackoverflow.com/a/65082990
// Posted by Leo
// Retrieved 2026-06-30, License - CC BY-SA 4.0

import QtQuick 2.0
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

    component LabeledText: RowLayout {
        property alias label: label_.text
        property alias value: text_.text

        Text {
            id: label_
            Layout.preferredWidth: parent.parent.maxLabelWidth
            visible: text
        }
        Text {
            id: text_
            textFormat: Text.RichText
            onLinkActivated: link => Qt.openUrlExternally(link)
        }
        visible: value

        Component.onCompleted: parent.maxLabelWidth = Math.max(parent.maxLabelWidth, label_.implicitWidth)
    }
        Column {
            property real maxLabelWidth
            LabeledText {
                label: "Bookmark:"
                value: root.map.bookmark ? "Yes" : ""
            }

            LabeledText {
                label: "Opening hours:"
                value: root.map.openingHours
            }

            LabeledText {
                label: "Cuisine:"
                value: root.map.cuisines
            }

            LabeledText {
                label: "Phone:"
                value: root.map.phone ? "<a href='tel:" + root.map.phone + "'>" + root.map.phone + "</a>" : ""
            }

            LabeledText {
                label: "Operator:"
                value: root.map.operator
            }

            LabeledText {
                label: "Wi-Fi:"
                value: root.map.wifi ? "Yes" : ""
            }

            LabeledText {
                label: "Website:"
                value: root.map.website ? "<a href='" + root.map.website + "'>" + root.map.website + "</a>" : ""
            }

            LabeledText {
                label: "Email:"
                value: root.map.email ? "<a href='mailto:" + root.map.email + "'>" + root.map.email + "</a>" : ""
            }

            LabeledText {
                label: "Facebook:"
                value: root.map.facebook
            }

            LabeledText {
                label: "Instagram:"
                value: root.map.instagram
            }

            LabeledText {
                label: "Twitter:"
                value: root.map.twitter
            }

            LabeledText {
                label: "VK:"
                value: root.map.vk
            }

            LabeledText {
                label: "Line:"
                value: root.map.line
            }

            LabeledText {
                label: "Level:"
                value: root.map.level
            }

            LabeledText {
                label: "ATM:"
                value: root.map.atm ? "Yes" : ""
            }

}
