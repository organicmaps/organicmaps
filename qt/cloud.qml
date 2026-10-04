import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import Map
import Location

Item {
    id: root
    required property MainWindow map
    Material.theme: map.nightMode

    component RoundOverayButton: RoundButton {
        Material.background: Material.theme === Material.Dark ? "#CC3C3F44" : "#CBFFFFFF"
        icon.color: Qt.rgba(Material.foreground.r, Material.foreground.g, Material.foreground.b,
                            0.541176)
        flat: true
    }

    component OverayButton: RoundOverayButton {
        radius: 10
    }

    Popup {
        id: layersPopup
        modal: true

        Material.theme: parent.Material.theme
        Material.accent: parent.Material.accent
        Material.primary: parent.Material.primary

        contentWidth: layersRow.width
        contentHeight: layersLabel.implicitHeight + layersRow.implicitHeight

        Label {
            id: layersLabel
            text: "Map Styles and Layers"
        }

        RowLayout {
            id: layersRow
            anchors.top: layersLabel.bottom
            property int totalChecked

            width: root.height / root.width < 16 / 9 ?
                       Math.min(implicitWidth, root.width - 2 * layersPopup.padding) :
                       root.width - 2 * layersPopup.padding

            Repeater {
                model: [
                    { source: "qrc:/layer/subway", caption: "Subway", action: root.map.transitAction },
                    { source: "qrc:/layer/contours", caption: "Contours", action: root.map.isolinesAction },
                    { source: "qrc:/layer/outdoor_lake", caption: "Outdoors", action: root.map.outdoorsAction },
                    { source: "qrc:/layer/outdoor_river_r2", caption: "Hiking", action: root.map.hikingAction },
                    { source: "qrc:/layer/outdoor_river", caption: "Cycling", action: root.map.cyclingAction }
                ]

                delegate: ItemDelegate {
                    required property var modelData

                    checked: modelData.action.checked
                    onCheckedChanged: layersRow.totalChecked += checked ? 1 : -1
                    onClicked: modelData.action.trigger()

                    Layout.fillWidth: true
                    horizontalPadding: 5

                    indicator: Rectangle {
                        id: indicator
                        parent: image
                        visible: parent.parent.parent.checked

                        color: "transparent"
                        border.color: Material.color(Material.LightBlue)
                        border.width: 2
                        radius: image.width * 6 / 64 + border.width

                        anchors.fill: parent
                        anchors.margins: -border.width
                        anchors.rightMargin: anchors.margins + 1
                        anchors.bottomMargin: anchors.rightMargin
                    }

                    contentItem: ColumnLayout {
                        Image {
                            id: image
                            source: modelData.source
                            fillMode: Image.PreserveAspectFit
                            Layout.fillWidth: true
                            Layout.preferredHeight: width
                            Layout.preferredWidth: 70
                            sourceSize.width: width
                        }

                        Label {
                            text: modelData.caption
                            // TODO: use better font/size from Pure Maps
                            font.pixelSize: 10
                            Layout.topMargin: indicator.border.width
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }
                }
            }
        }
    }

    RoundOverayButton {
        y: height / 2
        icon.source: "qrc:/navig64/layers.png"
        icon.color: layersRow.totalChecked ? Material.color(Material.LightBlue) : undefined
        onClicked: layersPopup.open()
    }

    Row {
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 10
        padding: spacing

        OverayButton {
            icon.source: "qrc:/ui/logo.png"
            icon.color: "transparent"
            onClicked: root.map.OnAbout()
        }
        OverayButton {
            icon.source: "qrc:/navig64/search.png"
            icon.color: "transparent"
            onClicked: root.map.searchAction.trigger()
        }
        OverayButton {
            icon.source: "qrc:/navig64/bookmark.png"
            icon.color: "transparent"
            onClicked: root.map.OnBookmarksAction()
        }
        OverayButton {
            icon.name: "application-menu"
            onClicked: root.map.toggleWidgetsUI()
        }
    }

    Column {
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter

        RoundOverayButton {
            icon.name: "list-add-symbolic"
            icon.width: 32
            icon.height: 32
            onClicked: root.map.scalePlus()
        }
        RoundOverayButton {
            icon.name: "list-remove-symbolic"
            icon.width: 32
            icon.height: 32
            onClicked: root.map.scaleMinus()
        }

        Item {
            height: positionMode.height / 2
            width: 1
        }

        RoundOverayButton {
            id: positionMode

            icon.source: "qrc:/navig64/routing.png"
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
                    PropertyChanges {
                        positionMode.icon.color: "transparent"
                    }
                },
                State {
                    name: PositionMode.FollowAndRotate
                    extend: PositionMode.Follow
                    PropertyChanges {
                        restoreEntryValues: false
                        positionMode.rotation: 0
                    }
                }
            ]
        }
    }

    component GridLabel: Label {
        // Positioner.index doesn't work here
        visible: parent.children[parent.children.indexOf(this) + 1].text
    }

    component LinkedText: Label {
        onLinkActivated: link => Qt.openUrlExternally(link)
        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
        Layout.fillWidth: true
        visible: text
    }

    Pane {
        id: pane

        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        visible: false
        topPadding: 0
        width: Math.min(implicitWidth, root.width)

        Column {
            id: column

            width: Math.min(implicitWidth, parent.width)

            Row {
                ColumnLayout {
                    id: innerColumn
                    width: Math.min(implicitWidth, root.width - pane.leftPadding - pane.rightPadding - close.implicitWidth)

                    LinkedText {
                        text: root.map.title
                        font.bold: true
                    }

                    LinkedText {
                        text: root.map.subTitle
                    }

                    LinkedText {
                        text: root.map.address
                    }
                }

                Item {
                    implicitWidth: close.implicitWidth
                    implicitHeight: close.implicitHeight

                    Item {
                        implicitWidth: column.width - innerColumn.width + pane.rightPadding

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

                LinkedText {
                    text: root.map.wikipedia ? "<a href='" + root.map.wikipedia + "'>Wikipedia</a>" : ""
                }

                LinkedText {
                    text: root.map.wikimedia ? "<a href='" + root.map.wikimedia + "'>Wikimedia Commons</a>" : ""
                }
            }

            Label {
                text: root.map.description
                wrapMode: Text.Wrap
                width: Math.min(implicitWidth, root.width - pane.leftPadding - pane.rightPadding)
            }

            GridLayout {
                columns: 2
                columnSpacing: 5
                width: Math.min(implicitWidth, root.width - pane.leftPadding - pane.rightPadding)

                GridLabel {
                    text: "Bookmark:"
                }
                LinkedText {
                    text: root.map.bookmark ? "Yes" : ""
                }

                GridLabel {
                    text: "Opening hours:"
                }
                LinkedText {
                    text: root.map.openingHours
                }

                GridLabel {
                    text: "Cuisine:"
                }
                LinkedText {
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
                LinkedText {
                    text: root.map.operator
                }

                GridLabel {
                    text: "Wi-Fi:"
                }
                LinkedText {
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
                Label {
                    text: root.map.level
                }

                GridLabel {
                    text: "ATM:"
                }
                Label {
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
