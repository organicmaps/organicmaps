import QtQuick 2.6
import Sailfish.Silica 1.0
import app.organicmaps 1.0

Page {
    id: page

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
                title: qsTrId("settings")
            }

            SectionHeader {
                text: qsTrId("prefs_group_general")
            }
            ComboBox {
                label: qsTrId("pref_appearance_title")
                currentIndex: appSettings.mapAppearance
                menu: ContextMenu {
                    onActivated: appSettings.mapAppearance = index
                    MenuItem { text: qsTrId("follow_system") }
                    MenuItem { text: qsTrId("pref_appearance_light") }
                    MenuItem { text: qsTrId("pref_appearance_dark") }
                }
            }
            ComboBox {
                label: qsTrId("measurement_units")
                description: qsTrId("measurement_units_summary")
                currentIndex: appSettings.units
                menu: ContextMenu {
                    onActivated: appSettings.units = index
                    MenuItem { text: qsTrId("kilometres") }
                    MenuItem { text: qsTrId("miles") }
                }
            }
            TextSwitch {
                text: qsTrId("pref_zoom_title")
                description: qsTrId("pref_zoom_summary")
                automaticCheck: false
                checked: appSettings.zoomButtons
                onClicked: appSettings.zoomButtons = !checked
            }
            // Off under maximum power saving.
            TextSwitch {
                readonly property bool powerSaving: appSettings.powerScheme === AppSettings.PowerEconomyMaximum

                text: qsTrId("pref_map_3d_buildings_title")
                description: powerSaving ? qsTrId("pref_map_3d_buildings_disabled_summary") : ""
                enabled: !powerSaving
                automaticCheck: false
                checked: !powerSaving && appSettings.buildings3d
                onClicked: appSettings.buildings3d = !appSettings.buildings3d
            }
            TextSwitch {
                text: qsTrId("show_downloaded_regions")
                automaticCheck: false
                checked: appSettings.showDownloadedRegions
                onClicked: appSettings.showDownloadedRegions = !checked
            }
            TextSwitch {
                text: qsTrId("big_font")
                automaticCheck: false
                checked: appSettings.largeFonts
                onClicked: appSettings.largeFonts = !checked
            }
            TextSwitch {
                text: qsTrId("transliteration_title")
                automaticCheck: false
                checked: appSettings.transliteration
                onClicked: appSettings.transliteration = !checked
            }
            TextSwitch {
                text: qsTrId("enable_logging")
                description: qsTrId("enable_logging_warning_message")
                             + (checked ? "\n" + qsTrId("log_file_size").arg(appInfo.formatSize(page.logSize))
                                        : "")
                automaticCheck: false
                checked: appSettings.logging
                onClicked: {
                    appSettings.logging = !checked
                    page.logSize = appSettings.logSize()
                }
            }
            ComboBox {
                id: powerSchemeBox
                readonly property var schemes: [AppSettings.PowerNormal, AppSettings.PowerEconomyMaximum,
                                                AppSettings.PowerAuto]

                label: qsTrId("power_managment_title")
                description: qsTrId("power_managment_description")
                currentIndex: Math.max(0, schemes.indexOf(appSettings.powerScheme))
                menu: ContextMenu {
                    onActivated: appSettings.powerScheme = powerSchemeBox.schemes[index]
                    MenuItem { text: qsTrId("power_managment_setting_never") }
                    MenuItem { text: qsTrId("power_managment_setting_manual_max") }
                    MenuItem { text: qsTrId("power_managment_setting_auto") }
                }
            }
            TextSwitch {
                text: qsTrId("enable_keep_screen_on")
                description: qsTrId("enable_keep_screen_on_description")
                automaticCheck: false
                checked: appSettings.keepScreenOn
                onClicked: appSettings.keepScreenOn = !checked
            }
            ValueButton {
                label: qsTrId("change_map_locale")
                value: appSettings.mapLanguageName
                onClicked: {
                    var languages = appSettings.mapLanguages
                    pageStack.push(Qt.resolvedUrl("ListPickerPage.qml"), {
                        title: qsTrId("change_map_locale"),
                        items: languages.map(function(language) {
                            return { name: language.name, selected: language.code === appSettings.mapLanguage }
                        }),
                        picked: function(index) { appSettings.mapLanguage = languages[index].code }
                    })
                }
            }
            ValueButton {
                label: qsTrId("pref_bg_tiles_title")
                value: appSettings.bgTilesEnabled ? qsTrId("on") : qsTrId("off")
                onClicked: pageStack.push(Qt.resolvedUrl("SatelliteSettingsPage.qml"))
            }

            SectionHeader {
                text: qsTrId("prefs_group_information")
            }
            TextRow {
                text: qsTrId("help")
                onClicked: pageStack.push(Qt.resolvedUrl("HelpPage.qml"))
            }
        }

        VerticalScrollDecorator {}
    }
}
