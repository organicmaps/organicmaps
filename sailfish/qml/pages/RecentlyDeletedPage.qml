import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: page

    // BookmarkCategoriesModel.
    property QtObject categories
    property var lists: categories.recentlyDeleted()

    function reload() { lists = categories.recentlyDeleted() }
    function allPaths() { return lists.map(function(list) { return list.path }) }

    allowedOrientations: Orientation.All
    onListsChanged: if (lists.length === 0 && status === PageStatus.Active) pageStack.pop()

    SilicaListView {
        anchors.fill: parent
        model: page.lists

        header: PageHeader {
            title: appInfo.localized("bookmarks_recently_deleted")
        }

        delegate: ListItem {
            id: item

            contentHeight: Theme.itemSizeMedium
            onClicked: openMenu()
            menu: ContextMenu {
                MenuItem {
                    text: appInfo.localized("recover")
                    onClicked: {
                        categories.recoverDeleted([modelData.path])
                        page.reload()
                    }
                }
                MenuItem {
                    text: appInfo.localized("delete")
                    onClicked: item.remorseDelete(function() {
                        categories.deleteForever([modelData.path])
                        page.reload()
                    })
                }
            }

            Column {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                anchors.verticalCenter: parent.verticalCenter

                Label {
                    width: parent.width
                    text: modelData.name
                    truncationMode: TruncationMode.Fade
                    highlighted: item.highlighted
                }
                Label {
                    width: parent.width
                    visible: !isNaN(modelData.date)
                    text: Format.formatDate(modelData.date, Formatter.DateMedium)
                    font.pixelSize: Theme.fontSizeSmall
                    color: item.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                }
            }
        }

        PullDownMenu {
            MenuItem {
                text: appInfo.localized("delete_all")
                onClicked: Remorse.popupAction(page, appInfo.localized("delete_all"), function() {
                    categories.deleteForever(page.allPaths())
                    page.reload()
                })
            }
            MenuItem {
                text: appInfo.localized("recover_all")
                onClicked: {
                    categories.recoverDeleted(page.allPaths())
                    page.reload()
                }
            }
        }

        VerticalScrollDecorator {}
    }
}
