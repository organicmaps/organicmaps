.import "notices.js" as Toast

// Not a library: it uses the Clipboard global of the page.
function copy(text) {
    Clipboard.text = text
    Toast.show(appInfo.localized("copied_to_clipboard", [text]))
}
