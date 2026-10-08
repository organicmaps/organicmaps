import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.Share 1.0
import app.organicmaps 1.0
import "bookmarks.js" as Bookmarks
import "navigation.js" as Navigation
import "share.js" as Share

Page {
    id: page

    property alias listId: bookmarks.listId
    // Shown until the model has the list.
    property string title
    property bool selecting
    property var selected: []
    // Of the whole list, whatever the search shows.
    readonly property bool hasItems: bookmarks.bookmarksCount + bookmarks.tracksCount > 0

    function toggleSelected(row) {
        var selected = page.selected.slice()
        var i = selected.indexOf(row)
        if (i >= 0)
            selected.splice(i, 1)
        else
            selected.push(row)
        page.selected = selected
    }

    // Here rather than in the item menu: the menu is gone when these callbacks run, and on Qt 5.6 its context no
    // longer resolves bookmarksIO.
    function deleteItems(item, items) {
        item.remorseDelete(function() { bookmarksIO.deleteItems(items) })
    }

    function shareTrack(trackId) {
        Bookmarks.pickExportFormat(pageStack, function(type) { bookmarksIO.exportTrack(trackId, type) })
    }

    // -1 is the default order.
    function sortingName(type) {
        switch (type) {
        case BookmarksModel.ByType: return appInfo.localized("sort_type")
        case BookmarksModel.ByDistance: return appInfo.localized("sort_distance")
        case BookmarksModel.ByTime: return appInfo.localized("sort_date")
        case BookmarksModel.ByName: return appInfo.localized("sort_name")
        default: return appInfo.localized("sort_default")
        }
    }

    allowedOrientations: Orientation.All
    onSelectingChanged: selected = []

    BookmarksModel {
        id: bookmarks
        // Rows change meaning with the list.
        onModelReset: page.selected = []
    }

    SilicaListView {
        id: listView
        anchors.fill: parent
        model: bookmarks

        header: Column {
            width: listView.width

            PageHeader {
                title: page.selecting ? appInfo.localized("select") + " (" + page.selected.length + ")"
                     : bookmarks.name !== "" ? bookmarks.name : page.title
            }
            PageLabel {
                visible: text !== ""
                text: bookmarks.descriptionText
                textFormat: Text.StyledText
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryHighlightColor
                bottomPadding: Theme.paddingMedium
                linkColor: Theme.highlightColor
                onLinkActivated: Qt.openUrlExternally(link)
            }
            SearchField {
                width: parent.width
                visible: page.hasItems
                placeholderText: appInfo.localized("search_in_the_list")
                onTextChanged: bookmarks.filter = text
                EnterKey.iconSource: "image://theme/icon-m-enter-close"
                EnterKey.onClicked: focus = false
            }
        }

        section {
            property: "block"
            delegate: SectionHeader {
                text: section
            }
        }

        delegate: ListItem {
            id: item

            readonly property bool selected: page.selected.indexOf(index) >= 0

            contentHeight: Theme.itemSizeMedium
            highlighted: down || selected || menuOpen
            menu: page.selecting ? null : itemMenu
            onClicked: {
                if (page.selecting) {
                    page.toggleSelected(index)
                    return
                }
                bookmarks.showOnMap(index)
                Navigation.popToMap(pageStack)
            }

            // The check mark is in theme colors, which any item color could hide.
            ColorDot {
                id: colorDot
                x: Theme.horizontalPageMargin
                anchors.verticalCenter: parent.verticalCenter
                color: item.selected ? Theme.highlightBackgroundColor : model.color || "transparent"

                Icon {
                    anchors.centerIn: parent
                    visible: item.selected
                    source: "image://theme/icon-s-accept"
                    color: Theme.primaryColor
                }
            }
            VisibilityButton {
                id: eyeButton
                anchors {
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin - Theme.paddingMedium
                    verticalCenter: parent.verticalCenter
                }
                visible: model.isTrack && !page.selecting
                shown: !!model.isVisible
                onClicked: bookmarks.setTrackVisible(index, !model.isVisible)
            }

            Column {
                anchors {
                    left: colorDot.right
                    leftMargin: Theme.paddingLarge
                    right: eyeButton.visible ? eyeButton.left : parent.right
                    rightMargin: eyeButton.visible ? 0 : Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }

                Label {
                    width: parent.width
                    text: model.name
                    truncationMode: TruncationMode.Fade
                    highlighted: item.highlighted
                }
                Label {
                    width: parent.width
                    visible: text !== ""
                    text: model.distance && model.type ? model.distance + " • " + model.type
                                                       : model.distance || model.type || ""
                    font.pixelSize: Theme.fontSizeSmall
                    color: item.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                    truncationMode: TruncationMode.Fade
                }
            }

            Component {
                id: itemMenu

                ContextMenu {
                    MenuItem {
                        visible: !model.isTrack
                        text: appInfo.localized("share")
                        onClicked: {
                            bookmarkShare.resources = [Share.textResource(bookmarks.shareText(index), model.name)]
                            bookmarkShare.trigger()
                        }
                    }
                    MenuItem {
                        text: appInfo.localized("edit")
                        onClicked: {
                            // A sorted list can still show an item deleted meanwhile.
                            var items = bookmarks.items([index])
                            if (items.length > 0)
                                pageStack.push(Qt.resolvedUrl("EditBookmarkPage.qml"),
                                               { itemId: items[0].id, isTrack: items[0].isTrack })
                        }
                    }
                    MenuItem {
                        visible: model.isTrack
                        text: appInfo.localized(model.isVisible ? "hide_track" : "show_track")
                        onClicked: bookmarks.setTrackVisible(index, !model.isVisible)
                    }
                    // A track is shared as a file.
                    MenuItem {
                        visible: model.isTrack
                        text: appInfo.localized("share")
                        onClicked: page.shareTrack(model.itemId)
                    }
                    MenuItem {
                        text: appInfo.localized("delete")
                        onClicked: page.deleteItems(item, bookmarks.items([index]))
                    }
                }
            }
        }

        PullDownMenu {
            MenuItem {
                visible: page.selecting
                text: appInfo.localized("cancel")
                onClicked: page.selecting = false
            }
            MenuItem {
                visible: page.selecting
                text: appInfo.localized(page.selected.length < listView.count ? "select_all" : "deselect_all")
                onClicked: {
                    var all = []
                    if (page.selected.length < listView.count)
                        for (var i = 0; i < listView.count; ++i)
                            all.push(i)
                    page.selected = all
                }
            }
            MenuItem {
                visible: !page.selecting && listView.count > 0
                text: appInfo.localized("select")
                onClicked: page.selecting = true
            }
            MenuItem {
                // The core keeps at least one list.
                visible: !page.selecting && bookmarksIO.lists.length > 1
                text: appInfo.localized("delete_list")
                onClicked: Remorse.popupAction(page, appInfo.localized("delete_list"), function() {
                    bookmarks.deleteList()
                    // Not pop(): a page opened meanwhile would be closed instead of this one.
                    Navigation.popPage(pageStack, page)
                })
            }
            MenuItem {
                visible: !page.selecting && page.hasItems
                text: appInfo.localized("zoom_to_country")
                onClicked: {
                    bookmarks.showListOnMap()
                    Navigation.popToMap(pageStack)
                }
            }
            MenuItem {
                visible: !page.selecting && page.hasItems
                text: appInfo.localized("share")
                onClicked: Bookmarks.pickExportFormat(pageStack, function(type) {
                    bookmarksIO.exportList(bookmarks.listId, type)
                })
            }
            MenuItem {
                visible: !page.selecting
                text: appInfo.localized("edit")
                onClicked: pageStack.push(Qt.resolvedUrl("EditListDialog.qml"), { listId: bookmarks.listId })
            }
            MenuItem {
                // Search results come in their own order.
                visible: !page.selecting && page.hasItems && bookmarks.filter === ""
                         && bookmarks.sortingTypes.length > 0
                text: appInfo.localized("sort_bookmarks")
                onClicked: {
                    var types = [-1].concat(bookmarks.sortingTypes)
                    pageStack.push(Qt.resolvedUrl("ListPickerPage.qml"), {
                        title: appInfo.localized("sort_bookmarks"),
                        items: types.map(function(type) {
                            return { name: page.sortingName(type), selected: type === bookmarks.sortingType }
                        }),
                        picked: function(index) { bookmarks.sortingType = types[index] }
                    })
                }
            }
        }

        ViewPlaceholder {
            enabled: listView.count === 0
            text: appInfo.localized(page.hasItems ? "search_not_found" : "bookmarks_empty_list_title")
            hintText: appInfo.localized(page.hasItems ? "search_not_found_query" : "bookmarks_empty_list_message")
        }

        VerticalScrollDecorator {}
    }

    DockedPanel {
        width: parent.width
        height: selectionButtons.height + 2 * Theme.paddingLarge
        dock: Dock.Bottom
        open: page.selecting && page.selected.length > 0

        ButtonLayout {
            id: selectionButtons
            anchors.centerIn: parent
            width: parent.width

            Button {
                text: appInfo.localized("move")
                onClicked: {
                    var items = bookmarks.items(page.selected)
                    Bookmarks.pickList(pageStack, bookmarks.listId, function(id) {
                        // Picking the current list keeps the selection.
                        if (id === bookmarks.listId)
                            return
                        bookmarks.moveItems(items, id)
                        page.selecting = false
                    })
                }
            }
            Button {
                text: appInfo.localized("change_color")
                onClicked: {
                    var items = bookmarks.items(page.selected)
                    pageStack.push(Qt.resolvedUrl("ColorPickerPage.qml"), {
                        title: appInfo.localized("change_color"),
                        chosen: function(colorIndex) {
                            bookmarks.setItemsColor(items, colorIndex)
                            page.selecting = false
                        }
                    })
                }
            }
            Button {
                text: appInfo.localized("delete")
                onClicked: {
                    var items = bookmarks.items(page.selected)
                    Remorse.popupAction(page, appInfo.localized("delete"), function() {
                        bookmarksIO.deleteItems(items)
                        page.selecting = false
                    })
                }
            }
        }
    }

    ShareAction {
        id: bookmarkShare
        mimeType: "text/plain"
    }
}
