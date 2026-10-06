import QtQuick 2.6
import Sailfish.Silica 1.0
import "notices.js" as Toast

// Sailfish OS has no speech engine, so this also leads to Speech Note.
Page {
    id: page

    // MapItem.routing.
    property QtObject routing

    allowedOrientations: Orientation.All

    // Back from Speech Note or the store, there may be new voices.
    onStatusChanged: if (status === PageStatus.Activating) routing.refreshVoice()
    Connections {
        target: Qt.application
        onActiveChanged: if (Qt.application.active && page.status === PageStatus.Active) routing.refreshVoice()
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width

            PageHeader {
                title: appInfo.localized("pref_tts_enable_title")
            }

            TextSwitch {
                text: appInfo.localized("pref_tts_enable_title")
                description: routing.voiceAvailable ? "" : appInfo.localized("pref_tts_unavailable")
                enabled: routing.voiceAvailable
                automaticCheck: false
                checked: routing.voiceEnabled
                onClicked: routing.voiceEnabled = !routing.voiceEnabled
            }
            ComboBox {
                visible: routing.voiceLanguages.length > 0
                enabled: routing.voiceEnabled
                label: appInfo.localized("pref_tts_language_title")
                currentIndex: routing.voiceLanguageIndex
                menu: ContextMenu {
                    Repeater {
                        model: routing.voiceLanguages
                        MenuItem {
                            text: modelData.speechNote ? modelData.name + " · Speech Note" : modelData.name
                            onClicked: routing.voiceLanguage = modelData.code
                        }
                    }
                }
            }
            ComboBox {
                visible: routing.voices.length > 0
                enabled: routing.voiceEnabled
                label: appInfo.localized("pref_tts_voice_title")
                currentIndex: {
                    for (var i = 0; i < routing.voices.length; ++i)
                        if (routing.voices[i].id === routing.voice)
                            return i
                    return 0
                }
                menu: ContextMenu {
                    Repeater {
                        model: routing.voices
                        MenuItem {
                            text: modelData.name
                            onClicked: routing.voice = modelData.id
                        }
                    }
                }
            }
            TextSwitch {
                enabled: routing.voiceEnabled
                text: appInfo.localized("pref_tts_street_names_title")
                description: appInfo.localized("pref_tts_street_names_description")
                automaticCheck: false
                checked: routing.announceStreets
                onClicked: routing.announceStreets = !routing.announceStreets
            }
            // Applied when released, and heard in a test.
            Slider {
                width: parent.width
                enabled: routing.voiceEnabled
                label: appInfo.localized("volume")
                minimumValue: 0
                maximumValue: 100
                stepSize: 5
                value: routing.voiceVolume
                valueText: Math.round(value)
                onDownChanged: {
                    if (down)
                        return
                    routing.voiceVolume = Math.round(value)
                    routing.testVoice()
                }
            }
            // Also shows the test hint as a notice.
            BackgroundItem {
                enabled: routing.voiceEnabled
                height: Math.max(Theme.itemSizeSmall, testLabel.height + 2 * Theme.paddingMedium)
                onClicked: {
                    routing.testVoice()
                    Toast.showLong(appInfo.localized("pref_tts_playing_test_voice"))
                }

                PageLabel {
                    id: testLabel
                    anchors.verticalCenter: parent.verticalCenter
                    text: appInfo.localized("pref_tts_test_voice_title")
                    color: parent.highlighted ? Theme.highlightColor : Theme.primaryColor
                    opacity: parent.enabled ? 1.0 : Theme.opacityLow
                }
            }
            SpeedCamerasComboBox {}

            SectionHeader {
                text: "Speech Note"
            }
            PageLabel {
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeSmall
                text: {
                    if (!routing.speechNoteInstalled) {
                        var install = appInfo.localized("speech_note_install", [routing.wantedVoiceLanguageName])
                        if (routing.voiceAvailable)
                            return install
                        return install + " " + appInfo.localized("speech_note_other_synthesizers")
                    }
                    if (!routing.wantedHasSpeechNoteVoice)
                        return appInfo.localized("speech_note_download_voice", [routing.wantedVoiceLanguageName])
                    return appInfo.localized("speech_note_in_use")
                }
            }
            Item {
                width: 1
                height: Theme.paddingLarge
            }
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: routing.speechNoteInstalled ? appInfo.localized("speech_note_open")
                                                  : appInfo.localized("speech_note_get")
                onClicked: {
                    if (routing.speechNoteInstalled)
                        routing.openSpeechNote()
                    else
                        Qt.openUrlExternally("https://openrepos.net/content/mkiol/speech-note")
                }
            }
        }

        VerticalScrollDecorator {}
    }
}
