import QtQuick 2.6
import Sailfish.Silica 1.0
import Nemo.KeepAlive 1.2
import Nemo.Notifications 1.0
import app.organicmaps 1.0
import "clipboard.js" as ClipboardHelper
import "downloads.js" as Downloads
import "icons.js" as Icons
import "notices.js" as Toast
import "notifications.js" as Notifications

Page {
    id: page

    // For children, where "map: map" would bind to their own property.
    readonly property QtObject mapItem: map
    readonly property bool navigating: map.routing.navigating
    readonly property bool choosingPosition: map.choosingPosition
    readonly property bool mapIsDark: {
        switch (appSettings.mapAppearance) {
        case AppSettings.AppearanceDark: return true
        case AppSettings.AppearanceScheduled: return map.routing.darkOutside
        case AppSettings.AppearanceAuto:
            if (Theme.colorScheme === Theme.LightOnDark)
                return true
            break
        }
        return appSettings.autoNightInNavigation && page.navigating && map.routing.darkOutside
    }
    readonly property real bottomPanelsSize: Math.max(placePagePanel.visibleSize, routePanel.bottomInset,
                                                      recordingPanel.visibleSize)
    // The lowest of the controls along the top right edge, or the padded camera notch in portrait; the compass
    // goes below it.
    readonly property real topControlsBottom: recordingButton.visible ? recordingButton.y + recordingButton.height
        : navigationTopPanel.visible ? navigationTopPanel.y + navigationTopPanel.rightHeight
        : topInset + buttonMargin
    // Silica keeps landscape pages clear of the camera notch.
    readonly property real topInset: page.orientation === Orientation.Portrait ? Screen.topCutout.height : 0
    // For the navigation and downloader layouts that keep the Android sizes.
    readonly property real androidDp: myPositionButton.androidDp
    readonly property real buttonMargin: Theme.paddingMedium
    // While the user types on a keepsUserInput page, the action waits for it to close rather than drop the input.
    property var pendingAction
    // A long tap on the empty map toggles the map buttons, except while a route is planned or followed.
    property bool fullscreen
    // The position chooser picks a route point or a position for another app instead of a new object's place.
    property bool choosingRoutePoint
    property bool choosingApiPoint
    property string apiAppName
    property string apiBackUrl
    // Closing a search result's place page returns to the results; another selection or a map tap forgets it.
    property bool placeFromSearch
    property string searchResultTitle

    function openSearchFromCover() {
        runOnMap(openSearch)
    }
    function runOnMap(action) {
        for (var p = pageStack.currentPage; p && p !== page; p = pageStack.previousPage(p)) {
            if (p.keepsUserInput) {
                pendingAction = action
                return
            }
        }
        pendingAction = null
        pageStack.pop(page, PageStackAction.Immediate)
        action()
    }
    function runPendingAction() {
        if (pendingAction && !pageStack.busy)
            runOnMap(pendingAction)
    }
    function openSearch() {
        var searchPage = pageStack.push(Qt.resolvedUrl("SearchPage.qml"), { search: search, map: map })
        searchPage.resultActivated.connect(function() {
            page.searchResultTitle = ""
            page.placeFromSearch = true
        })
    }
    function openBookmarks() {
        pageStack.push(Qt.resolvedUrl("BookmarksPage.qml"))
    }
    function showRecording() {
        recordingPanel.open = true
    }

    objectName: "mapPage"
    allowedOrientations: Orientation.All
    backNavigation: false

    onMapIsDarkChanged: appSettings.applyMapAppearance(mapIsDark)
    onChoosingPositionChanged: {
        if (choosingPosition) {
            fullscreen = false
        } else {
            choosingRoutePoint = false
            choosingApiPoint = false
        }
    }
    // Navigation starts without the results of an earlier search, so the first tap on its search button
    // searches; quick search results leave the map when navigation ends.
    onNavigatingChanged: {
        quickSearch.expanded = false
        if (navigating) {
            fullscreen = false
            search.query = ""
        } else if (quickSearch.key !== "") {
            search.query = ""
        }
    }
    Component.onCompleted: {
        appSettings.applyMapAppearance(mapIsDark)
        map.routing.restoreSavedRoute()
        // A recording goes on after a restart.
        if (map.trackRecording)
            recordingNotification.publish()
    }
    Component.onDestruction: recordingNotification.close()

    // Recording keeps the device awake.
    KeepAlive {
        enabled: map.trackRecording || map.routing.navigating
    }
    DisplayBlanking {
        preventBlanking: (appSettings.keepScreenOn || map.routing.navigating) && Qt.application.active
    }


    MapItem {
        id: map
        // Silica keeps landscape pages clear of the camera cutout, at the left in Landscape and the right in
        // LandscapeInverted; the map alone is drawn under it.
        anchors {
            fill: parent
            leftMargin: -leftInset
            rightMargin: -rightInset
        }
        leftInset: page.orientation === Orientation.Landscape ? Screen.topCutout.height : 0
        rightInset: page.orientation === Orientation.LandscapeInverted ? Screen.topCutout.height : 0
        // Keeps the scale line and attribution above the bottom row with its padding, or above the navigation
        // panel.
        bottomWidgetsOffset: page.navigating ? navigationBottomPanel.height
                           : bottomButtons.visible ? bottomButtons.height + 2 * page.buttonMargin : 0
        topWidgetsOffset: page.topControlsBottom
        // Routes are fitted into the map above the open sheets.
        viewportBottomInset: page.navigating ? navigationBottomPanel.height : page.bottomPanelsSize
    }

    Connections {
        target: pageStack
        onCurrentPageChanged: page.runPendingAction()
        onBusyChanged: page.runPendingAction()
    }
    Connections {
        target: map
        onNotice: Toast.showLong(message)
        onIsolinesNeedMaps: pageStack.push(Qt.resolvedUrl("MessageDialog.qml"), {
            title: appInfo.localized("downloader_update_maps"),
            message: appInfo.localized("isolines_activation_error_dialog"),
            acceptAction: function() { pageStack.push(Qt.resolvedUrl("MapsPage.qml")) }
        })
        onTrackRecordingChanged: {
            if (map.trackRecording)
                recordingNotification.publish()
            else
                recordingNotification.close()
        }
        onLocationLostChanged: if (map.trackRecording) recordingNotification.publish()
    }
    // Planning a route leaves fullscreen.
    Connections {
        target: map.routing
        onPointsChanged: if (map.routing.active) page.fullscreen = false
    }
    Connections {
        target: map.placePage
        onSwitchFullScreen: {
            if (map.routing.active || page.navigating)
                return
            page.fullscreen = !page.fullscreen
            if (page.fullscreen) {
                map.placePage.close()
                // Every time, so that the way back isn't forgotten.
                Toast.showLong(appInfo.localized("long_tap_toast"))
            }
        }
        onChanged: {
            if (!page.placeFromSearch)
                return
            if (!map.placePage.open)
                forgetSearchResult.restart()
            else if (page.searchResultTitle === "")
                page.searchResultTitle = map.placePage.title
            else if (map.placePage.title !== page.searchResultTitle)
                page.placeFromSearch = false
        }
    }
    // The close button and swipe report closedByUser right after the place page closes; else it was a map tap.
    Timer {
        id: forgetSearchResult
        interval: 100
        onTriggered: page.placeFromSearch = false
    }
    Connections {
        target: urlHandler
        onRouteRequested: page.runOnMap(function() { map.routing.planRoute(routerType, points) })
        onSearchRequested: page.runOnMap(function() {
            search.query = query
            if (onMap)
                search.showOnMap()
            else
                page.openSearch()
        })
        onCrosshairRequested: page.runOnMap(function() {
            page.apiAppName = appName
            page.apiBackUrl = backUrl
            page.choosingApiPoint = true
            map.startChoosingPosition(false)
        })
        // The notification promises a save, so an empty recording is told.
        onStopTrackRecordingRequested: {
            if (map.trackRecording && !map.saveAndStopTrackRecording())
                Toast.show(appInfo.localized("track_recording_toast_nothing_to_save"))
        }
    }

    NavigationTopPanel {
        id: navigationTopPanel
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            // margin_half around the Android turn card.
            margins: page.buttonMargin
            // Clear the camera notch in portrait, like the Silica PageHeader does.
            topMargin: page.buttonMargin + page.topInset
        }
        visible: page.navigating
        navigation: map.routing.navigation
        androidDp: page.androidDp
        landscape: page.isLandscape
    }

    MapButton {
        visible: !page.navigating && !page.choosingPosition && !page.fullscreen
        anchors {
            top: parent.top
            topMargin: page.topInset + page.buttonMargin
            left: parent.left
            leftMargin: page.buttonMargin
        }
        source: "image://theme/icon-m-levels"
        description: appInfo.localized("layers_title")
        highlighted: map.enabledLayers !== 0 || layersPanel.open
        onClicked: layersPanel.open = true
    }

    // Positions follow Android's layout-h400dp/map_buttons_layout_regular.xml and layout-land, in Silica sizes.
    Column {
        id: rightButtons
        visible: !page.choosingPosition && !page.fullscreen
        anchors {
            right: parent.right
            rightMargin: page.buttonMargin
            // Portrait: map_buttons_bottom_margin plus the padding from the bottom edge. Landscape: my position
            // is in the bottom right corner. While navigating: above the navigation panel.
            bottom: page.navigating && page.isPortrait ? navigationBottomPanel.top : parent.bottom
            bottomMargin: page.isPortrait && !page.navigating ? 3 * myPositionButton.size : page.buttonMargin
        }
        spacing: page.buttonMargin
        transform: Translate {
            id: rightButtonsShift
            y: -Math.max(0, page.bottomPanelsSize - (page.height - rightButtons.y - rightButtons.height)
                            + page.buttonMargin)
        }

        // Buttons pushed above the top controls are hidden. Only faded out, so that the
        // column keeps its layout.
        function fits(button) {
            return y + button.y + rightButtonsShift.y >= page.topControlsBottom + page.buttonMargin
        }

        MapButton {
            id: zoomInButton
            visible: appSettings.zoomButtons
            enabled: rightButtons.fits(zoomInButton)
            opacity: enabled ? 1.0 : 0.0
            source: "image://theme/icon-m-add"
            description: appInfo.localized("zoom_in")
            onClicked: map.zoomIn()
        }
        MapButton {
            id: zoomOutButton
            visible: appSettings.zoomButtons
            enabled: rightButtons.fits(zoomOutButton)
            opacity: enabled ? 1.0 : 0.0
            source: "image://theme/icon-m-remove"
            description: appInfo.localized("zoom_out")
            onClicked: map.zoomOut()
        }
        // A wider gap between the zoom buttons and my position in portrait.
        Item {
            visible: zoomOutButton.visible && page.isPortrait
            width: 1
            height: Theme.paddingLarge
        }
        // Indexed by MapItem.MyPositionMode.
        MapButton {
            id: myPositionButton
            readonly property var icons: ["", "ic_location_off", "ic_not_follow", "ic_follow", "ic_follow_and_rotate"]

            enabled: rightButtons.fits(myPositionButton)
            opacity: enabled ? 1.0 : 0.0
            description: appInfo.localized("core_my_position")

            source: map.myPositionMode === MapItem.PendingPosition
                    ? "" : Qt.resolvedUrl("../../icons/myposition/" + icons[map.myPositionMode] + ".svg")
            highlighted: map.myPositionMode === MapItem.Follow || map.myPositionMode === MapItem.FollowAndRotate
            busy: map.myPositionMode === MapItem.PendingPosition
            onClicked: map.switchMyPositionMode()
        }
    }

    // Spread over at most about six buttons' width, inset by another margin in portrait, like map_buttons_bottom
    // on Android.
    Row {
        id: bottomButtons
        readonly property real inset: page.buttonMargin * (page.isPortrait ? 2 : 1)

        visible: !page.navigating && !page.choosingPosition && !page.fullscreen
        // Positioned by x: switching between left and horizontalCenter anchors on rotation can leave both set.
        x: page.isPortrait ? (parent.width - width) / 2 : inset
        anchors {
            bottom: parent.bottom
            bottomMargin: page.buttonMargin
        }
        spacing: (Math.min(parent.width, 6 * menuButton.width) - 2 * inset - 4 * menuButton.width) / 3

        MapButton {
            square: true
            // The promos of the help button.
            source: Qt.resolvedUrl(appSettings.helpPromo === AppSettings.PromoCrowdfunding
                                   ? "../../icons/help/ic_crowdfunding.png"
                                   : appSettings.helpPromo === AppSettings.PromoNewYear
                                     ? "../../icons/help/ic_christmas_tree.svg" : "../../icons/help/logo.svg")
            description: appInfo.localized("help")
            onClicked: pageStack.push(Qt.resolvedUrl("HelpPage.qml"))
        }
        MapButton {
            square: true
            source: "image://theme/icon-m-search"
            description: appInfo.localized("search")
            onClicked: page.openSearch()
        }
        MapButton {
            square: true
            source: "image://theme/icon-m-favorite"
            description: appInfo.localized("bookmarks")
            onClicked: page.openBookmarks()
        }
        MapButton {
            id: menuButton
            square: true
            source: "image://theme/icon-m-menu"
            description: appInfo.localized("menu")
            onClicked: pageStack.push(Qt.resolvedUrl("MenuPage.qml"), { mapPage: page })

            // Map updates are waiting; without them a dot tells of a recording.
            // Placed like the Material badge with the offsets of MapButtonsController, relative to the button.
            Rectangle {
                // Not badgeLabel.visible: a child of a hidden item is never visible.
                readonly property bool counted: downloads.updateCount > 0

                visible: counted || map.trackRecording
                x: parent.width * 35 / 48 - width / 2
                y: parent.height * 9 / 48 - height / 2
                width: counted ? Math.max(height, badgeLabel.implicitWidth + Theme.paddingMedium) : height
                height: counted ? parent.height / 3 : parent.height / 8
                radius: height / 2
                color: Theme.highlightBackgroundColor

                Label {
                    id: badgeLabel
                    visible: parent.counted
                    anchors.centerIn: parent
                    text: downloads.updateCount
                    font.pixelSize: Theme.fontSizeTiny
                    font.bold: true
                }
            }
        }
    }

    // Under the navigation panel while navigating.
    MapButton {
        id: recordingButton
        anchors {
            top: page.navigating ? navigationTopPanel.top : parent.top
            topMargin: (page.navigating ? navigationTopPanel.rightHeight : page.topInset) + page.buttonMargin
            right: parent.right
            rightMargin: page.buttonMargin
        }
        visible: map.trackRecording && !page.fullscreen
        highlighted: true
        source: Qt.resolvedUrl("../../icons/menu/ic_track_recording_status.svg")
        description: appInfo.localized("track_recording")
        onClicked: recordingPanel.open = !recordingPanel.open

        SequentialAnimation on opacity {
            running: recordingButton.visible && Qt.application.active
            loops: Animation.Infinite
            NumberAnimation { to: 0.4; duration: 800 }
            NumberAnimation { to: 1.0; duration: 800 }
        }
    }

    Notification {
        id: recordingNotification
        appName: "Organic Maps"
        appIcon: appInfo.name
        // Without a position the recording stalls.
        summary: map.locationLost ? appInfo.localized("current_location_unknown_error_title")
                                  : appInfo.localized("track_recording")
        body: map.locationLost ? appInfo.localized("dialog_routing_location_turn_wifi") : ""
        remoteActions: [Notifications.openApp(),
                        Notifications.action("stop", appInfo.localized("track_recording_stop_and_save"),
                                             "stopTrackRecording")]
    }

    // Declared before the navigation panel, which covers them while its menu is pulled up.
    Column {
        anchors {
            left: parent.left
            bottom: navigationBottomPanel.top
            margins: page.buttonMargin
        }
        visible: page.navigating
        spacing: page.buttonMargin

        // The first tap offers categories to show on the map, the next opens search; a shown category or another
        // query is cleared.
        Row {
            spacing: page.buttonMargin

            MapButton {
                // A search left from before navigation clears with a tap, so it shows the clear icon like
                // ic_routing_search_off on Android.
                source: quickSearch.key !== "" ? quickSearch.icon(quickSearch.key)
                      : search.query !== "" ? "image://theme/icon-m-clear" : "image://theme/icon-m-search"
                monochrome: quickSearch.key === ""
                description: appInfo.localized("search")
                highlighted: search.query !== "" || quickSearch.expanded
                onClicked: {
                    if (search.query !== "") {
                        search.query = ""
                    } else if (quickSearch.expanded) {
                        quickSearch.expanded = false
                        page.openSearch()
                    } else {
                        quickSearch.expanded = true
                    }
                }
            }
            Repeater {
                model: quickSearch.expanded ? quickSearch.keys : []

                MapButton {
                    source: quickSearch.icon(modelData)
                    monochrome: false
                    description: appInfo.localized(modelData)
                    onClicked: quickSearch.start(modelData)
                }
            }
        }
        MapButton {
            source: "image://theme/icon-m-favorite"
            description: appInfo.localized("bookmarks")
            onClicked: page.openBookmarks()
        }
    }

    NavigationBottomPanel {
        id: navigationBottomPanel
        anchors {
            left: parent.left
            bottom: parent.bottom
        }
        width: page.isPortrait ? parent.width : Math.round(parent.width * 0.4)
        radius: page.isPortrait ? 0 : Theme.paddingLarge
        visible: page.navigating
        navigation: map.routing.navigation
        routing: map.routing
        androidDp: page.androidDp
        onStopClicked: map.routing.stopNavigation()
        onSettingsClicked: pageStack.push(Qt.resolvedUrl("SettingsPage.qml"), { routing: map.routing })
        onVoiceSettingsClicked: pageStack.push(Qt.resolvedUrl("VoicePage.qml"), { routing: map.routing })
    }

    QtObject {
        id: quickSearch

        // The SearchWheel categories on Android, as search::DisplayedCategories keys.
        readonly property var keys: ["category_fuel", "category_parking", "category_eat", "category_food",
                                     "category_atm"]
        property string key
        property string query
        property bool expanded

        function icon(categoryKey) {
            return Icons.category(categoryKey, Theme.colorScheme === Theme.LightOnDark)
        }
        function start(categoryKey) {
            var categories = search.categories()
            for (var i = 0; i < categories.length; ++i) {
                if (categories[i].key === categoryKey) {
                    search.searchCategory(categories[i].name, false /* addToHistory */)
                    key = categoryKey
                    query = search.query
                    break
                }
            }
            expanded = false
        }

        onExpandedChanged: if (expanded) collapseTimer.restart()
    }
    // The categories fold away by themselves, after the Android delay.
    Timer {
        id: collapseTimer
        interval: 5000
        onTriggered: quickSearch.expanded = false
    }
    // Another query, from the search page or a cleared one, ends the category search.
    Connections {
        target: search
        onQueryChanged: if (search.query !== quickSearch.query) quickSearch.key = ""
    }
    // Developer command, not translated like the core's debug commands.
    Connections {
        target: search
        onQueryChanged: {
            if (search.query.trim() !== "?navsim")
                return
            map.simulatingLocation = !map.simulatingLocation
            Toast.show(map.simulatingLocation ? "Location simulation on" : "Location simulation off")
        }
    }

    Rectangle {
        id: onMapDownloader

        readonly property var country: map.currentCountry
        readonly property bool busy: Downloads.busy(country.status)

        visible: !!country.countryId && !page.navigating && !page.choosingPosition && page.bottomPanelsSize === 0
                 && routePanel.visibleSize === 0
        anchors.centerIn: parent
        // As wide as the Android onmap_downloader with its 180dp button and margin_base padding, or wider for
        // the Silica button.
        width: Math.min(parent.width - 2 * Theme.horizontalPageMargin,
                        Math.max(180 * page.androidDp, downloadButton.implicitWidth) + 2 * downloaderColumn.x)
        height: downloaderColumn.height + 2 * downloaderColumn.x
        radius: 14 * page.androidDp
        color: Theme.rgba(Theme.overlayBackgroundColor, 0.9)

        Column {
            id: downloaderColumn
            x: 16 * page.androidDp
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width - 2 * x
            spacing: Theme.paddingMedium

            Label {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                text: onMapDownloader.country.name || ""
                font.pixelSize: Theme.fontSizeLarge
                color: Theme.highlightColor
                wrapMode: Text.Wrap
            }
            Label {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                text: onMapDownloader.country.status === CountriesModel.Error
                      ? appInfo.localized("country_status_download_failed")
                      : onMapDownloader.country.size || ""
                color: onMapDownloader.country.status === CountriesModel.Error ? Theme.errorColor
                                                                               : Theme.secondaryHighlightColor
            }
            ProgressBar {
                width: parent.width
                visible: onMapDownloader.busy
                indeterminate: onMapDownloader.country.status !== CountriesModel.Downloading
                value: onMapDownloader.country.progress || 0
            }
            Button {
                id: downloadButton
                anchors.horizontalCenter: parent.horizontalCenter
                text: onMapDownloader.busy ? appInfo.localized("cancel")
                    : onMapDownloader.country.status === CountriesModel.Error ? appInfo.localized("downloader_retry")
                    : appInfo.localized("downloader_download_map")
                onClicked: Downloads.toggle(pageStack, map, onMapDownloader.country)
            }
        }
    }

    // The core draws the cross in the middle of the map; this bar explains it.
    Rectangle {
        id: positionChooser

        property bool invalidPosition

        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
        height: chooserColumn.height + 2 * Theme.paddingMedium + page.topInset
        visible: page.choosingPosition
        color: Theme.overlayBackgroundColor
        onVisibleChanged: invalidPosition = false

        PanelBackground {
            anchors.fill: parent
        }

        IconButton {
            id: chooserCancel
            anchors {
                left: parent.left
                verticalCenter: chooserColumn.verticalCenter
            }
            icon.source: "image://theme/icon-m-cancel"
            onClicked: {
                if (page.choosingRoutePoint)
                    map.routing.cancelPick()
                map.stopChoosingPosition()
            }
        }

        Column {
            id: chooserColumn
            anchors {
                left: chooserCancel.right
                right: chooserDone.left
                bottom: parent.bottom
                bottomMargin: Theme.paddingMedium
            }
            spacing: Theme.paddingSmall

            Label {
                width: parent.width
                text: page.choosingApiPoint && page.apiAppName !== "" ? page.apiAppName
                    : appInfo.localized(page.choosingRoutePoint || page.choosingApiPoint ? "choose_on_map"
                                                                                       : "editor_add_select_location")
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeLarge
                truncationMode: TruncationMode.Fade
            }
            Label {
                width: parent.width
                text: {
                    if (positionChooser.invalidPosition)
                        return appInfo.localized("message_invalid_feature_position")
                    if (page.choosingRoutePoint || page.choosingApiPoint)
                        return appInfo.localized("choose_point_on_map_hint")
                    return appInfo.localized("editor_focus_map_on_location")
                }
                color: positionChooser.invalidPosition ? Theme.errorColor : Theme.secondaryHighlightColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.Wrap
            }
        }

        IconButton {
            id: chooserDone
            anchors {
                right: parent.right
                verticalCenter: chooserColumn.verticalCenter
            }
            icon.source: "image://theme/icon-m-acknowledge"
            onClicked: {
                // Sailfish has no app results: the position is copied, and passed to the app's back link if any.
                if (page.choosingApiPoint) {
                    var chosen = map.confirmChosenPosition(false)
                    var ll = chosen[0].toFixed(6) + "," + chosen[1].toFixed(6)
                    ClipboardHelper.copy(ll)
                    if (page.apiBackUrl !== "")
                        Qt.openUrlExternally(page.apiBackUrl + (page.apiBackUrl.indexOf("?") >= 0 ? "&" : "?")
                                             + "ll=" + ll)
                    return
                }
                if (page.choosingRoutePoint) {
                    var point = map.confirmChosenPosition(false)
                    map.routing.pickPosition(point[0], point[1])
                    return
                }
                var position = map.confirmChosenPosition()
                positionChooser.invalidPosition = position.length === 0
                if (!positionChooser.invalidPosition)
                    pageStack.push(Qt.resolvedUrl("CategoryPage.qml"), { lat: position[0], lon: position[1] })
            }
        }
    }

    // Lives with the map so the last query and its results come back when search is reopened.
    SearchModel {
        id: search
        highlightColor: Theme.highlightColor
    }

    PlacePagePanel {
        id: placePagePanel
        placePage: map.placePage
        routing: map.routing
        map: page.mapItem
        onAddPlaceClicked: map.startChoosingPosition(false)
        onAddBusinessClicked: map.startChoosingPosition(true)
        onDirectionClicked: directionOverlay.shown = true
        onClosedByUser: {
            if (page.placeFromSearch) {
                forgetSearchResult.stop()
                page.placeFromSearch = false
                page.openSearch()
            }
        }
    }

    TrackRecordingPanel {
        id: recordingPanel
        map: page.mapItem
    }

    RoutePanel {
        id: routePanel
        routing: map.routing
        placePage: map.placePage
        onSearchClicked: page.openSearch()
        onBookmarksClicked: page.openBookmarks()
        onChooseOnMapClicked: {
            page.choosingRoutePoint = true
            map.startChoosingPosition(false)
        }
    }

    MapPanel {
        id: layersPanel
        closesOnPressOutside: true

        Item {
            width: parent.width
            height: closeButton.height

            Label {
                anchors {
                    left: parent.left
                    leftMargin: Theme.horizontalPageMargin
                    right: closeButton.left
                    verticalCenter: parent.verticalCenter
                }
                text: appInfo.localized("layers_title")
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeLarge
                truncationMode: TruncationMode.Fade
            }

            IconButton {
                id: closeButton
                anchors.right: parent.right
                anchors.rightMargin: Theme.paddingMedium
                icon.source: "image://theme/icon-m-cancel"
                onClicked: layersPanel.open = false
            }
        }

        Row {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * x

            Repeater {
                id: layersRepeater
                // Order and labels of the Android layers sheet; Satellite once a tile server is set.
                model: {
                    var layers = [
                        { layer: MapItem.Outdoors, icon: "ic_layers_outdoors",
                          text: appInfo.localized("button_layer_outdoor") },
                        { layer: MapItem.Isolines, icon: "ic_layers_isoline",
                          text: appInfo.localized("button_layer_isolines") },
                        { layer: MapItem.Hiking, icon: "ic_layers_hiking",
                          text: appInfo.localized("button_layer_hiking") },
                        { layer: MapItem.Cycling, icon: "ic_layers_cycling",
                          text: appInfo.localized("button_layer_cycling") },
                        { layer: MapItem.Subway, icon: "ic_layers_subway",
                          text: appInfo.localized("button_layer_subway") }
                    ]
                    if (appSettings.bgTilesUrl !== "")
                        layers.push({ layer: MapItem.Satellite, icon: "ic_layers_satellite",
                                      text: appInfo.localized("button_layer_satellite") })
                    return layers
                }

                LayerButton {
                    width: parent.width / layersRepeater.count
                    source: Qt.resolvedUrl("../../icons/layers/" + modelData.icon
                                           + (page.mapIsDark ? "_night" : "") + ".svg")
                    text: modelData.text
                    checked: (map.enabledLayers & (1 << modelData.layer)) !== 0
                    isNew: (map.newLayers & (1 << modelData.layer)) !== 0
                    onClicked: {
                        var enable = !checked
                        map.setLayerEnabled(modelData.layer, enable)
                        if (!enable)
                            return
                        if (modelData.layer === MapItem.Isolines && map.isolinesNeedZoom())
                            Toast.show(appInfo.localized("isolines_toast_zooms_1_10"))
                        else if ((modelData.layer === MapItem.Hiking || modelData.layer === MapItem.Cycling)
                                 && map.needUpdateForRoutes())
                            Toast.showLong(appInfo.localized("routes_update_maps_text"))
                    }
                }
            }
        }
    }

    // Last, to cover the map and all panels.
    DirectionOverlay {
        id: directionOverlay
        placePage: map.placePage
    }
}
