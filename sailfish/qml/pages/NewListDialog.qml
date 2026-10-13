import QtQuick 2.6
import Sailfish.Silica 1.0

Dialog {
    // MapPage holds links back while the user types here.
    property bool keepsUserInput: true
    // Called with the name, e.g. to move items to the new list.
    property var createAction

    allowedOrientations: Orientation.All
    canAccept: nameField.valid
    onAccepted: createAction(nameField.text)

    Column {
        width: parent.width

        DialogHeader {
            title: appInfo.localized("bookmarks_create_new_group")
            acceptText: appInfo.localized("create")
        }
        ListNameField {
            id: nameField
            focus: true
            EnterKey.enabled: canAccept
            EnterKey.iconSource: "image://theme/icon-m-enter-accept"
            EnterKey.onClicked: accept()
        }
    }
}
