import QtQuick 2.6
import Sailfish.Silica 1.0
import app.organicmaps 1.0

// Nothing changes until accepted.
Dialog {
    // MapPage holds links back while the user types here.
    property bool keepsUserInput: true
    property alias listId: bookmarks.listId
    // Colors chosen for all bookmarks and all tracks, -1 for unchanged.
    property int bookmarksColor: -1
    property int tracksColor: -1

    allowedOrientations: Orientation.All
    canAccept: nameField.valid
    onAccepted: {
        bookmarks.setListInfo(nameField.text, descriptionArea.text)
        if (bookmarksColor >= 0)
            bookmarks.setAllColor(false, bookmarksColor)
        if (tracksColor >= 0)
            bookmarks.setAllColor(true, tracksColor)
    }

    BookmarksModel {
        id: bookmarks
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: parent.width

            DialogHeader {
                title: appInfo.localized("edit")
            }
            ListNameField {
                id: nameField
                currentName: bookmarks.name
                text: bookmarks.name
                EnterKey.iconSource: "image://theme/icon-m-enter-next"
                EnterKey.onClicked: descriptionArea.focus = true
            }
            TextArea {
                id: descriptionArea
                width: parent.width
                label: appInfo.localized("description")
                placeholderText: label
                text: bookmarks.description
            }
            Repeater {
                model: [{ tracks: false, text: appInfo.localized("change_all_bookmarks_color") },
                        { tracks: true, text: appInfo.localized("change_all_tracks_color") }]

                BackgroundItem {
                    readonly property int chosenColor: modelData.tracks ? tracksColor : bookmarksColor

                    width: column.width
                    // Only for what the list has.
                    visible: modelData.tracks ? bookmarks.tracksCount > 0 : bookmarks.bookmarksCount > 0
                    onClicked: {
                        var tracks = modelData.tracks
                        pageStack.push(Qt.resolvedUrl("ColorPickerPage.qml"), {
                            title: modelData.text,
                            current: chosenColor >= 0 ? appInfo.bookmarkColors[chosenColor] : "",
                            chosen: function(colorIndex) {
                                if (tracks)
                                    tracksColor = colorIndex
                                else
                                    bookmarksColor = colorIndex
                            }
                        })
                    }

                    Label {
                        anchors {
                            left: parent.left
                            leftMargin: Theme.horizontalPageMargin
                            right: colorDot.left
                            rightMargin: Theme.paddingMedium
                            verticalCenter: parent.verticalCenter
                        }
                        text: modelData.text
                        truncationMode: TruncationMode.Fade
                        color: parent.highlighted ? Theme.highlightColor : Theme.primaryColor
                    }
                    // The color to apply on accept.
                    ColorDot {
                        id: colorDot
                        anchors {
                            right: parent.right
                            rightMargin: Theme.horizontalPageMargin
                            verticalCenter: parent.verticalCenter
                        }
                        visible: chosenColor >= 0
                        color: visible ? appInfo.bookmarkColors[chosenColor] : "transparent"
                    }
                }
            }
        }

        VerticalScrollDecorator {}
    }
}
