import QtQuick 2.6
import Sailfish.Silica 1.0

// Items are {name, selected}; picked is called with the index, then the page goes back.
Page {
    id: page

    property string title
    property var items: []
    property var picked

    allowedOrientations: Orientation.All

    SilicaListView {
        anchors.fill: parent
        header: PageHeader {
            title: page.title
        }
        model: page.items

        delegate: ListItem {
            highlighted: down || !!modelData.selected
            onClicked: {
                page.picked(index)
                pageStack.pop()
            }

            Label {
                anchors {
                    left: parent.left
                    leftMargin: Theme.horizontalPageMargin
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                text: modelData.name
                truncationMode: TruncationMode.Fade
                highlighted: parent.highlighted
            }
        }

        Component.onCompleted: {
            for (var i = 0; i < page.items.length; ++i) {
                if (page.items[i].selected) {
                    positionViewAtIndex(i, ListView.Center)
                    break
                }
            }
        }

        VerticalScrollDecorator {}
    }
}
