import QtQuick 2.6
import Sailfish.Silica 1.0
import app.organicmaps 1.0
import "downloads.js" as Downloads
import "navigation.js" as Navigation
import "notices.js" as Toast

Page {
    id: page

    property alias parentId: countries.parentId
    property alias downloadedOnly: countries.downloadedOnly
    readonly property bool isRoot: countries.parentId === "Countries"
    readonly property bool searching: countries.query !== ""
    // The maps of this group left to download, unless all are downloaded.
    readonly property bool canBrowseAll: downloadedOnly && !searching
                                         && countries.parentStatus !== CountriesModel.OnDisk

    function noSpace() {
        Toast.showLong(appInfo.localized("downloader_no_space_title") + ". "
                       + appInfo.localized("downloader_no_space_message"))
    }
    function download(id) {
        if (!countries.hasSpaceFor(id))
            noSpace()
        else
            Downloads.start(pageStack, function() { countries.download(id) })
    }
    // Maps can't be deleted while navigating, and unsent edits would go with them.
    function remove(item, countryId) {
        if (pageStack.find(function(p) { return p.objectName === "mapPage" }).navigating) {
            Toast.showLong(appInfo.localized("downloader_delete_map_while_routing_dialog"))
        } else if (countries.hasUnsavedEdits(countryId)) {
            pageStack.push(Qt.resolvedUrl("MessageDialog.qml"), {
                title: appInfo.localized("downloader_delete_map"),
                message: appInfo.localized("downloader_delete_map_dialog"),
                acceptText: appInfo.localized("delete"),
                acceptAction: function() { countries.remove(countryId) }
            })
        } else {
            item.remorseDelete(function() { countries.remove(countryId) })
        }
    }

    function statusText(status, error, progress, name) {
        switch (status) {
        case CountriesModel.Downloading:
            return appInfo.localized("downloader_downloading") + " " + Math.round(progress * 100) + "%"
        case CountriesModel.Applying:
            return appInfo.localized("downloader_applying", [name])
        case CountriesModel.InQueue:
            return appInfo.localized("downloader_queued")
        case CountriesModel.Error:
            if (error === CountriesModel.NoInetConnection)
                return appInfo.localized("common_check_internet_connection_dialog")
            if (error === CountriesModel.OutOfMemFailed)
                return appInfo.localized("downloader_no_space_title")
            return appInfo.localized("country_status_download_failed")
        case CountriesModel.OnDiskOutOfDate:
            return appInfo.localized("downloader_status_outdated")
        default:
            return ""
        }
    }

    function browseAll() {
        pageStack.push(Qt.resolvedUrl("MapsPage.qml"), { parentId: countries.parentId, downloadedOnly: false })
    }

    function showOnMap(countryId) {
        countries.showOnMap(countryId)
        Navigation.popToMap(pageStack)
    }

    allowedOrientations: Orientation.All

    CountriesModel {
        id: countries
        downloadedOnly: true
    }

    SilicaListView {
        id: list
        anchors.fill: parent
        model: countries
        // Without a current item, which a reset by new results would give the focus, taking it from the search field.
        currentIndex: -1

        header: Column {
            width: list.width

            PageHeader {
                title: !page.isRoot ? countries.title
                                    : page.downloadedOnly ? appInfo.localized("download_maps")
                                                          : appInfo.localized("downloader_available_maps")
            }
            SearchField {
                width: parent.width
                visible: page.isRoot
                placeholderText: appInfo.localized("downloader_search_field_hint")
                inputMethodHints: Qt.ImhNoPredictiveText
                onTextChanged: countries.query = text
                EnterKey.iconSource: "image://theme/icon-m-enter-close"
                EnterKey.onClicked: focus = false
            }
        }

        section.property: "section"
        section.delegate: SectionHeader {
            text: section
        }

        delegate: ListItem {
            id: item

            readonly property bool busy: Downloads.busy(model.status)
            readonly property bool hasLocal: model.status === CountriesModel.OnDisk
                                             || model.status === CountriesModel.OnDiskOutOfDate
                                             || model.status === CountriesModel.Partly
            readonly property string status: page.statusText(model.status, model.error, model.progress, model.name)

            contentHeight: Theme.itemSizeMedium
            menu: contextMenu

            onClicked: {
                if (model.isGroup)
                    pageStack.push(Qt.resolvedUrl("MapsPage.qml"), {
                        parentId: model.countryId,
                        downloadedOnly: page.downloadedOnly && !page.searching
                    })
                else if (model.status === CountriesModel.NotDownloaded || model.status === CountriesModel.Error)
                    page.download(model.countryId)
                else if (model.status === CountriesModel.OnDisk)
                    page.showOnMap(model.countryId)
                else
                    openMenu()
            }

            Rectangle {
                id: statusIcon
                anchors {
                    left: parent.left
                    leftMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                width: Theme.iconSizeMedium + Theme.paddingSmall
                height: width
                radius: width / 2
                color: item.hasLocal && !model.isGroup ? Theme.rgba(Theme.primaryColor, 0.1)
                                                       : Theme.rgba(Theme.highlightBackgroundColor, 0.5)

                Icon {
                    anchors.centerIn: parent
                    visible: !item.busy
                    sourceSize: Qt.size(Theme.iconSizeSmallPlus, Theme.iconSizeSmallPlus)
                    source: model.isGroup ? "image://theme/icon-m-file-folder"
                          : model.status === CountriesModel.OnDiskOutOfDate ? "image://theme/icon-m-refresh"
                          : model.status === CountriesModel.Error ? "image://theme/icon-m-reload"
                          : item.hasLocal ? "image://theme/icon-m-acknowledge"
                          : "image://theme/icon-m-cloud-download"
                }
                ProgressCircle {
                    anchors.fill: parent
                    visible: item.busy
                    value: model.progress
                    progressColor: Theme.highlightColor
                    backgroundColor: Theme.rgba(Theme.highlightDimmerColor, 0.5)
                }
            }

            Column {
                anchors {
                    left: statusIcon.right
                    leftMargin: Theme.paddingLarge
                    right: sizeLabel.left
                    rightMargin: Theme.paddingMedium
                    verticalCenter: parent.verticalCenter
                }

                Label {
                    width: parent.width
                    text: page.searching && model.foundName !== "" ? model.foundName : model.name
                    truncationMode: TruncationMode.Fade
                    color: item.highlighted ? Theme.highlightColor : Theme.primaryColor
                }
                Label {
                    width: parent.width
                    text: item.status !== "" ? item.status
                        : model.isGroup ? appInfo.localized("downloader_status_maps") + ": "
                                          + appInfo.localized("downloader_of", [model.localMapsCount, model.mapsCount])
                        : page.searching ? model.parentName
                        : model.description
                    visible: text !== ""
                    font.pixelSize: Theme.fontSizeExtraSmall
                    truncationMode: TruncationMode.Fade
                    color: model.status === CountriesModel.Error ? Theme.errorColor
                                                                 : item.highlighted ? Theme.secondaryHighlightColor
                                                                                    : Theme.secondaryColor
                }
            }

            Label {
                id: sizeLabel
                anchors {
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                text: appInfo.formatSize(page.downloadedOnly && !page.searching && !item.busy ? model.localSize
                                                                                              : model.size)
                font.pixelSize: Theme.fontSizeSmall
                color: item.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
            }

            Component {
                id: contextMenu
                ContextMenu {
                    MenuItem {
                        text: appInfo.localized("downloader_download_map")
                        visible: model.status === CountriesModel.NotDownloaded || model.status === CountriesModel.Partly
                        onClicked: page.download(model.countryId)
                    }
                    MenuItem {
                        text: appInfo.localized("downloader_update_map")
                        visible: model.status === CountriesModel.OnDiskOutOfDate
                        onClicked: page.download(model.countryId)
                    }
                    MenuItem {
                        text: appInfo.localized("cancel")
                        visible: item.busy || model.status === CountriesModel.Error
                        onClicked: countries.cancel(model.countryId)
                    }
                    MenuItem {
                        text: appInfo.localized("zoom_to_country")
                        visible: model.present
                        onClicked: page.showOnMap(model.countryId)
                    }
                    MenuItem {
                        text: appInfo.localized("delete")
                        visible: model.deletable
                                 && (item.hasLocal || model.status === CountriesModel.Error && model.present)
                        onClicked: page.remove(item, model.countryId)
                    }
                }
            }
        }

        // Browsing the maps left to download, and the actions on all maps of the group, which in the list of all
        // maps only show below the root.
        PullDownMenu {
            id: listMenu

            readonly property bool groupActions: !page.searching && (page.downloadedOnly || !page.isRoot)
            // Not derived from the items' visible: a child of a hidden menu is never visible.
            readonly property bool showCancelAll: groupActions && Downloads.busy(countries.parentStatus)
            readonly property bool canUpdateAll: countries.parentStatus === CountriesModel.OnDiskOutOfDate
            // Always in the downloaded maps, greyed out without updates, so that updating can be found.
            readonly property bool showUpdateAll: groupActions && (canUpdateAll || page.downloadedOnly)
            readonly property bool showRetryAll: groupActions && countries.parentStatus === CountriesModel.Error
            readonly property bool showDownloadAll: groupActions && !page.downloadedOnly
                                                    && (countries.parentStatus === CountriesModel.NotDownloaded
                                                        || countries.parentStatus === CountriesModel.Partly)

            visible: showCancelAll || showUpdateAll || showRetryAll || showDownloadAll || page.canBrowseAll
            MenuItem {
                visible: listMenu.showCancelAll
                text: appInfo.localized("downloader_cancel_all")
                onClicked: countries.cancelAll()
            }
            MenuItem {
                visible: listMenu.showUpdateAll
                enabled: listMenu.canUpdateAll
                text: appInfo.localized("downloader_update_all_button")
                      + (listMenu.canUpdateAll ? " (" + countries.updateSize + ")" : "")
                onClicked: {
                    if (!countries.hasSpaceToUpdate(countries.parentId))
                        page.noSpace()
                    else
                        Downloads.start(pageStack, function() { countries.updateAll() })
                }
            }
            MenuItem {
                visible: listMenu.showRetryAll
                text: appInfo.localized("downloader_retry")
                onClicked: Downloads.start(pageStack, function() { countries.download(countries.parentId) })
            }
            MenuItem {
                visible: listMenu.showDownloadAll
                text: appInfo.localized("downloader_download_all_button")
                onClicked: page.download(countries.parentId)
            }
            MenuItem {
                visible: page.canBrowseAll
                text: appInfo.localized("download_maps")
                onClicked: page.browseAll()
            }
        }
        ViewPlaceholder {
            id: noMapsPlaceholder
            enabled: list.count === 0 && page.downloadedOnly && !page.searching
            text: appInfo.localized("downloader_no_downloaded_maps_title")
            hintText: appInfo.localized("downloader_no_downloaded_maps_message")
        }
        ViewPlaceholder {
            enabled: list.count === 0 && page.searching
            text: appInfo.localized("search_not_found")
            hintText: appInfo.localized("search_not_found_query")
        }
        PositionMapButtons {
            anchors {
                bottom: parent.bottom
                bottomMargin: Theme.itemSizeLarge
            }
            width: parent.width
            visible: noMapsPlaceholder.enabled
            country: downloads.positionMap
            onDownloadClicked: page.download(countryId)
        }

        VerticalScrollDecorator {}
    }
}
