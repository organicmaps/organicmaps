import QtQuick 2.6
import Sailfish.Silica 1.0
import Nemo.KeepAlive 1.2
import app.organicmaps 1.0
import "notices.js" as Toast

Page {
    id: page

    // For children, where "map: map" would bind to their own property.
    readonly property QtObject mapItem: map
    readonly property bool mapIsDark: {
        switch (appSettings.mapAppearance) {
        case AppSettings.AppearanceDark: return true
        case AppSettings.AppearanceAuto: return Theme.colorScheme === Theme.LightOnDark
        }
        return false
    }
    // The lowest of the controls along the top right edge, or the padded camera notch in portrait; the compass
    // goes below it.
    readonly property real topControlsBottom: topInset + buttonMargin
    // Silica keeps landscape pages clear of the camera notch.
    readonly property real topInset: page.orientation === Orientation.Portrait ? Screen.topCutout.height : 0
    readonly property real buttonMargin: Theme.paddingMedium

    objectName: "mapPage"
    allowedOrientations: Orientation.All
    backNavigation: false

    onMapIsDarkChanged: appSettings.applyMapAppearance(mapIsDark)
    Component.onCompleted: appSettings.applyMapAppearance(mapIsDark)

    DisplayBlanking {
        preventBlanking: appSettings.keepScreenOn && Qt.application.active
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
        // Keeps the scale line and attribution above the bottom row with its padding.
        bottomWidgetsOffset: bottomButtons.height + 2 * page.buttonMargin
        topWidgetsOffset: page.topControlsBottom
    }

    Connections {
        target: map
        onEngineCreated: urlHandler.processPending()
        onNotice: Toast.showLong(message)
        onIsolinesNeedMaps: Toast.showLong(appInfo.localized("isolines_activation_error_dialog"))
    }

    MapButton {
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
        anchors {
            right: parent.right
            rightMargin: page.buttonMargin
            // Portrait: map_buttons_bottom_margin plus the padding from the bottom edge. Landscape: my position
            // is in the bottom right corner.
            bottom: parent.bottom
            bottomMargin: page.isPortrait ? 3 * myPositionButton.size : page.buttonMargin
        }
        spacing: page.buttonMargin

        // Buttons pushed above the top controls are hidden. Only faded out, so that the
        // column keeps its layout.
        function fits(button) {
            return y + button.y >= page.topControlsBottom + page.buttonMargin
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

        // Positioned by x: switching between left and horizontalCenter anchors on rotation can leave both set.
        x: page.isPortrait ? (parent.width - width) / 2 : inset
        anchors {
            bottom: parent.bottom
            bottomMargin: page.buttonMargin
        }
        spacing: Math.min(parent.width, 6 * menuButton.width) - 2 * inset - 2 * menuButton.width

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
            id: menuButton
            square: true
            source: "image://theme/icon-m-menu"
            description: appInfo.localized("menu")
            onClicked: pageStack.push(Qt.resolvedUrl("MenuPage.qml"), { mapPage: page })
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
}
