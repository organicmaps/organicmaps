import QtQuick 2.6
import Sailfish.Silica 1.0
import app.organicmaps 1.0
import "downloads.js" as Downloads
import "notices.js" as Toast
import "routepoints.js" as RoutePoints

// Not modal, so places can still be picked on the map. Docked in the left half in landscape, like
// layout-land/routing_bottom_sheet.xml on Android.
MapPanel {
    id: panel

    // MapItem.routing; the uncreatable C++ type can't be a property type in Qt 5.6.
    property QtObject routing
    property QtObject placePage
    readonly property bool shouldShow: routing.active && !routing.navigating && !placePage.open
    readonly property bool hasStart: routing.points.length > 0 && routing.points[0].type === Routing.Start
    readonly property bool hasFinish: routing.points.length > 0
                                      && routing.points[routing.points.length - 1].type === Routing.Finish
    readonly property bool landscape: parent.width > parent.height
    // The map space the sheet covers from the bottom; none beside it in landscape.
    readonly property real bottomInset: landscape ? 0 : visibleSize

    // Empty slots and stops are picked like any place: from search, bookmarks or the map.
    signal searchClicked()
    signal bookmarksClicked()
    signal chooseOnMapClicked()

    // Navigation starts from the position, which is asked about first, and the disclaimer is accepted once.
    function startNavigation() {
        if (!routing.startIsMyPosition) {
            pageStack.push(Qt.resolvedUrl("MessageDialog.qml"), {
                title: appInfo.localized("p2p_only_from_current"),
                message: appInfo.localized("p2p_reroute_from_current"),
                acceptAction: function() { panel.checkDisclaimerAndStart() }
            })
            return
        }
        checkDisclaimerAndStart()
    }
    function checkDisclaimerAndStart() {
        if (routing.disclaimerAccepted) {
            routing.start()
            return
        }
        var message = ["dialog_routing_disclaimer_priority", "dialog_routing_disclaimer_precision",
                       "dialog_routing_disclaimer_recommendations", "dialog_routing_disclaimer_borders",
                       "dialog_routing_disclaimer_beware"].map(function(key) { return appInfo.localized(key) })
        pageStack.push(Qt.resolvedUrl("MessageDialog.qml"), {
            title: appInfo.localized("dialog_routing_disclaimer_title"),
            message: message.join("\n\n"),
            acceptText: appInfo.localized("accept"),
            acceptAction: function() {
                routing.acceptDisclaimer()
                routing.start()
            }
        })
    }

    width: landscape ? Math.round(parent.width / 2) : parent.width
    modal: false
    spacing: 0
    onShouldShowChanged: open = shouldShow
    // It can't be swiped away; the close button ends the route.
    onOpenChanged: if (!open && shouldShow) open = true
    Component.onCompleted: open = shouldShow

    Connections {
        target: routing
        onMessage: Toast.show(text)
    }

    // Everything but the bottom bar scrolls when it doesn't fit on the screen.
    SilicaFlickable {
        width: parent.width
        height: Math.min(content.height, panel.parent.height - bottomBar.height - 2 * Theme.paddingLarge)
        contentHeight: content.height
        clip: true

        Column {
            id: content
            width: parent.width

            Item {
                width: parent.width
                height: Theme.itemSizeMedium

                Rectangle {
                    id: routerBar
                    anchors {
                        left: parent.left
                        leftMargin: Theme.horizontalPageMargin
                        right: closeButton.left
                        verticalCenter: parent.verticalCenter
                    }
                    height: Theme.itemSizeSmall
                    radius: height / 2
                    color: Theme.rgba(Theme.primaryColor, 0.08)

                    Row {
                        anchors.fill: parent

                        Repeater {
                            model: [
                                { type: Routing.Vehicle, icon: "ic_car.webp" },
                                { type: Routing.Pedestrian, icon: "ic_pedestrian.webp" },
                                { type: Routing.Transit, icon: "ic_transit.webp" },
                                { type: Routing.Bicycle, icon: "ic_bike.webp" },
                                { type: Routing.Ruler, icon: "ic_ruler_route.svg" }
                            ]

                            MouseArea {
                                readonly property bool selected: routing.routerType === modelData.type

                                width: routerBar.width / 5
                                height: routerBar.height
                                onClicked: routing.routerType = modelData.type

                                Rectangle {
                                    anchors.fill: parent
                                    anchors.margins: Theme.paddingSmall / 2
                                    radius: height / 2
                                    visible: parent.selected || parent.pressed
                                    color: Theme.rgba(Theme.highlightBackgroundColor, parent.pressed ? 0.5 : 0.3)
                                }
                                Icon {
                                    anchors.centerIn: parent
                                    source: "../../icons/routing/" + modelData.icon
                                    sourceSize: Qt.size(Theme.iconSizeMedium, Theme.iconSizeMedium)
                                    color: parent.selected ? Theme.highlightColor : Theme.primaryColor
                                }
                            }
                        }
                    }
                }
                IconButton {
                    id: closeButton
                    anchors {
                        right: parent.right
                        rightMargin: Theme.paddingMedium
                        verticalCenter: parent.verticalCenter
                    }
                    icon.source: "image://theme/icon-m-clear"
                    Accessible.name: appInfo.localized("close")
                    onClicked: routing.close()
                }
            }

            Item {
                width: parent.width
                height: Math.max(routeButtons.height, statusColumn.height)

                Column {
                    id: statusColumn
                    anchors {
                        left: parent.left
                        leftMargin: Theme.horizontalPageMargin
                        right: routeButtons.left
                        verticalCenter: parent.verticalCenter
                    }

                    BusyIndicator {
                        visible: running
                        size: BusyIndicatorSize.ExtraSmall
                        running: routing.building
                    }
                    // "24 min • (walk) 810 m" for transit.
                    Row {
                        visible: routing.built
                        spacing: Theme.paddingSmall

                        Label {
                            text: routing.summary
                            font.bold: true
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        // The arrival if started now.
                        Label {
                            visible: routing.arrival !== ""
                            text: "• " + routing.arrival
                            color: Theme.secondaryColor
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Label {
                            visible: routing.walkingDistance !== ""
                            text: "•"
                            color: Theme.secondaryColor
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Icon {
                            visible: routing.walkingDistance !== ""
                            source: "../../icons/routing/ic_20px_route_planning_walk.webp"
                            sourceSize: Qt.size(Theme.iconSizeExtraSmall, Theme.iconSizeExtraSmall)
                            color: Theme.secondaryColor
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Label {
                            visible: routing.walkingDistance !== ""
                            text: routing.walkingDistance
                            color: Theme.secondaryColor
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    Label {
                        visible: routing.built && text !== ""
                        text: routing.ascentDescent
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.secondaryColor
                    }
                    // For the ruler, the distances between the stops.
                    Flow {
                        width: parent.width
                        visible: routing.transitSteps.length > 0
                        spacing: Theme.paddingSmall
                        topPadding: Theme.paddingSmall

                        Repeater {
                            model: routing.transitSteps

                            Row {
                                spacing: Theme.paddingSmall

                                Label {
                                    visible: index > 0
                                    text: "•"
                                    color: Theme.secondaryColor
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                Rectangle {
                                    width: stepRow.width + 2 * Theme.paddingSmall
                                    height: Theme.iconSizeSmall + Theme.paddingSmall
                                    radius: Theme.paddingSmall
                                    color: modelData.color || Theme.rgba(Theme.primaryColor, 0.15)

                                    Row {
                                        id: stepRow
                                        anchors.centerIn: parent
                                        spacing: Theme.paddingSmall

                                        Icon {
                                            visible: !!modelData.icon
                                            source: modelData.icon ? "../../icons/routing/" + modelData.icon : ""
                                            sourceSize: Qt.size(Theme.iconSizeExtraSmall, Theme.iconSizeExtraSmall)
                                            color: modelData.textColor || Theme.primaryColor
                                            anchors.verticalCenter: parent.verticalCenter
                                        }
                                        Label {
                                            visible: !!modelData.number
                                            text: modelData.number || ""
                                            color: modelData.textColor || Theme.primaryColor
                                            font.pixelSize: Theme.fontSizeExtraSmall
                                            font.bold: true
                                            anchors.verticalCenter: parent.verticalCenter
                                        }
                                    }
                                }
                            }
                        }
                    }
                    Label {
                        width: parent.width
                        visible: text !== ""
                        text: routing.errorTitle
                        color: Theme.errorColor
                        wrapMode: Text.Wrap
                    }
                    Label {
                        width: parent.width
                        visible: text !== ""
                        text: routing.errorMessage
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.secondaryColor
                        wrapMode: Text.Wrap
                    }
                }
                Row {
                    id: routeButtons
                    anchors {
                        right: parent.right
                        rightMargin: Theme.paddingMedium
                        verticalCenter: parent.verticalCenter
                    }

                    IconButton {
                        enabled: routing.canReverse
                        icon.source: "image://theme/icon-m-transfer"
                        icon.sourceSize: Qt.size(Theme.iconSizeMedium, Theme.iconSizeMedium)
                        Accessible.name: appInfo.localized("reverse_route")
                        onClicked: routing.reverseRoute()
                    }
                    IconButton {
                        id: optionsButton
                        // A badge for avoided roads.
                        readonly property int avoidedCount: [Routing.Toll, Routing.Dirty, Routing.Ferry,
                                                             Routing.Motorway].filter(function(road) {
                            return (routing.avoidRoads & road) !== 0
                        }).length

                        icon.source: "image://theme/icon-m-setting"
                        Accessible.name: appInfo.localized("driving_options_title")
                        onClicked: pageStack.push(Qt.resolvedUrl("RoutingOptionsPage.qml"), { routing: routing })

                        Rectangle {
                            visible: optionsButton.avoidedCount > 0
                            anchors {
                                right: parent.right
                                top: parent.top
                            }
                            width: Math.max(height, badgeLabel.implicitWidth + Theme.paddingSmall)
                            height: badgeLabel.implicitHeight
                            radius: height / 2
                            color: Theme.highlightBackgroundColor

                            Label {
                                id: badgeLabel
                                anchors.centerIn: parent
                                text: optionsButton.avoidedCount
                                font.pixelSize: Theme.fontSizeTiny
                                font.bold: true
                            }
                        }
                    }
                }
            }

            // Lower than on the track page, leaving room for the route points.
            ElevationChart {
                height: Theme.itemSizeExtraLarge
                elevation: routing.elevation
                activePoint: routing.elevationActivePoint
                onPointClicked: routing.setElevationActivePoint(distance)
            }

            // Avoided roads are the likely cause of a failed car route.
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                visible: routing.optionsError
                text: appInfo.localized("settings")
                onClicked: pageStack.push(Qt.resolvedUrl("RoutingOptionsPage.qml"), { routing: routing })
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                visible: routing.missingMaps.length > 0
                text: appInfo.localized("download") + " (" + routing.missingMapsSize + ")"
                onClicked: Downloads.start(pageStack, function() { routing.downloadMissingMaps() })
            }

            // Press and hold a point to reorder or remove it.
            Item {
                width: parent.width
                height: card.height + Theme.paddingMedium

                Rectangle {
                    id: card
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * x
                    height: pointsColumn.height
                    radius: Theme.paddingLarge
                    color: Theme.rgba(Theme.primaryColor, 0.06)

                    Column {
                        id: pointsColumn
                        width: parent.width

                        RoutePointSlot {
                            visible: !panel.hasStart
                            empty: true
                            icon: "../../icons/routing/route_point_start.png"
                            text: appInfo.localized("p2p_from_here")
                            onClicked: routing.startPick(Routing.Start, -1)
                        }

                        Repeater {
                            model: routing.points

                            RoutePointSlot {
                                id: pointSlot
                                icon: modelData.isMyPosition ? "../../icons/routing/ic_location_arrow_blue.svg"
                                    : modelData.type === Routing.Start ? "../../icons/routing/route_point_start.png"
                                    : modelData.type === Routing.Finish ? "../../icons/routing/route_point_finish.png"
                                    : "../../icons/routing/route_point_0" + Math.min(index, 9) + ".svg"
                                text: modelData.title

                                menu: ContextMenu {
                                    MenuItem {
                                        visible: index > 0
                                        text: appInfo.localized("move_up")
                                        onClicked: routing.movePoint(index, index - 1)
                                    }
                                    MenuItem {
                                        visible: index < routing.points.length - 1
                                        text: appInfo.localized("move_down")
                                        onClicked: routing.movePoint(index, index + 1)
                                    }
                                    MenuItem {
                                        visible: modelData.type === Routing.Start && !modelData.isMyPosition
                                        text: appInfo.localized("core_my_position")
                                        onClicked: routing.setStartToMyPosition()
                                    }
                                    MenuItem {
                                        text: RoutePoints.pickTitle(modelData.type, true)
                                        onClicked: routing.startPick(modelData.type, index)
                                    }
                                    MenuItem {
                                        text: appInfo.localized("delete")
                                        onClicked: {
                                            var removed = index
                                            pointSlot.remorseDelete(function() { routing.removePoint(removed) })
                                        }
                                    }
                                }
                            }
                        }

                        RoutePointSlot {
                            visible: !panel.hasFinish
                            empty: true
                            icon: "../../icons/routing/route_point_finish.png"
                            text: appInfo.localized("p2p_to_here")
                            onClicked: routing.startPick(Routing.Finish, -1)
                        }
                        RoutePointSlot {
                            visible: panel.hasStart && panel.hasFinish && routing.canAddStop
                            icon: "image://theme/icon-m-add"
                            text: appInfo.localized("placepage_add_stop")
                            accent: true
                            onClicked: routing.startPick(Routing.Intermediate, -1)
                        }
                    }
                }
            }

            Column {
                width: parent.width
                visible: routing.pickType >= 0

                SectionHeader {
                    text: RoutePoints.pickTitle(routing.pickType, routing.pickIndex >= 0)
                }
                Flow {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * x
                    spacing: Theme.paddingMedium

                    Button {
                        text: appInfo.localized("search")
                        onClicked: panel.searchClicked()
                    }
                    Button {
                        text: appInfo.localized("bookmarks")
                        onClicked: panel.bookmarksClicked()
                    }
                    Button {
                        text: appInfo.localized("choose_on_map")
                        onClicked: panel.chooseOnMapClicked()
                    }
                    Button {
                        visible: routing.pickType !== Routing.Intermediate
                        text: appInfo.localized("p2p_your_location")
                        onClicked: routing.pickMyPosition()
                    }
                    Button {
                        text: appInfo.localized("cancel")
                        onClicked: routing.cancelPick()
                    }
                }
                Item {
                    width: 1
                    height: Theme.paddingMedium
                }
            }
        }

        VerticalScrollDecorator {}
    }

    // Search, bookmarks and save before the start button, as on Android.
    Row {
        id: bottomBar

        x: Theme.paddingMedium
        width: parent.width - x - Theme.horizontalPageMargin
        height: Theme.itemSizeMedium

        IconButton {
            anchors.verticalCenter: parent.verticalCenter
            icon.source: "image://theme/icon-m-search"
            Accessible.name: appInfo.localized("search")
            onClicked: panel.searchClicked()
        }
        IconButton {
            anchors.verticalCenter: parent.verticalCenter
            icon.source: "image://theme/icon-m-favorite"
            Accessible.name: appInfo.localized("bookmarks")
            onClicked: panel.bookmarksClicked()
        }
        // Saves the route as a track, once per built route.
        IconButton {
            anchors.verticalCenter: parent.verticalCenter
            icon.source: "image://theme/icon-m-downloads"
            Accessible.name: appInfo.localized("save")
            enabled: routing.built && !routing.routeSaved
            onClicked: routing.saveRoute()
        }
        Item {
            width: Theme.paddingMedium
            height: 1
        }
        Button {
            anchors.verticalCenter: parent.verticalCenter
            width: bottomBar.width - x
            enabled: routing.canStart
            text: appInfo.localized("p2p_start").toUpperCase()
            onClicked: panel.startNavigation()
        }
    }
}
