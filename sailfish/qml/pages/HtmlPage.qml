import QtQuick 2.6
import Sailfish.Silica 1.0

// A HTML file of the data folder, like faq.html, in the app's language.
Page {
    id: page

    property string title
    property string file

    function openLink(link) {
        if (link.charAt(0) !== "#") {
            Qt.openUrlExternally(link)
            return
        }
        for (var i = 0; i < listView.count; ++i) {
            if (listView.model[i].id === link.substring(1)) {
                listView.positionViewAtIndex(i, ListView.Beginning)
                return
            }
        }
    }

    allowedOrientations: Orientation.All

    SilicaListView {
        id: listView
        anchors.fill: parent
        header: PageHeader {
            title: page.title
        }
        model: appInfo.bundledHtml(page.file)

        delegate: PageLabel {
            // linkColor only works for StyledText.
            text: "<style>a { color: " + Theme.highlightColor + "; }</style>" + modelData.html
            textFormat: Text.RichText
            font.pixelSize: Theme.fontSizeSmall
            onLinkActivated: page.openLink(link)
        }

        VerticalScrollDecorator {}
    }
}
