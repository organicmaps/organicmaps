package app.organicmaps.intent.geo.handlers

import app.organicmaps.intent.geo.navigation.Avoid
import app.organicmaps.intent.geo.navigation.TravelMode
import app.organicmaps.sdk.util.log.Logger

class DefaultNavigationActionHandler : NavigationActionHandler {
    override fun navigate(lat: Double, lon: Double, query: String?, avoid: List<Avoid>?, travelMode: TravelMode?) {
        Logger.d(TAG, "Not implemented: navigate to $lat, $lon ($query) avoid=$avoid, travelMode=$travelMode")
    }

    override fun addStop(lat: Double, lon: Double, query: String?) {
        Logger.d(TAG, "Not implemented: add stop at $lat, $lon ($query)")
    }

    override fun showDirections(
        lat: Double,
        lon: Double,
        query: String?,
        avoid: List<Avoid>?,
        travelMode: TravelMode?,
    ) {
        Logger.d(TAG, "Not implemented: show directions to $lat, $lon ($query) avoid=$avoid, travelMode=$travelMode")
    }

    companion object {
        private val TAG: String = DefaultNavigationActionHandler::class.java.simpleName
    }
}
