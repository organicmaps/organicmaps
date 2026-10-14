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
        action()
        break
    case OM.AppSettings.DownloadAsk:
        pageStack.push(Qt.resolvedUrl("MessageDialog.qml"), {
            title: appInfo.localized("download_over_mobile_header"),
            message: appInfo.localized("download_over_mobile_message"),
            acceptText: appInfo.localized("download"),
            acceptAction: action
        })
        break
    default:
        Toast.show(appInfo.localized("mobile_data_downloads_off"))
    }
}
