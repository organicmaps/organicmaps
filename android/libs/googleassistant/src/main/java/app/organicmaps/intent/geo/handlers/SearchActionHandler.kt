package app.organicmaps.intent.geo.handlers

interface SearchActionHandler {
    /**
     * Runs a map search by query with location bias.
     *
     * If lat/lon are provided (not NaN), the search results will be biased towards that location.
     *
     * @param lat   bias latitude.
     * @param lon   bias longitude.
     * @param query search query.
     */
    fun search(lat: Double, lon: Double, query: String)

    /**
     * Closes search results if they are currently shown.
     */
    fun clearSearchResults()

    /**
     * Selects a result from the current search results list.
     *
     * @param index zero-based result index.
     */
    fun selectSearchResult(index: Int)

    /**
     * Enables or disables electric vehicle connector filtering for search results.
     *
     * @param enable `true` to enable the filter, `false` to remove it.
     */
    fun enableElectricVehicleConnectorFilter(enable: Boolean)

    /**
     * Enables or disables electric vehicle payment filtering for search results.
     *
     * @param enable `true` to enable the filter, `false` to remove it.
     */
    fun enableElectricVehiclePaymentFilter(enable: Boolean)

    /**
     * Enables or disables electric vehicle fast charging filtering for search results.
     *
     * @param enable `true` to enable the filter, `false` to remove it.
     */
    fun enableElectricVehicleFastChargingFilter(enable: Boolean)
}
