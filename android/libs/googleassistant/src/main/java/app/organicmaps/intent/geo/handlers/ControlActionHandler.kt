package app.organicmaps.intent.geo.handlers

interface ControlActionHandler {
    /**
     * Cancels active navigation guidance.
     */
    fun exitNavigation()

    /**
     * Switches the map to follow mode.
     */
    fun changeMapViewToFollowMode()

    /**
     * Returns to the previous screen.
     */
    fun goBack()

    /**
     * Enables or disables satellite map style.
     *
     * @param show `true` to enable satellite imagery, `false` to hide it.
     */
    fun processShowSatelliteAction(show: Boolean)

    /**
     * Enables or disables traffic overlay.
     *
     * @param show `true` to show traffic, `false` to hide traffic.
     */
    fun processShowTrafficAction(show: Boolean)

    /**
     * Opens route overview for the active route.
     */
    fun showRouteOverview()

    /**
     * Opens available alternative routes for the active route.
     */
    fun showAlternativeRoutes()

    /**
     * Opens the turn-by-turn directions list for the active route.
     */
    fun showDirectionsList()

    /**
     * Recenters the map on the current user location.
     */
    fun showMyLocation()

    /**
     * Closes open cards and returns to the map.
     */
    fun showMap()
}
