.import "notices.js" as Toast
.import app.organicmaps 1.0 as OM

function busy(status) {
    return status === OM.CountriesModel.Downloading || status === OM.CountriesModel.InQueue
           || status === OM.CountriesModel.Applying
}

function toggle(pageStack, map, country) {
    var countryId = country.countryId
    if (busy(country.status))
        map.cancelMap(countryId)
    else
        start(pageStack, function() { map.downloadMap(countryId) })
}

// Honors the Mobile Internet setting on cellular connections, asking first when set to.
function start(pageStack, action) {
    switch (appSettings.downloadPermission()) {
    case OM.AppSettings.DownloadAllowed:
        appSettings.confirmDownload()
        action()
        break
    case OM.AppSettings.DownloadAsk:
        pageStack.push(Qt.resolvedUrl("MessageDialog.qml"), {
            title: qsTrId("download_over_mobile_header"),
            message: qsTrId("download_over_mobile_message"),
            acceptText: qsTrId("download"),
            acceptAction: function() {
                appSettings.confirmDownload()
                action()
            }
        })
        break
    default:
        Toast.show(qsTrId("mobile_data_downloads_off"))
    }
}
