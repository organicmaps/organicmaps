import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: page

    property var streets: []
    property string current

    signal selected(string street)

    function select(street) {
        page.selected(street)
        pageStack.pop()
    }

    allowedOrientations: Orientation.All

    SilicaListView {
        anchors.fill: parent
        model: page.streets

        header: Column {
            width: parent.width

            PageHeader {
                title: appInfo.localized("choose_street")
            }
            TextField {
                width: parent.width
                label: appInfo.localized("add_street")
                placeholderText: label
                EnterKey.enabled: text.trim() !== ""
                EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                EnterKey.onClicked: page.select(text.trim())
            }
        }

        delegate: TextRow {
            text: modelData
            current: modelData === page.current
            onClicked: page.select(modelData)
        }

        VerticalScrollDecorator {}
    }
}
