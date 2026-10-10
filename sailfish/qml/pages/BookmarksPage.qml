import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.Pickers 1.0
import app.organicmaps 1.0
import "bookmarks.js" as Bookmarks
import "navigation.js" as Navigation

Page {
    allowedOrientations: Orientation.All

    BookmarkCategoriesModel {
        id: categories
    }

    SilicaListView {
        id: listView
        anchors.fill: parent
        model: categories

        header: Column {
            width: listView.width

            PageHeader {
                title: appInfo.localized("bookmarks_and_tracks")
            }
            SectionHeader {
                text: appInfo.localized("bookmark_lists")
            }
        }

        delegate: ListItem {
            id: item

            contentHeight: Theme.itemSizeMedium
            onClicked: pageStack.push(Qt.resolvedUrl("BookmarkListPage.qml"),
                                      { listId: model.listId, title: model.name })
            menu: ContextMenu {
                MenuItem {
                    text: appInfo.localized("edit")
                    onClicked: pageStack.push(Qt.resolvedUrl("EditListDialog.qml"), { listId: model.listId })
                }
                MenuItem {
                    text: model.isVisible ? appInfo.localized("hide") : appInfo.localized("show")
                    onClicked: categories.setVisible(index, !model.isVisible)
                }
                MenuItem {
                    text: appInfo.localized("zoom_to_country")
                    onClicked: {
                        categories.setVisible(index, true)
                        categories.showOnMap(index)
                        Navigation.popToMap(pageStack)
                    }
                }
                MenuItem {
                    text: appInfo.localized("share")
                    onClicked: {
                        var listId = model.listId
                        Bookmarks.pickExportFormat(pageStack, function(type) {
                            bookmarksIO.exportList(listId, type)
                        })
                    }
                }
                MenuItem {
                    // The core keeps at least one list.
                    visible: listView.count > 1
                    text: appInfo.localized("delete")
                    onClicked: {
                        var listId = model.listId
                        item.remorseDelete(function() { categories.deleteList(listId) })
                    }
                }
            }

            VisibilityButton {
                id: visibilityButton
                anchors {
                    left: parent.left
                    leftMargin: Theme.horizontalPageMargin - Theme.paddingMedium
                    verticalCenter: parent.verticalCenter
                }
                shown: model.isVisible
                onClicked: categories.setVisible(index, !model.isVisible)
            }
            Column {
                anchors {
                    left: visibilityButton.right
                    leftMargin: Theme.paddingMedium
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin
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
                    // Counts after labels need no plural forms.
                    text: {
                        var bookmarks = appInfo.localized("bookmarks") + ": " + model.bookmarksCount
                        var tracks = appInfo.localized("tracks_title") + ": " + model.tracksCount
                        if (model.tracksCount === 0 && model.bookmarksCount > 0)
                            return bookmarks
                        if (model.bookmarksCount === 0 && model.tracksCount > 0)
                            return tracks
                        return appInfo.localized("comma_separated_pair", [bookmarks, tracks])
                    }
                    font.pixelSize: Theme.fontSizeSmall
                    color: item.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                }
            }
        }

        PullDownMenu {
            MenuItem {
                text: appInfo.localized("bookmarks_import")
                onClicked: pageStack.push(importPicker)
            }
            MenuItem {
                text: appInfo.localized("bookmarks_export")
                onClicked: bookmarksIO.exportAll()
            }
            MenuItem {
                text: appInfo.localized(categories.allInvisible ? "bookmark_lists_show_all" : "bookmark_lists_hide_all")
                onClicked: categories.setAllVisible(categories.allInvisible)
            }
            MenuItem {
                visible: categories.recentlyDeletedCount > 0
                text: appInfo.localized("bookmarks_recently_deleted")
                onClicked: pageStack.push(Qt.resolvedUrl("RecentlyDeletedPage.qml"), { categories: categories })
            }
            MenuItem {
                text: appInfo.localized("bookmarks_create_new_group")
                onClicked: pageStack.push(Qt.resolvedUrl("NewListDialog.qml"), {
                    createAction: function(name) { bookmarksIO.createList(name) }
                })
            }
        }

        VerticalScrollDecorator {}
    }

    Component {
        id: importPicker

        FilePickerPage {
            title: appInfo.localized("bookmarks_import")
            nameFilters: bookmarksIO.importFilters
            onSelectedContentPropertiesChanged: bookmarksIO.importFile(selectedContentProperties.filePath)
        }
    }
}
