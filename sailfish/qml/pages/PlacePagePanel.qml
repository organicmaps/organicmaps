import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.Share 1.0
import app.organicmaps 1.0
import "clipboard.js" as ClipboardHelper
import "colors.js" as Colors
import "downloads.js" as Downloads
import "share.js" as Share

// Not modal, so the map stays usable.
MapPanel {
    id: panel

    // MapItem.placePage; the uncreatable C++ type can't be a property type in Qt 5.6.
    property QtObject placePage
    // The MapItem.
    property QtObject map
    property bool hoursExpanded
    property bool wikiExpanded
    // A line count for no limit: maximumLineCount can't be reset to unlimited on Qt 5.6.
    readonly property int unlimitedLines: 10000

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
        onChanged: panel.open = panel.placePage.open
        onSelected: {
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
                        width: parent.width
                        text: placePage.title
                        font.pixelSize: Theme.fontSizeLarge
                        color: Theme.highlightColor
                        wrapMode: Text.Wrap

                        MouseArea {
                            anchors.fill: parent
                            onPressAndHold: ClipboardHelper.copy(placePage.title)
                        }
                    }
                    Label {
                        width: parent.width
                        visible: text !== ""
                        text: placePage.secondaryTitle
                        color: Theme.highlightColor
                        wrapMode: Text.Wrap
                    }
                    Label {
                        width: parent.width
                        visible: text !== ""
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
                    icon.source: "image://theme/icon-m-share"
                    onClicked: shareAction.trigger()
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

            // The OpenStreetMap description is plain text.
            PageLabel {
                visible: text !== ""
                text: placePage.osmDescription
                textFormat: Text.PlainText
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryHighlightColor
                bottomPadding: Theme.paddingMedium
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
            MenuRow {
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
                icon: "image://theme/icon-m-shortcut"
                text: appInfo.localized("open_in_app")
                onClicked: Qt.openUrlExternally(placePage.geoUri)
            }
        }

        VerticalScrollDecorator {}
    }

    Row {
        id: actions
        width: parent.width

        PlaceAction {
            visible: placePage.apiBackUrl !== ""
            width: actions.width
            icon: "image://theme/icon-m-back"
            text: appInfo.localized("back")
            onClicked: Qt.openUrlExternally(placePage.apiBackUrl)
        }
    }

    ShareAction {
        id: shareAction
        mimeType: "text/plain"
        resources: [Share.textResource(placePage.shareText, placePage.title)]
    }
}
