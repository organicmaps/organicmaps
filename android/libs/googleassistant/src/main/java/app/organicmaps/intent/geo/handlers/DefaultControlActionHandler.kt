package app.organicmaps.intent.geo.handlers

import app.organicmaps.sdk.util.log.Logger

class DefaultControlActionHandler : ControlActionHandler {
    override fun exitNavigation() {
        Logger.d(TAG, "Not implemented: exit navigation")
    }

    override fun changeMapViewToFollowMode() {
        Logger.d(TAG, "Not implemented: change map view to follow mode")
    }

    override fun goBack() {
        Logger.d(TAG, "Not implemented: go back")
    }

    override fun processShowSatelliteAction(show: Boolean) {
        Logger.d(TAG, "Not implemented: process show satellite action show=$show")
    }

    override fun processShowTrafficAction(show: Boolean) {
        Logger.d(TAG, "Not implemented: process show traffic action show=$show")
    }

    override fun showRouteOverview() {
        Logger.d(TAG, "Not implemented: show route overview")
    }

    override fun showAlternativeRoutes() {
        Logger.d(TAG, "Not implemented: show alternative routes")
    }

    override fun showDirectionsList() {
        Logger.d(TAG, "Not implemented: show directions list")
    }

    override fun showMyLocation() {
        Logger.d(TAG, "Not implemented: show my location")
    }

    override fun showMap() {
        Logger.d(TAG, "Not implemented: show map")
    }

    companion object {
        private val TAG: String = DefaultControlActionHandler::class.java.simpleName
    }
}
