import QtQuick 2.6
import Sailfish.Silica 1.0

// acceptAction runs once the accepted dialog is gone, so that it can open another page.
Dialog {
    id: dialog

    property string title
    property string message
    property string acceptText
    property string cancelText
    property var acceptAction
    property bool _runAction

    allowedOrientations: Orientation.All
    onAccepted: _runAction = !!acceptAction
    onStatusChanged: {
        if (status === PageStatus.Inactive && _runAction) {
            _runAction = false
            acceptAction()
        }
    }

    // Long messages, like the routing disclaimer, scroll in landscape.
    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width

            DialogHeader {
                acceptText: dialog.acceptText !== "" ? dialog.acceptText : defaultAcceptText
                cancelText: dialog.cancelText !== "" ? dialog.cancelText : defaultCancelText
            }
            PageLabel {
                visible: text !== ""
                text: title
                font.pixelSize: Theme.fontSizeLarge
                color: Theme.highlightColor
            }
            PageLabel {
                topPadding: Theme.paddingLarge
                text: message
                color: Theme.secondaryHighlightColor
            }
        }

        VerticalScrollDecorator {}
    }
}
