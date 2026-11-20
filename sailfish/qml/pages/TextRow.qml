import QtQuick 2.6
import Sailfish.Silica 1.0

BackgroundItem {
    id: row

    property alias text: label.text
    property bool current
    property string url

    width: parent.width
    height: Theme.itemSizeSmall
    onClicked: if (url !== "") Qt.openUrlExternally(url)

    Label {
        id: label
        anchors {
            left: parent.left
            leftMargin: Theme.horizontalPageMargin
            right: parent.right
            rightMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        color: row.current || row.highlighted ? Theme.highlightColor : Theme.primaryColor
        truncationMode: TruncationMode.Fade
    }
}
