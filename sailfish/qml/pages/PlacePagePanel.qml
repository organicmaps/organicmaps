import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.Share 1.0
import app.organicmaps 1.0
import "bookmarks.js" as Bookmarks
import "clipboard.js" as ClipboardHelper
import "colors.js" as Colors
import "downloads.js" as Downloads
import "routepoints.js" as RoutePoints
import "share.js" as Share

// Not modal, so the map stays usable.
MapPanel {
    id: panel

    // MapItem.placePage; the uncreatable C++ type can't be a property type in Qt 5.6.
    property QtObject placePage
    // MapItem.routing and the MapItem.
    property QtObject routing
    property QtObject map
    property bool hoursExpanded
    property bool wikiExpanded
    // A line count for no limit: maximumLineCount can't be reset to unlimited on Qt 5.6.
    readonly property int unlimitedLines: 10000
    // A road warning on the route offers to avoid such roads, and a route point to be removed, instead.
    readonly property bool specialAction: placePage.roadToAvoid !== 0 || picking || placePage.isRoutePoint
    // A route slot waits for a place: this one fills it.
    readonly property bool picking: routing.pickType >= 0

    signal addPlaceClicked()
    signal addBusinessClicked()
    // The close button or a swipe closed the panel, not a tap on the map or another selection.
    signal closedByUser()
    signal directionClicked()

    modal: false
    spacing: 0

    // Swiping the panel away deselects the place.
    onOpenChanged: {
        if (!open && placePage.open) {
            placePage.close()
            closedByUser()
        }
    }
    Connections {
        target: panel.placePage
        onChanged: {
            panel.open = panel.placePage.open
            hoursExpanded = false
            wikiExpanded = false
            flickable.contentY = 0
        }
    }

    // Takes at most about the lower 60% of the map.
    SilicaFlickable {
        id: flickable
        width: parent.width
        height: Math.min(content.height, panel.parent.height * 0.6 - actions.height)
        contentHeight: content.height
        clip: true

        Column {
            id: content
            width: parent.width

            Item {
                width: parent.width
                height: header.height + Theme.paddingLarge

                Column {
                    id: header
                    y: Theme.paddingLarge
                    anchors {
                        left: parent.left
                        leftMargin: Theme.horizontalPageMargin
                        right: shareButton.left
                    }

                    Label {
                        width: parent.width - (candidatesButton.visible ? candidatesButton.width : 0)
                        text: placePage.title
                        font.pixelSize: Theme.fontSizeLarge
                        color: Theme.highlightColor
                        wrapMode: Text.Wrap

                        MouseArea {
                            anchors.fill: parent
                            onPressAndHold: ClipboardHelper.copy(placePage.title)
                        }
                        // Several tracks under the tap: choose another one.
                        IconButton {
                            id: candidatesButton
                            anchors {
                                left: parent.right
                                verticalCenter: parent.verticalCenter
                            }
                            visible: placePage.trackCandidates.length > 1
                            icon.source: "image://theme/icon-m-down"
                            onClicked: pageStack.push(Qt.resolvedUrl("ListPickerPage.qml"), {
                                title: appInfo.localized("tracks_title"),
                                items: placePage.trackCandidates.map(function(track) {
                                    return { name: track.title, color: track.color, selected: track.selected }
                                }),
                                picked: function(index) { placePage.selectTrackCandidate(index) }
                            })
                        }
                    }
                    Label {
                        width: parent.width
                        visible: text !== ""
                        text: placePage.secondaryTitle
                        color: Theme.highlightColor
                        wrapMode: Text.Wrap
                    }
                    // Tracks show their statistics instead.
                    Label {
                        width: parent.width
                        visible: text !== "" && !placePage.isTrack
                        text: placePage.subtitle
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.secondaryHighlightColor
                        wrapMode: Text.Wrap
                    }
                    Label {
                        width: parent.width
                        visible: text !== ""
                        text: placePage.address
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.secondaryColor
                        wrapMode: Text.Wrap

                        MouseArea {
                            anchors.fill: parent
                            onPressAndHold: ClipboardHelper.copy(placePage.address)
                        }
                    }
                }
                IconButton {
                    id: shareButton
                    anchors {
                        right: closeButton.left
                        top: closeButton.top
                    }
                    visible: !placePage.isRelationTrack
                    icon.source: "image://theme/icon-m-share"
                    // A track is shared as a file.
                    onClicked: {
                        if (!placePage.isTrack) {
                            shareAction.trigger()
                            return
                        }
                        var trackId = placePage.userMarkId
                        Bookmarks.pickExportFormat(pageStack, function(type) { bookmarksIO.exportTrack(trackId, type) })
                    }
                }
                IconButton {
                    id: closeButton
                    anchors {
                        right: parent.right
                        rightMargin: Theme.paddingMedium
                        top: parent.top
                        topMargin: Theme.paddingMedium
                    }
                    icon.source: "image://theme/icon-m-cancel"
                    onClicked: {
                        placePage.close()
                        panel.closedByUser()
                    }
                }
                Row {
                    id: direction
                    anchors {
                        right: parent.right
                        rightMargin: Theme.horizontalPageMargin
                        top: closeButton.bottom
                    }
                    spacing: Theme.paddingSmall
                    visible: placePage.distance !== ""

                    Image {
                        anchors.verticalCenter: parent.verticalCenter
                        width: Theme.iconSizeSmall
                        height: width
                        sourceSize: Qt.size(width, height)
                        source: "../../icons/placepage/ic_direction_pagepreview.webp"
                        rotation: placePage.azimuth
                        visible: placePage.azimuth >= 0
                    }
                    Label {
                        text: placePage.distance
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.highlightColor
                    }
                }
                MouseArea {
                    anchors {
                        fill: direction
                        margins: -Theme.paddingSmall
                    }
                    enabled: direction.visible
                    onClicked: panel.directionClicked()
                }
            }

            ListItem {
                id: categoryRow
                visible: placePage.listName !== ""
                contentHeight: Theme.itemSizeSmall
                onClicked: Bookmarks.pickList(pageStack, placePage.listId,
                                              function(id) { placePage.setList(id) })

                ColorDot {
                    id: colorDot
                    x: Theme.horizontalPageMargin
                    anchors.verticalCenter: parent.verticalCenter
                    width: Theme.iconSizeSmallPlus
                    color: placePage.color || "transparent"

                    MouseArea {
                        anchors {
                            fill: parent
                            margins: -Theme.paddingMedium
                        }
                        onClicked: pageStack.push(Qt.resolvedUrl("ColorPickerPage.qml"), {
                            title: appInfo.localized("choose_color"),
                            current: placePage.color,
                            chosen: function(colorIndex) { placePage.setColor(colorIndex) }
                        })
                    }
                }
                Label {
                    anchors {
                        left: colorDot.right
                        leftMargin: Theme.paddingLarge
                        right: editButton.left
                        verticalCenter: parent.verticalCenter
                    }
                    text: placePage.listName
                    truncationMode: TruncationMode.Fade
                    highlighted: categoryRow.highlighted
                }
                IconButton {
                    id: editButton
                    anchors {
                        right: parent.right
                        rightMargin: Theme.horizontalPageMargin - Theme.paddingMedium
                        verticalCenter: parent.verticalCenter
                    }
                    icon.source: "image://theme/icon-m-edit"
                    onClicked: pageStack.push(Qt.resolvedUrl("EditBookmarkPage.qml"),
                                              { itemId: placePage.userMarkId, isTrack: placePage.isTrack })
                }
            }
            Repeater {
                model: [placePage.notes, placePage.osmDescription]

                PageLabel {
                    visible: modelData !== ""
                    text: modelData
                    textFormat: Text.StyledText
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.secondaryHighlightColor
                    bottomPadding: Theme.paddingMedium
                    linkColor: Theme.highlightColor
                    onLinkActivated: Qt.openUrlExternally(link)
                }
            }
            ListItem {
                id: countryRow

                readonly property var country: placePage.country
                readonly property bool busy: Downloads.busy(country.status)

                visible: !!country.countryId
                contentHeight: Theme.itemSizeMedium
                onClicked: Downloads.toggle(pageStack, panel.map, country)

                Icon {
                    id: countryIcon
                    x: Theme.horizontalPageMargin
                    anchors.verticalCenter: parent.verticalCenter
                    visible: !countryRow.busy
                    source: "image://theme/icon-m-cloud-download"
                    sourceSize: Qt.size(Theme.iconSizeMedium, Theme.iconSizeMedium)
                    highlighted: countryRow.highlighted
                }
                ProgressCircle {
                    anchors.fill: countryIcon
                    visible: countryRow.busy
                    value: countryRow.country.progress || 0
                    progressColor: Theme.highlightColor
                    backgroundColor: Theme.rgba(Theme.highlightDimmerColor, 0.5)
                }
                Label {
                    anchors {
                        left: countryIcon.right
                        leftMargin: Theme.paddingLarge
                        right: parent.right
                        rightMargin: Theme.horizontalPageMargin
                        verticalCenter: parent.verticalCenter
                    }
                    text: {
                        var name = countryRow.country.name || ""
                        if (countryRow.busy)
                            return appInfo.localized("cancel_download") + " • " + name
                        var action = countryRow.country.outdated ? "downloader_update_map" : "downloader_download_map"
                        return appInfo.localized(action) + " • " + name + " (" + (countryRow.country.size || "") + ")"
                    }
                    truncationMode: TruncationMode.Fade
                    highlighted: countryRow.highlighted
                }
            }

            ElevationProfile {
                visible: placePage.isTrack
                placePage: panel.placePage
            }

            ListItem {
                id: routesRow
                visible: placePage.routeRefs !== ""
                contentHeight: Math.max(Theme.itemSizeMedium, routesLabel.height + 2 * Theme.paddingMedium)
                onClicked: openMenu()
                menu: ContextMenu {
                    Repeater {
                        model: placePage.routes

                        MenuItem {
                            text: modelData.label
                            truncationMode: TruncationMode.Fade
                            onClicked: placePage.showRoute(index)

                            Rectangle {
                                visible: modelData.color !== ""
                                anchors {
                                    left: parent.left
                                    leftMargin: Theme.paddingMedium
                                    verticalCenter: parent.verticalCenter
                                }
                                width: Theme.paddingSmall
                                height: parent.height * 0.6
                                radius: width / 2
                                color: modelData.color || "transparent"
                            }
                        }
                    }
                }

                Icon {
                    id: routesIcon
                    x: Theme.horizontalPageMargin
                    anchors.verticalCenter: routesLabel.verticalCenter
                    width: Theme.iconSizeMedium
                    height: width
                    sourceSize: Qt.size(width, height)
                    source: "../../icons/placepage/" + (placePage.isTramStop ? "ic_category_tram.svg"
                                                                             : "ic_category_bus.svg")
                    highlighted: routesRow.highlighted
                }
                Label {
                    id: routesLabel
                    anchors {
                        left: routesIcon.right
                        leftMargin: Theme.paddingLarge
                        right: parent.right
                        rightMargin: Theme.horizontalPageMargin
                        verticalCenter: parent.verticalCenter
                    }
                    text: placePage.routeRefs
                    textFormat: Text.StyledText
                    wrapMode: Text.Wrap
                    highlighted: routesRow.highlighted
                }
            }

            BackgroundItem {
                width: parent.width
                height: hoursColumn.height + 2 * Theme.paddingMedium
                visible: placePage.openingHours !== ""
                onClicked: hoursExpanded = !hoursExpanded

                Icon {
                    id: hoursIcon
                    x: Theme.horizontalPageMargin
                    y: Theme.paddingMedium
                    source: "image://theme/icon-m-clock"
                }
                Column {
                    id: hoursColumn
                    y: Theme.paddingMedium
                    anchors {
                        left: hoursIcon.right
                        leftMargin: Theme.paddingLarge
                        right: expandIcon.left
                    }

                    Label {
                        width: parent.width
                        text: placePage.openTitle !== "" ? placePage.openTitle : placePage.openingHours
                        color: placePage.openState === PlacePage.Open ? Colors.open
                             : placePage.openState === PlacePage.Closed ? Colors.closed : Theme.primaryColor
                        wrapMode: Text.Wrap
                    }
                    Label {
                        width: parent.width
                        visible: text !== ""
                        text: placePage.openDescription
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.secondaryColor
                    }
                    Column {
                        width: parent.width
                        visible: hoursExpanded && placePage.openingSchedule.length > 0
                        topPadding: Theme.paddingSmall

                        Repeater {
                            model: placePage.openingSchedule

                            Row {
                                width: parent.width

                                Label {
                                    width: parent.width * 0.4
                                    text: modelData.days
                                    font.pixelSize: Theme.fontSizeSmall
                                    font.bold: modelData.today
                                    truncationMode: TruncationMode.Fade
                                }
                                Label {
                                    width: parent.width * 0.6
                                    text: modelData.hours
                                    font.pixelSize: Theme.fontSizeSmall
                                    font.bold: modelData.today
                                    wrapMode: Text.Wrap
                                }
                            }
                        }
                    }
                    // Rules that don't fit a weekly table.
                    Label {
                        width: parent.width
                        visible: hoursExpanded && placePage.openTitle !== "" && placePage.openingSchedule.length === 0
                        text: placePage.openingHours.split(";").map(function(rule) { return rule.trim() }).join("\n")
                        font.pixelSize: Theme.fontSizeSmall
                        wrapMode: Text.Wrap
                    }
                }
                Icon {
                    id: expandIcon
                    anchors {
                        right: parent.right
                        rightMargin: Theme.horizontalPageMargin
                    }
                    y: Theme.paddingMedium
                    visible: placePage.openTitle !== "" || placePage.openingSchedule.length > 0
                    source: hoursExpanded ? "image://theme/icon-m-up" : "image://theme/icon-m-down"
                }
            }

            Column {
                width: parent.width
                visible: placePage.wikiDescription !== "" || placePage.wikiUrl !== ""

                MenuRow {
                    icon: "../../icons/placepage/ic_wiki.svg"
                    text: appInfo.localized("read_in_wikipedia")
                    enabled: placePage.wikiUrl !== ""
                    onClicked: Qt.openUrlExternally(placePage.wikiUrl)
                }
                PageLabel {
                    id: wikiLabel
                    visible: placePage.wikiDescription !== ""
                    text: placePage.wikiDescription
                    textFormat: Text.StyledText
                    font.pixelSize: Theme.fontSizeSmall
                    maximumLineCount: wikiExpanded ? unlimitedLines : 9
                    elide: Text.ElideRight
                }
                Label {
                    x: Theme.horizontalPageMargin
                    visible: wikiLabel.truncated || wikiExpanded
                    text: wikiExpanded ? appInfo.localized("less") : appInfo.localized("text_more_button")
                    color: Theme.highlightColor
                    font.pixelSize: Theme.fontSizeSmall
                    bottomPadding: Theme.paddingMedium

                    MouseArea {
                        anchors.fill: parent
                        onClicked: wikiExpanded = !wikiExpanded
                    }
                }
            }

            Repeater {
                model: placePage.details

                MenuRow {
                    icon: modelData.icon.indexOf("image://") === 0 ? modelData.icon : "../../icons/" + modelData.icon
                    text: modelData.text
                    onClicked: {
                        if (modelData.url !== "")
                            Qt.openUrlExternally(modelData.url)
                        else
                            openMenu()
                    }

                    menu: ContextMenu {
                        MenuItem {
                            text: appInfo.localized("copy_value", [modelData.text])
                            onClicked: Clipboard.text = modelData.text
                        }
                        MenuItem {
                            visible: modelData.url !== "" && modelData.url !== modelData.text
                            text: appInfo.localized("copy_value", [modelData.url])
                            onClicked: Clipboard.text = modelData.url
                        }
                    }
                }
            }
            // Tracks have no single point to show or open elsewhere.
            MenuRow {
                visible: !placePage.isTrack
                icon: "image://theme/icon-m-whereami"
                text: placePage.coordinates
                onClicked: placePage.nextCoordinatesFormat()

                menu: ContextMenu {
                    Repeater {
                        model: placePage.coordinateValues

                        MenuItem {
                            text: appInfo.localized("copy_value", [modelData])
                            onClicked: Clipboard.text = modelData
                        }
                    }
                }
            }
            // Hands the geo: link to the default handler, e.g. Pure Maps.
            MenuRow {
                visible: !placePage.isTrack
                icon: "image://theme/icon-m-shortcut"
                text: appInfo.localized("open_in_app")
                onClicked: Qt.openUrlExternally(placePage.geoUri)
            }
            MenuRow {
                visible: placePage.canEdit && !routing.active
                enabled: placePage.editable
                icon: "image://theme/icon-m-edit"
                text: appInfo.localized("edit_place")
                onClicked: pageStack.push(Qt.resolvedUrl("EditPlacePage.qml"))
            }
            MenuRow {
                visible: placePage.canAddPlace && !routing.active
                enabled: placePage.editable
                icon: "image://theme/icon-m-add"
                text: appInfo.localized("placepage_add_place_button")
                onClicked: panel.addPlaceClicked()
            }
            MenuRow {
                visible: placePage.canAddBusiness && !routing.active
                enabled: placePage.editable
                icon: "image://theme/icon-m-add"
                text: appInfo.localized("placepage_add_business_button")
                onClicked: panel.addBusinessClicked()
            }
        }

        VerticalScrollDecorator {}
    }

    PlaceAction {
        width: parent.width
        visible: panel.picking && placePage.roadToAvoid === 0
        icon: routing.pickType === Routing.Start ? "../../icons/routing/ic_route_from.webp"
            : routing.pickType === Routing.Finish ? "../../icons/routing/ic_route_to.webp"
            : "../../icons/routing/ic_route_via.webp"
        text: RoutePoints.pickActionTitle(routing.pickType, routing.pickIndex >= 0)
        onClicked: routing.pickPlace()
    }
    PlaceAction {
        width: parent.width
        visible: panel.specialAction && !(panel.picking && placePage.roadToAvoid === 0)
        icon: placePage.roadToAvoid === Routing.Toll ? "../../icons/routing/ic_avoid_tolls.webp"
            : placePage.roadToAvoid === Routing.Dirty ? "../../icons/routing/ic_avoid_unpaved.webp"
            : placePage.roadToAvoid === Routing.Ferry ? "../../icons/routing/ic_avoid_ferry.webp"
            : "image://theme/icon-m-delete"
        text: placePage.roadToAvoid === Routing.Toll ? appInfo.localized("avoid_tolls")
            : placePage.roadToAvoid === Routing.Dirty ? appInfo.localized("avoid_unpaved")
            : placePage.roadToAvoid === Routing.Ferry ? appInfo.localized("avoid_ferry")
            : appInfo.localized("placepage_remove_stop")
        onClicked: {
            if (placePage.roadToAvoid !== 0)
                routing.avoidRoad(placePage.roadToAvoid)
            else
                routing.removePlacePoint()
        }
    }
    Row {
        id: actions

        readonly property bool canDeleteTrack: placePage.isTrack && !placePage.isRelationTrack && !routing.active
        readonly property int count: (routing.active ? 4 : 3) + (placePage.apiBackUrl !== "" ? 1 : 0)
                                     + (canDeleteTrack ? 1 : 0)

        width: parent.width
        visible: !panel.specialAction

        PlaceAction {
            visible: placePage.apiBackUrl !== ""
            width: actions.width / actions.count
            icon: "image://theme/icon-m-back"
            text: appInfo.localized("back")
            onClicked: Qt.openUrlExternally(placePage.apiBackUrl)
        }
        PlaceAction {
            width: actions.width / actions.count
            icon: "../../icons/routing/ic_route_from.webp"
            text: appInfo.localized("p2p_from_here")
            onClicked: routing.routeFromPlace()
        }
        PlaceAction {
            visible: routing.active
            width: actions.width / actions.count
            icon: "../../icons/routing/ic_route_via.webp"
            text: appInfo.localized("placepage_add_stop")
            onClicked: routing.addStopFromPlace()
        }
        Loader {
            active: !routing.active
            visible: active
            width: actions.width / actions.count
            sourceComponent: saveAction
        }
        PlaceAction {
            visible: actions.canDeleteTrack
            width: actions.width / actions.count
            icon: "image://theme/icon-m-delete"
            text: appInfo.localized("delete")
            onClicked: {
                var trackId = placePage.userMarkId
                Remorse.popupAction(panel, appInfo.localized("delete"), function() {
                    bookmarksIO.deleteItems([{ id: trackId, isTrack: true }])
                })
            }
        }
        PlaceAction {
            width: actions.width / actions.count
            icon: "../../icons/routing/ic_route_to.webp"
            text: appInfo.localized("p2p_to_here")
            onClicked: routing.routeToPlace()
        }
        Loader {
            active: routing.active
            visible: active
            width: actions.width / actions.count
            sourceComponent: saveAction
        }
    }
    Component {
        id: saveAction

        PlaceAction {
            icon: placePage.isBookmark ? "image://theme/icon-m-favorite-selected" : "image://theme/icon-m-favorite"
            text: placePage.isBookmark ? appInfo.localized("delete")
                : appInfo.localized(placePage.canRestoreBookmark ? "restore" : "save")
            onClicked: placePage.toggleBookmark()
        }
    }

    ShareAction {
        id: shareAction
        mimeType: "text/plain"
        resources: [Share.textResource(placePage.shareText, placePage.title)]
    }
}
