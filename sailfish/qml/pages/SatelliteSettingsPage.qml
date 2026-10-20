import QtQuick 2.6
import Sailfish.Silica 1.0

// Changes apply when leaving the page, if the server URL is valid.
Page {
    id: page

    readonly property bool valid: !enabledSwitch.checked || appSettings.isWellFormedTilesUrl(urlField.text)

    // Applying reloads the tiles, so only when something changed.
    readonly property bool modified: enabledSwitch.checked !== appSettings.bgTilesEnabled
                                     || urlField.text.trim() !== appSettings.bgTilesUrl
                                     || cacheSlider.value !== appSettings.bgTilesCacheSize
                                     || opacitySlider.value !== appSettings.bgTilesOpacity

    allowedOrientations: Orientation.All
    onStatusChanged: {
        if (status === PageStatus.Deactivating && valid && modified)
            appSettings.setBackgroundTiles(enabledSwitch.checked, urlField.text, cacheSlider.value, opacitySlider.value)
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width

            PageHeader {
                title: appInfo.localized("pref_bg_tiles_title")
            }
            TextSwitch {
                id: enabledSwitch
                text: appInfo.localized("pref_bg_tiles_title")
                checked: appSettings.bgTilesEnabled
            }
            TextField {
                id: urlField
                width: parent.width
                enabled: enabledSwitch.checked
                label: appInfo.localized(page.valid ? "pref_bg_tiles_url_title" : "pref_bg_tiles_url_error")
                placeholderText: "https://…/{z}/{x}/{y}.jpg"
                text: appSettings.bgTilesUrl
                errorHighlight: !page.valid
                inputMethodHints: Qt.ImhUrlCharactersOnly | Qt.ImhNoPredictiveText
                EnterKey.iconSource: "image://theme/icon-m-enter-close"
                EnterKey.onClicked: focus = false
            }
            Slider {
                id: cacheSlider
                width: parent.width
                enabled: enabledSwitch.checked
                label: appInfo.localized("pref_bg_tiles_size_title")
                minimumValue: 1
                maximumValue: 1000
                stepSize: 1
                value: appSettings.bgTilesCacheSize
                valueText: Math.round(value)
            }
            Slider {
                id: opacitySlider
                width: parent.width
                enabled: enabledSwitch.checked
                label: appInfo.localized("pref_bg_tiles_opacity_title")
                minimumValue: 0
                maximumValue: 100
                stepSize: 1
                value: appSettings.bgTilesOpacity
                valueText: Math.round(value) + " %"
            }
            PageLabel {
                text: appInfo.localized("pref_bg_tiles_disclaimer")
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
            }
        }

        VerticalScrollDecorator {}
    }
}
