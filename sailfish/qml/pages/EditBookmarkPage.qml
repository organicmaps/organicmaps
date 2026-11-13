import QtQuick 2.6
import Sailfish.Silica 1.0
import app.organicmaps 1.0
import "bookmarks.js" as Bookmarks
import "navigation.js" as Navigation

Dialog {
    id: dialog

    // MapPage holds links back while the user types here.
    property bool keepsUserInput: true

    property var itemId
    property bool isTrack
    property int colorIndex: editor.colorIndex
    property var listId: editor.listId

    allowedOrientations: Orientation.All
    // A bookmark without a name shows its feature type; a track needs one.
    canAccept: !isTrack || nameField.text.trim() !== ""
    onAccepted: editor.save(nameField.text, notesArea.text, colorIndex, listId, visibilitySwitch.checked)
    Component.onCompleted: {
        if (isTrack)
            editor.loadTrack(itemId)
        else
            editor.loadBookmark(itemId)
    }

    BookmarkEditor {
        id: editor
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        PullDownMenu {
            MenuItem {
                text: appInfo.localized("delete")
                onClicked: Remorse.popupAction(dialog, appInfo.localized("delete"), function() {
                    bookmarksIO.deleteItems([{ id: dialog.itemId, isTrack: dialog.isTrack }])
                    Navigation.popPage(pageStack, dialog)
                })
            }
        }

        Column {
            id: column
            width: parent.width
            bottomPadding: Theme.paddingLarge

            DialogHeader {
                title: appInfo.localized(isTrack ? "edit_track" : "placepage_edit_bookmark_button")
                acceptText: appInfo.localized("save")
                cancelText: appInfo.localized("cancel")
            }
            TextField {
                id: nameField
                width: parent.width
                label: appInfo.localized(isTrack ? "placepage_track_name_hint" : "placepage_bookmark_name_hint")
                placeholderText: label
                text: editor.name
                EnterKey.iconSource: "image://theme/icon-m-enter-close"
                EnterKey.onClicked: focus = false
            }
            ValueButton {
                label: appInfo.localized("list")
                value: Bookmarks.listName(dialog.listId)
                onClicked: Bookmarks.pickList(pageStack, dialog.listId, function(id) { dialog.listId = id })
            }
            TextSwitch {
                id: visibilitySwitch
                visible: editor.isTrack
                text: appInfo.localized("show_track")
                checked: editor.trackVisible
            }
            SectionHeader {
                text: appInfo.localized("choose_color")
            }
            // A custom color (colorIndex -1) has no swatch here and stays until another one is chosen.
            ColorGrid {
                selectedIndex: dialog.colorIndex
                onColorClicked: dialog.colorIndex = index
            }
            TextArea {
                id: notesArea
                width: parent.width
                label: appInfo.localized("description")
                placeholderText: appInfo.localized("placepage_personal_notes_hint")
                text: editor.description
            }
        }

        VerticalScrollDecorator {}
    }
}
