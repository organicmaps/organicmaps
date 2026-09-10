package app.organicmaps.intent.geo

import android.content.Intent
import app.organicmaps.intent.geo.enums.Enums
import app.organicmaps.intent.geo.navigation.Avoid
import app.organicmaps.intent.geo.navigation.TravelMode
import app.organicmaps.intent.geo.navigation.UserIntent
import app.organicmaps.sdk.util.log.Logger

/**
 * Use a navigation intent to fulfill a user's request to navigate to a specific destination. This destination can
 * either be a single location (address) or multiple locations (for example, coffee shops and gas stations).
 *
 * Intent data follows a URI format specified for each intent.
 *
 * @see <a href="https://developer.android.com/develop/devices/assistant/intents-assistant-nav-app.intent-format">Intent format</a>
 */
class GeoNavigationIntent private constructor(
    val lat: Double,
    val lon: Double,
    val query: String?,
    val intent: UserIntent,
    val avoidList: List<Avoid>,
    val travelMode: TravelMode?,
    /** Used for logging the source of entry.
     *
     * Possible values: assistant. */
    val entry: String?,
) {
    companion object {
        private val TAG: String = GeoNavigationIntent::class.java.simpleName
        private val SUPPORTED_ACTIONS =
            setOf("androidx.car.app.action.NAVIGATE", "android.intent.action.NAVIGATE")
        private val SUPPORTED_SCHEMES = setOf("geo", "geo.offline")
        private const val QUERY_KEY = "q"
        private const val ENTRY_KEY = "entry"

        fun fromIntent(intent: Intent): GeoNavigationIntent? {
            if (!SUPPORTED_ACTIONS.contains(intent.action)) return null

            if (!SUPPORTED_SCHEMES.contains(intent.scheme)) return null

            val latLon = getCoordinates(intent)
            val queryParameters = getQueryParameters(intent)
            val query = queryParameters[QUERY_KEY]
            val userIntent = parseUserIntent(queryParameters[UserIntent.KEY])
            val avoidList = parseAvoidList(queryParameters[Avoid.KEY])
            val travelMode = parseTravelMode(queryParameters[TravelMode.KEY])
            val entry = queryParameters[ENTRY_KEY]
            return GeoNavigationIntent(latLon[0], latLon[1], query, userIntent, avoidList, travelMode, entry)
        }

        private fun parseUserIntent(userIntentStr: String?): UserIntent {
            if (userIntentStr.isNullOrEmpty()) return UserIntent.Navigation

            return Enums.fromRaw<UserIntent>(userIntentStr) ?: run {
                Logger.w(TAG, "Unknown user intent: $userIntentStr")
                UserIntent.Navigation
            }
        }

        private fun parseAvoidList(avoidListStr: String?): List<Avoid> {
            if (avoidListStr.isNullOrEmpty()) return emptyList()

            val avoidList = mutableListOf<Avoid>()
            for (item in avoidListStr.split(',')) {
                if (item.length != 1) {
                    Logger.w(TAG, "Invalid avoid item: $item")
                    continue
                }

                val avoid = Enums.fromRaw<Avoid>(item[0])
                if (avoid == null) {
                    Logger.w(TAG, "Unknown avoid item: $item")
                    continue
                }

                avoidList.add(avoid)
            }
            return avoidList
        }

        private fun parseTravelMode(travelModeStr: String?): TravelMode? {
            if (travelModeStr.isNullOrEmpty()) return null

            val rawMode = travelModeStr.singleOrNull()
            if (rawMode == null) {
                Logger.w(TAG, "Invalid travel mode: $travelModeStr")
                return null
            }

            return Enums.fromRaw<TravelMode>(rawMode) ?: run {
                Logger.w(TAG, "Unknown travel mode: $travelModeStr")
                null
            }
        }
    }
}
