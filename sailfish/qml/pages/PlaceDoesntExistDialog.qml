import QtQuick 2.6
import Sailfish.Silica 1.0

Dialog {
    property QtObject placeEditor
    // Where to return to, past the editor.
    property Item returnPage

    allowedOrientations: Orientation.All
    canAccept: commentField.text.trim() !== ""
    acceptDestination: returnPage
    acceptDestinationAction: PageStackAction.Pop
    onAccepted: {
        placeEditor.placeDoesntExist(commentField.text)
        osmAccount.uploadChanges()
    }

    Column {
        width: parent.width

        DialogHeader {
            title: appInfo.localized("editor_place_doesnt_exist")
            acceptText: appInfo.localized("editor_report_problem_send_button")
        }
        TextArea {
            id: commentField
            width: parent.width
            focus: true
            label: text.trim() === "" ? appInfo.localized("delete_place_empty_comment_error")
                                      : appInfo.localized("editor_comment_hint")
            placeholderText: appInfo.localized("editor_comment_hint")
        }
    }
}
