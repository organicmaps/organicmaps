import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.Share 1.0
import "clipboard.js" as ClipboardHelper

Page {
    id: page

    readonly property string siteUrl: appInfo.localized("translated_om_site_url")
    // Not in the shared strings: Android and iOS have their own addresses.
    readonly property string supportMail: "support@organicmaps.app"

    function icon(name) {
        return Qt.resolvedUrl("../../icons/help/" + name)
    }
    function mail(subject) {
        return "mailto:" + supportMail + "?subject=" + encodeURIComponent(subject)
    }

    allowedOrientations: Orientation.All

    ShareAction {
        id: logShare
        mimeType: "text/plain"
        resources: [appSettings.logUrl]
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width

            // Tapping copies the app and map data versions, e.g. for bug reports.
            PageHeader {
                title: "Organic Maps"
                description: appInfo.version

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        ClipboardHelper.copy(appInfo.version + " • " + Qt.formatDate(appInfo.dataVersion, "yyMMdd"))
                    }
                }
            }

            Icon {
                anchors.horizontalCenter: parent.horizontalCenter
                width: Theme.iconSizeExtraLarge
                height: width
                sourceSize: Qt.size(width, height)
                source: page.icon("logo.svg")
                color: Theme.highlightColor
            }
            PageLabel {
                topPadding: Theme.paddingLarge
                text: appInfo.localized("about_headline")
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeLarge
            }
            PageLabel {
                topPadding: Theme.paddingMedium
                text: [appInfo.localized("about_proposition_1"), appInfo.localized("about_proposition_2"),
                       appInfo.localized("about_proposition_3"), "",
                       appInfo.localized("about_developed_by_enthusiasts")].join("\n")
                color: Theme.secondaryHighlightColor
            }
            PageLabel {
                topPadding: Theme.paddingLarge
                text: "Неофициальная сборка для ОС Аврора, не связана с проектом Organic Maps.\n"
                      + "Unofficial Aurora OS build, not affiliated with the Organic Maps project."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
            }
            TextRow {
                text: "Порт для ОС Аврора: Юрасов Леонид"
                url: "https://gitflic.ru/project/ub3gad/maps"
            }

            MenuRow {
                visible: appSettings.donateUrl !== ""
                icon: page.icon("../menu/ic_donate.svg")
                text: appInfo.localized("donate")
                onClicked: appSettings.openDonatePage()
            }

            SectionHeader {
                text: "OpenStreetMap"
            }
            Row {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                spacing: Theme.paddingLarge

                Image {
                    id: osmLogo
                    width: Theme.iconSizeLarge
                    height: width
                    sourceSize: Qt.size(width, height)
                    source: page.icon("ic_openstreetmap_color.webp")
                }
                Label {
                    width: parent.width - osmLogo.width - parent.spacing
                    anchors.verticalCenter: parent.verticalCenter
                    text: appInfo.localized("osm_presentation",
                                            [Qt.formatDate(appInfo.dataVersion, Locale.ShortFormat)])
                    wrapMode: Text.Wrap
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.secondaryColor
                }
            }
            Item {
                width: 1
                height: Theme.paddingMedium
            }
            PageLabel {
                topPadding: Theme.paddingMedium
                textFormat: Text.RichText
                text: "Map data © <a href=\"https://www.openstreetmap.org/copyright\">OpenStreetMap</a>"
                      + " and <a href=\"https://organicmaps.app\">Organic Maps</a>."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                linkColor: Theme.primaryColor
                onLinkActivated: Qt.openUrlExternally(link)
            }

            Repeater {
                model: [
                    { icon: "ic_question_mark.svg", text: appInfo.localized("faq"), file: "faq.html" },
                    { icon: "ic_report_a_bug.svg", text: appInfo.localized("report_a_bug"), bugReport: true,
                      url: page.mail("[" + appInfo.version + "] Organic Maps Bugreport") },
                    { icon: "../menu/ic_donate.svg", text: appInfo.localized("how_to_support_us"),
                      url: page.siteUrl + "support-us/" },
                    { icon: "ic_news.svg", text: appInfo.localized("news"), url: page.siteUrl + "news/" },
                    { icon: "ic_telegram.svg", text: "Telegram", url: appInfo.localized("telegram_url") },
                    { icon: "ic_github.svg", text: "GitHub", url: "https://github.com/organicmaps/organicmaps" },
                    { icon: "ic_website.svg", text: appInfo.localized("website"), url: page.siteUrl },
                    { icon: "../editor/ic_email.svg", text: appInfo.localized("email"),
                      url: page.mail("Organic Maps") },
                    { icon: "ic_matrix.svg", text: "Matrix", url: "https://matrix.to/#/%23organicmaps:matrix.org" },
                    { icon: "ic_mastodon.svg", text: "Mastodon", url: "https://fosstodon.org/@organicmaps" },
                    { icon: "../editor/ic_facebook.svg", text: "Facebook",
                      url: "https://www.facebook.com/OrganicMaps" },
                    { icon: "../editor/ic_twitterx.svg", text: "X (Twitter)",
                      url: "https://twitter.com/OrganicMapsApp" },
                    { icon: "../editor/ic_instagram.svg", text: "Instagram", url: appInfo.localized("instagram_url") },
                    { icon: "ic_openstreetmap.svg", text: "OpenStreetMap",
                      url: appInfo.localized("osm_wiki_about_url") }
                ]

                MenuRow {
                    icon: page.icon(modelData.icon)
                    text: modelData.text
                    // A bug report is a mail, or with logging on the log to share.
                    onClicked: {
                        if (modelData.file)
                            pageStack.push(Qt.resolvedUrl("HtmlPage.qml"),
                                           { title: modelData.text, file: modelData.file })
                        else if (modelData.bugReport && appSettings.logging)
                            logShare.trigger()
                        else
                            Qt.openUrlExternally(modelData.url)
                    }
                }
            }

            Item {
                width: 1
                height: Theme.paddingLarge
            }
            TextRow {
                text: appInfo.localized("privacy_policy")
                url: page.siteUrl + "privacy/"
            }
            TextRow {
                text: appInfo.localized("terms_of_use")
                url: page.siteUrl + "terms/"
            }
            TextRow {
                text: appInfo.localized("copyright")
                onClicked: pageStack.push(Qt.resolvedUrl("HtmlPage.qml"), { title: text, file: "copyright.html" })
            }
        }

        VerticalScrollDecorator {}
    }
}
