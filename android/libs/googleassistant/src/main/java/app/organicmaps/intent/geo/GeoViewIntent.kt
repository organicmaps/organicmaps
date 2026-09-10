package app.organicmaps.intent.geo

import android.content.Intent

/**
 * Use a search intent to search for a query and display multiple results along the route while driving.
 *
 * @see <a href="https://developer.android.com/develop/devices/assistant/intents-assistant-nav-app#search-intent">Intent format</a>
 */
class GeoViewIntent private constructor(val lat: Double, val lon: Double, val query: String) {
    companion object {
        private val SUPPORTED_SCHEMES = setOf("geo", "geo.offline")
        private const val QUERY_KEY = "q"

        fun fromIntent(intent: Intent): GeoViewIntent? {
            if (Intent.ACTION_VIEW != intent.action) return null

            if (!SUPPORTED_SCHEMES.contains(intent.scheme)) return null

            val latLon = getCoordinates(intent)
            val queryParameters = getQueryParameters(intent)

            val query = queryParameters[QUERY_KEY]
            if (query.isNullOrEmpty()) return null

            return GeoViewIntent(latLon[0], latLon[1], query)
        }
    }
}
