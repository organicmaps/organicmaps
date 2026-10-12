// Not a library: it reads the appInfo and appSettings context properties.

// MessageDialog properties for the one-time notice before the first edit or note: they are public on
// OpenStreetMap. Accepting runs action.
function publicEditNotice(action) {
    return {
        title: appInfo.localized("editor_share_to_all_dialog_title"),
        message: appInfo.localized("editor_share_to_all_dialog_message_1") + " "
                 + appInfo.localized("editor_share_to_all_dialog_message_2"),
        acceptAction: function() {
            appSettings.editsPublicNoticeShown = true
            action()
        }
    }
}
