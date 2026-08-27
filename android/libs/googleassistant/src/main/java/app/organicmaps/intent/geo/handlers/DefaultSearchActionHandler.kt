package app.organicmaps.intent.geo.handlers

import app.organicmaps.sdk.util.log.Logger

class DefaultSearchActionHandler : SearchActionHandler {
    override fun search(lat: Double, lon: Double, query: String) {
        Logger.d(TAG, "Not implemented: search for $query at $lat, $lon")
    }

    override fun clearSearchResults() {
        Logger.d(TAG, "Not implemented: clear search results")
    }

    override fun selectSearchResult(index: Int) {
        Logger.d(TAG, "Not implemented: select search result index=$index")
    }

    override fun enableElectricVehicleConnectorFilter(enable: Boolean) {
        Logger.d(TAG, "Not implemented: enable electric vehicle connector filter enable=$enable")
    }

    override fun enableElectricVehiclePaymentFilter(enable: Boolean) {
        Logger.d(TAG, "Not implemented: enable electric vehicle payment filter enable=$enable")
    }

    override fun enableElectricVehicleFastChargingFilter(enable: Boolean) {
        Logger.d(TAG, "Not implemented: enable electric vehicle fast charging filter enable=$enable")
    }

    companion object {
        private val TAG: String = DefaultSearchActionHandler::class.java.simpleName
    }
}
