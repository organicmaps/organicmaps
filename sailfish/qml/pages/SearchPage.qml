import QtQuick 2.6
import Sailfish.Silica 1.0
import app.organicmaps 1.0
import "colors.js" as Colors
import "downloads.js" as Downloads
import "icons.js" as Icons

Page {
    id: page

    property SearchModel search
    // The MapItem.
    property QtObject map
    signal resultActivated()

    function showOnMap() {
        search.showOnMap()
        pageStack.pop()
    }

    allowedOrientations: Orientation.All
    onStatusChanged: {
        if (status === PageStatus.Active && search.query === "")
            searchField.forceActiveFocus()
    }

    SearchField {
        id: searchField
        width: parent.width
        // Clear the camera notch in portrait, like the Silica PageHeader does.
        y: page.orientation === Orientation.Portrait ? Screen.topCutout.height : 0
        placeholderText: appInfo.localized("search")
        text: search.query
        inputMethodHints: Qt.ImhNoPredictiveText
        onTextChanged: search.query = text
        EnterKey.iconSource: "image://theme/icon-m-enter-accept"
        EnterKey.enabled: resultsView.count > 0
        EnterKey.onClicked: page.showOnMap()

        // Typing breaks the text binding, so follow queries set by categories, history and suggestions.
        Connections {
            target: search
            onQueryChanged: if (searchField.text !== search.query) searchField.text = search.query
        }
    }

    SilicaListView {
        id: categoriesView
        anchors {
            top: searchField.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        clip: true
        visible: search.query === ""
        model: search.categories()

        header: Column {
            width: categoriesView.width

            Column {
                readonly property var country: downloads.positionMap

                width: parent.width
                visible: downloads.noMaps
                spacing: Theme.paddingMedium
                bottomPadding: Theme.paddingLarge

                PageLabel {
                    text: appInfo.localized("search_without_internet_advertisement")
                    color: Theme.highlightColor
                }
                PageLabel {
                    visible: !parent.country.countryId
                    text: appInfo.localized("unknown_current_position")
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.secondaryHighlightColor
                }
                PositionMapButtons {
                    width: parent.width
                    country: parent.country
                    onDownloadClicked: Downloads.start(pageStack, function() { page.map.downloadMap(countryId) })
                }
            }

            SectionHeader {
                text: appInfo.localized("history")
                visible: historyRepeater.count > 0
            }
            Repeater {
                id: historyRepeater
                model: appSettings.searchHistory ? search.history : []

                MenuRow {
                    icon: "image://theme/icon-m-history"
                    text: modelData
                    onClicked: search.query = modelData
                }
            }
            MenuRow {
                visible: historyRepeater.count > 0
                icon: "image://theme/icon-m-cancel"
                text: appInfo.localized("clear_search")
                onClicked: Remorse.popupAction(page, appInfo.localized("clear_search"),
                                               function() { search.clearHistory() })
            }
            SectionHeader {
                text: appInfo.localized("categories")
            }
        }

        delegate: MenuRow {
            icon: Icons.category(modelData.key, Theme.colorScheme === Theme.LightOnDark)
            monochrome: false
            text: modelData.name
            onClicked: search.searchCategory(modelData.name)
        }

        VerticalScrollDecorator {}
    }

    SilicaListView {
        id: resultsView
        anchors.fill: categoriesView
        clip: true
        visible: !categoriesView.visible
        model: search

        PullDownMenu {
            visible: resultsView.count > 0

            MenuItem {
                text: appInfo.localized("search_show_on_map")
                onClicked: page.showOnMap()
            }
        }

        delegate: ListItem {
            contentHeight: column.height + 2 * Theme.paddingMedium
            onClicked: {
                if (search.activate(index)) {
                    page.resultActivated()
                    pageStack.pop()
                }
            }

            Column {
                id: column
                x: Theme.horizontalPageMargin
                y: Theme.paddingMedium
                width: parent.width - 2 * x

                Row {
                    width: parent.width
                    spacing: Theme.paddingSmall

                    Icon {
                        id: suggestIcon
                        visible: model.suggest
                        anchors.verticalCenter: parent.verticalCenter
                        width: Theme.iconSizeExtraSmall
                        height: width
                        sourceSize: Qt.size(width, height)
                        source: "image://theme/icon-m-search"
                    }
                    Label {
                        width: parent.width - (suggestIcon.visible ? suggestIcon.width + parent.spacing : 0)
                        text: model.name
                        textFormat: Text.StyledText
                        truncationMode: TruncationMode.Fade
                        font.italic: model.suggest
                    }
                }
                ResultLine {
                    text: model.description
                    trailingText: model.openStatus
                    trailingColor: model.openState === SearchModel.Open ? Colors.open
                                 : model.openState === SearchModel.ClosingSoon ? Colors.closingSoon : Colors.closed
                }
                ResultLine {
                    text: model.address
                    textFormat: Text.StyledText
                    trailingText: model.distance
                    trailingColor: Theme.highlightColor
                }
            }
        }

        ViewPlaceholder {
            enabled: resultsView.count === 0 && !search.searching
            text: appInfo.localized("search_not_found")
            hintText: appInfo.localized("search_not_found_query")
        }

        VerticalScrollDecorator {}
    }

    BusyIndicator {
        anchors.centerIn: resultsView
        size: BusyIndicatorSize.Large
        running: search.searching && resultsView.count === 0
    }
}
