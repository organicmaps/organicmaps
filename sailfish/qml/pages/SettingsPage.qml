import QtQuick 2.6
import Sailfish.Silica 1.0
import app.organicmaps 1.0

Page {
    id: page

    // MapItem.routing.
    property QtObject routing
    // The log grows without a change signal, so it's read when the page shows.
    property real logSize

    allowedOrientations: Orientation.All
    onStatusChanged: if (status === PageStatus.Activating) logSize = appSettings.logSize()

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width

            PageHeader {
                title: appInfo.localized("settings")
            }
            ValueButton {
                label: appInfo.localized("profile")
                value: osmAccount.loggedIn ? osmAccount.userName : appInfo.localized("not_signed_in")
                onClicked: pageStack.push(Qt.resolvedUrl("OsmAccountPage.qml"))
            }

            SectionHeader {
                text: appInfo.localized("prefs_group_general")
            }
            ComboBox {
                label: appInfo.localized("pref_appearance_title")
                currentIndex: appSettings.mapAppearance
                menu: ContextMenu {
                    onActivated: appSettings.mapAppearance = index
                    MenuItem { text: appInfo.localized("follow_system") }
                    MenuItem { text: appInfo.localized("pref_appearance_light") }
                    MenuItem { text: appInfo.localized("pref_appearance_dark") }
                    MenuItem { text: appInfo.localized("pref_appearance_scheduled") }
                }
            }
            ComboBox {
                label: appInfo.localized("measurement_units")
                description: appInfo.localized("measurement_units_summary")
                currentIndex: appSettings.units
                menu: ContextMenu {
                    onActivated: appSettings.units = index
                    MenuItem { text: appInfo.localized("kilometres") }
                    MenuItem { text: appInfo.localized("miles") }
                }
            }
            TextSwitch {
                text: appInfo.localized("pref_zoom_title")
                description: appInfo.localized("pref_zoom_summary")
                automaticCheck: false
                checked: appSettings.zoomButtons
                onClicked: appSettings.zoomButtons = !checked
            }
            // Off under maximum power saving.
            TextSwitch {
                readonly property bool powerSaving: appSettings.powerScheme === AppSettings.PowerEconomyMaximum

                text: appInfo.localized("pref_map_3d_buildings_title")
                description: powerSaving ? appInfo.localized("pref_map_3d_buildings_disabled_summary") : ""
                enabled: !powerSaving
                automaticCheck: false
                checked: !powerSaving && appSettings.buildings3d
                onClicked: appSettings.buildings3d = !appSettings.buildings3d
            }
            TextSwitch {
                text: appInfo.localized("autodownload")
                automaticCheck: false
                checked: appSettings.autoDownload
                onClicked: appSettings.autoDownload = !checked
            }
            TextSwitch {
                text: appInfo.localized("show_downloaded_regions")
                automaticCheck: false
                checked: appSettings.showDownloadedRegions
                onClicked: appSettings.showDownloadedRegions = !checked
            }
            TextSwitch {
                text: appInfo.localized("big_font")
                automaticCheck: false
                checked: appSettings.largeFonts
                onClicked: appSettings.largeFonts = !checked
            }
            TextSwitch {
                text: appInfo.localized("transliteration_title")
                automaticCheck: false
                checked: appSettings.transliteration
                onClicked: appSettings.transliteration = !checked
            }
            ValueButton {
                label: appInfo.localized("maps_storage")
                value: mapsStorage.currentName
                onClicked: pageStack.push(Qt.resolvedUrl("StoragePage.qml"))
            }
            ValueButton {
                label: appInfo.localized("backup_automatic")
                value: bookmarksIO.backupPeriod === 0 ? appInfo.localized("off")
                     : appInfo.localized(bookmarksIO.backupPeriod === 1 ? "daily" : "backup_weekly")
                onClicked: pageStack.push(Qt.resolvedUrl("BackupPage.qml"))
            }
            TextSwitch {
                text: appInfo.localized("enable_logging")
                description: appInfo.localized("enable_logging_warning_message")
                             + (checked ? "\n" + appInfo.localized("log_file_size", [appInfo.formatSize(page.logSize)])
                                        : "")
                automaticCheck: false
                checked: appSettings.logging
                onClicked: {
                    appSettings.logging = !checked
                    page.logSize = appSettings.logSize()
                }
            }
            ComboBox {
                label: appInfo.localized("mobile_data")
                description: appInfo.localized("mobile_data_description")
                currentIndex: appSettings.mobileData
                menu: ContextMenu {
                    onActivated: appSettings.mobileData = index
                    MenuItem { text: appInfo.localized("mobile_data_option_ask") }
                    MenuItem { text: appInfo.localized("mobile_data_option_always") }
                    MenuItem { text: appInfo.localized("mobile_data_option_never") }
                }
            }
            ComboBox {
                id: powerSchemeBox
                readonly property var schemes: [AppSettings.PowerNormal, AppSettings.PowerEconomyMaximum,
                                                AppSettings.PowerAuto]

                label: appInfo.localized("power_managment_title")
                description: appInfo.localized("power_managment_description")
                currentIndex: Math.max(0, schemes.indexOf(appSettings.powerScheme))
                menu: ContextMenu {
                    onActivated: appSettings.powerScheme = powerSchemeBox.schemes[index]
                    MenuItem { text: appInfo.localized("power_managment_setting_never") }
                    MenuItem { text: appInfo.localized("power_managment_setting_manual_max") }
                    MenuItem { text: appInfo.localized("power_managment_setting_auto") }
                }
            }
            ComboBox {
                label: appInfo.localized("bookmarks_text_placement_title")
                description: appInfo.localized("bookmarks_text_placement_description")
                currentIndex: appSettings.bookmarksTextPlacement
                menu: ContextMenu {
                    onActivated: appSettings.bookmarksTextPlacement = index
                    MenuItem { text: appInfo.localized("hide") }
                    MenuItem { text: appInfo.localized("show_to_the_right") }
                    MenuItem { text: appInfo.localized("show_at_the_bottom") }
                }
            }
            TextSwitch {
                text: appInfo.localized("enable_keep_screen_on")
                description: appInfo.localized("enable_keep_screen_on_description")
                automaticCheck: false
                checked: appSettings.keepScreenOn
                onClicked: appSettings.keepScreenOn = !checked
            }
            ValueButton {
                label: appInfo.localized("change_map_locale")
                value: appSettings.mapLanguageName
                onClicked: {
                    var languages = appSettings.mapLanguages
                    pageStack.push(Qt.resolvedUrl("ListPickerPage.qml"), {
                        title: appInfo.localized("change_map_locale"),
                        items: languages.map(function(language) {
                            return { name: language.name, selected: language.code === appSettings.mapLanguage }
                        }),
                        picked: function(index) { appSettings.mapLanguage = languages[index].code }
                    })
                }
            }
            ValueButton {
                label: appInfo.localized("pref_bg_tiles_title")
                value: appSettings.bgTilesEnabled ? appInfo.localized("on") : appInfo.localized("off")
                onClicked: pageStack.push(Qt.resolvedUrl("SatelliteSettingsPage.qml"))
            }

            SectionHeader {
                text: appInfo.localized("prefs_group_route")
            }
            TextSwitch {
                text: appInfo.localized("pref_auto_night_in_navigation_title")
                automaticCheck: false
                checked: appSettings.autoNightInNavigation
                onClicked: appSettings.autoNightInNavigation = !checked
            }
            TextSwitch {
                text: appInfo.localized("pref_map_3d_title")
                automaticCheck: false
                checked: appSettings.perspectiveView
                onClicked: appSettings.perspectiveView = !checked
            }
            TextSwitch {
                text: appInfo.localized("pref_map_auto_zoom")
                automaticCheck: false
                checked: appSettings.autoZoom
                onClicked: appSettings.autoZoom = !checked
            }
            ValueButton {
                visible: appInfo.voiceSupported
                label: appInfo.localized("pref_tts_enable_title")
                value: !routing.voiceAvailable ? appInfo.localized("pref_tts_unavailable")
                     : routing.voiceEnabled ? routing.voiceLanguageName
                     : appInfo.localized("off")
                onClicked: pageStack.push(Qt.resolvedUrl("VoicePage.qml"), { routing: routing })
            }
            // Otherwise on the voice page, which Harbour builds lack.
            SpeedCamerasComboBox {
                visible: !appInfo.voiceSupported
            }
            ValueButton {
                label: appInfo.localized("driving_options_title")
                value: appInfo.localized(routing.avoidRoads !== 0 || routing.routeOptimization ? "on" : "off")
                onClicked: pageStack.push(Qt.resolvedUrl("RoutingOptionsPage.qml"), { routing: routing })
            }

            SectionHeader {
                text: appInfo.localized("privacy")
            }
            TextSwitch {
                text: appInfo.localized("search_history_title")
                automaticCheck: false
                checked: appSettings.searchHistory
                onClicked: appSettings.searchHistory = !checked
            }

            SectionHeader {
                text: appInfo.localized("prefs_group_information")
            }
            TextRow {
                text: appInfo.localized("help")
                onClicked: pageStack.push(Qt.resolvedUrl("HelpPage.qml"))
            }
        }

        VerticalScrollDecorator {}
    }
}
