import QtQuick 2.6
import Sailfish.Silica 1.0

Item {
    property alias text: label.text
    property alias textFormat: label.textFormat
    property alias trailingText: trailing.text
    property alias trailingColor: trailing.color

    width: parent.width
    height: Math.max(label.height, trailing.height)
    visible: label.text !== "" || trailing.text !== ""

    Label {
        id: label
        anchors {
            left: parent.left
            right: trailing.left
            rightMargin: trailing.text !== "" ? Theme.paddingMedium : 0
        }
        font.pixelSize: Theme.fontSizeSmall
        color: Theme.secondaryColor
        truncationMode: TruncationMode.Fade
    }
    Label {
        id: trailing
        anchors.right: parent.right
        font.pixelSize: Theme.fontSizeSmall
    }
}
