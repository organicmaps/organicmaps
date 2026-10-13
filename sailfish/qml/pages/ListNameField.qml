import QtQuick 2.6
import Sailfish.Silica 1.0

// The name of a bookmark list, which must be unique.
ValidatedTextField {
    // The name the list has, which it can keep.
    property string currentName
    readonly property bool taken: text.trim() !== currentName && bookmarksIO.isListNameTaken(text)
    readonly property bool valid: text.trim() !== "" && !taken

    title: appInfo.localized("bookmark_set_name")
    // CategoryValidator.MAX_NAME_LENGTH on Android.
    maximumLength: 60
    error: taken ? appInfo.localized("bookmarks_error_title_list_name_already_taken") : ""
}
