import QtQuick 2.6
import Sailfish.Silica 1.0
import "downloads.js" as Downloads

Column {
    // A MissingMapInfo().
    property var country
    readonly property bool busy: Downloads.busy(country.status)

    signal downloadClicked(string countryId)

    spacing: Theme.paddingLarge

    Button {
        anchors.horizontalCenter: parent.horizontalCenter
        visible: !!country.countryId
        enabled: !parent.busy
        text: parent.busy
              ? qsTrId("downloader_downloading") + " " + Math.round((country.progress || 0) * 100) + "%"
              : qsTrId("downloader_download_map") + " (" + country.size + ")"
        onClicked: parent.downloadClicked(country.countryId)
    }
    Button {
        anchors.horizontalCenter: parent.horizontalCenter
        text: qsTrId("search_select_map")
        onClicked: pageStack.push(Qt.resolvedUrl("MapsPage.qml"), { downloadedOnly: false })
    }
}
