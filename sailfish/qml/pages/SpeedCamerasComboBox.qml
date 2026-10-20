import QtQuick 2.6
import Sailfish.Silica 1.0

// Speed camera alerts, a voice instructions setting.
ComboBox {
    label: appInfo.localized("speedcams_alert_title")
    currentIndex: appSettings.speedCamerasMode
    menu: ContextMenu {
        onActivated: appSettings.speedCamerasMode = index
        MenuItem { text: appInfo.localized("pref_tts_speedcams_auto") }
        MenuItem { text: appInfo.localized("pref_tts_speedcams_always") }
        MenuItem { text: appInfo.localized("pref_tts_speedcams_never") }
    }
}
