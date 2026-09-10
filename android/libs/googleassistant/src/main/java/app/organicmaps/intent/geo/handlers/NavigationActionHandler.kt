package app.organicmaps.intent.geo.handlers

import app.organicmaps.intent.geo.navigation.Avoid
import app.organicmaps.intent.geo.navigation.TravelMode

interface NavigationActionHandler {
    /**
     * Starts turn-by-turn navigation to a destination by coordinates and/or query.
     *
     * If `query` is null or empty, the destination is set to the coordinates.
     * If `query` is not null or empty and coordinates are not provided, the destination is set to the place resolved from the query.
     * If `query` is not null or empty and coordinates are provided, the destination to the place resolved from the query with search biased to the coordinates.
     *
     * @param lat        destination latitude.
     * @param lon        destination longitude.
     * @param query      destination query.
     * @param avoid      route restrictions.
     * @param travelMode travel mode.
     */
    fun navigate(lat: Double, lon: Double, query: String?, avoid: List<Avoid>?, travelMode: TravelMode?)

    /**
     * Adds a stop to the current route by coordinates and/or query.
     *
     * If `query` is null or empty, the stop is set to the coordinates.
     * If `query` is not null or empty and coordinates are not provided, the stop is set to the place resolved from the query.
     * If `query` is not null or empty and coordinates are provided, the stop is set to the place resolved from the query with search biased to the coordinates.
     *
     * @param lat   stop latitude.
     * @param lon   stop longitude.
     * @param query stop query.
     */
    fun addStop(lat: Double, lon: Double, query: String?)

    /**
     * Shows directions to a destination by coordinates and/or query without starting navigation.
     *
     * If `query` is null or empty, the destination is set to the coordinates.
     * If `query` is not null or empty and coordinates are not provided, the destination is set to the place resolved from the query.
     * If `query` is not null or empty and coordinates are provided, the destination to the place resolved from the query with search biased to the coordinates.
     *
     * @param lat        destination latitude.
     * @param lon        destination longitude.
     * @param query      destination query.
     * @param avoid      route restrictions.
     * @param travelMode travel mode.
     */
    fun showDirections(lat: Double, lon: Double, query: String?, avoid: List<Avoid>?, travelMode: TravelMode?)
}
