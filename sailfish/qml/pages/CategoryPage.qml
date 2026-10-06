import QtQuick 2.6
import Sailfish.Silica 1.0
import app.organicmaps 1.0
import "editor.js" as Editor
import "navigation.js" as Navigation
import "notices.js" as Toast

Page {
    id: page

    property real lat
    property real lon

    function load(query) {
        var items = categories.categories(query)
        if (items.length === 0 || !items[0].recent)
            return items
        var list = [{ header: appInfo.localized("editor_add_select_category_recent_subtitle") }]
        var i = 0
        for (; i < items.length && items[i].recent; ++i)
            list.push(items[i])
        list.push({ header: appInfo.localized("editor_add_select_category_all_subtitle") })
        return list.concat(items.slice(i))
    }

    allowedOrientations: Orientation.All

    PlaceCategories {
        id: categories
    }

    // Kept outside the list: a new model would take the focus from a field in its header.
    Column {
        id: header
        width: parent.width

        PageHeader {
            title: appInfo.localized("editor_add_select_category")
        }
        SearchField {
            width: parent.width
            placeholderText: appInfo.localized("search")
            onTextChanged: list.model = page.load(text)
            EnterKey.iconSource: "image://theme/icon-m-enter-close"
            EnterKey.onClicked: focus = false
        }
    }

    SilicaListView {
        id: list
        anchors {
            top: header.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        clip: true
        model: page.load("")
        currentIndex: -1

        delegate: TextRow {
            readonly property bool isHeader: modelData.header !== undefined

            enabled: !isHeader
            height: isHeader ? sectionHeader.height : Theme.itemSizeSmall
            text: isHeader ? "" : modelData.name
            onClicked: pageStack.replace(Qt.resolvedUrl("EditPlacePage.qml"),
                                         { newPlaceType: modelData.type, lat: page.lat, lon: page.lon })

            SectionHeader {
                id: sectionHeader
                visible: isHeader
                text: isHeader ? modelData.header : ""
            }
        }

        footer: Column {
            width: list.width
            spacing: Theme.paddingMedium
            topPadding: Theme.paddingLarge
            bottomPadding: Theme.paddingLarge

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                visible: list.count === 0
                text: appInfo.localized("search_not_found")
                color: Theme.highlightColor
            }
            PageLabel {
                text: appInfo.localized("editor_category_unsuitable_text")
                textFormat: Text.StyledText
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryHighlightColor
                linkColor: Theme.highlightColor
                onLinkActivated: Qt.openUrlExternally(link)
            }
            PageLabel {
                text: appInfo.localized("osm_note_hint")
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryHighlightColor
            }
            TextArea {
                id: noteArea
                width: parent.width
                placeholderText: appInfo.localized("editor_note_hint")
            }
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                enabled: noteArea.text.trim() !== ""
                text: appInfo.localized("editor_report_problem_send_button")
                onClicked: {
                    var send = function() {
                        categories.createStandaloneNote(page.lat, page.lon, noteArea.text)
                        osmAccount.uploadChanges()
                        Toast.show(appInfo.localized("osm_note_toast"))
                        // Not pop(): after the notice dialog it would close that instead.
                        Navigation.popPage(pageStack, page)
                    }
                    if (appSettings.editsPublicNoticeShown)
                        send()
                    else
                        pageStack.push(Qt.resolvedUrl("MessageDialog.qml"), Editor.publicEditNotice(send))
                }
            }
        }

        VerticalScrollDecorator {}
    }
}
