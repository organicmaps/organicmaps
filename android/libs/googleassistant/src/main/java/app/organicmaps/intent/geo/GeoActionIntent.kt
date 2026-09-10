package app.organicmaps.intent.geo

import android.content.Intent
import app.organicmaps.intent.geo.action.AccidentType
import app.organicmaps.intent.geo.action.ActionType
import app.organicmaps.intent.geo.action.HazardType
import app.organicmaps.intent.geo.action.LocationOnRoad
import app.organicmaps.intent.geo.action.RoadClosureType
import app.organicmaps.intent.geo.action.RoadDirection
import app.organicmaps.intent.geo.action.TrafficType
import app.organicmaps.intent.geo.enums.Enums
import app.organicmaps.sdk.util.log.Logger

/**
 * Use a custom intent for custom actions like reporting accidents and ending navigation.
 * The main action type is defined by the act query parameter.
 * You can set additional parameters depending on the action type.
 *
 * @see <a href="https://developer.android.com/develop/devices/assistant/intents-assistant-nav-app.custom-action-intent-format">Intent format</a>
 *
 * @see <a href="https://developer.android.com/training/cars/platforms/automotive-os/android-intents-automotive.action-intents">Android automotive intent parameters</a>
 */
class GeoActionIntent private constructor(
    val actionType: ActionType,
    /** Supported action types:
     *
     * @see ActionType.ReportCrash
     */
    val accidentType: AccidentType?,
    /** Supported action types:
     *
     * @see ActionType.ReportHazard
     */
    val hazardType: HazardType?,
    /** Supported action types:
     *
     * @see ActionType.ReportHazard
     */
    val locationOnRoad: LocationOnRoad?,
    /** Supported action types:
     *
     * @see ActionType.ReportRoadClosure
     */
    val roadClosureType: RoadClosureType?,
    /** Supported action types:
     *
     * @see ActionType.ReportTraffic
     */
    val trafficType: TrafficType?,
    /** Supported action types:
     *
     * @see ActionType.ReportCrash
     *
     * @see ActionType.ReportHazard
     *
     * @see ActionType.ReportPolice
     *
     * @see ActionType.ReportTraffic
     */
    val roadDirection: RoadDirection?,
    val searchId: Int,
) {
    companion object {
        private val TAG: String = GeoActionIntent::class.java.simpleName
        private val SUPPORTED_SCHEMES = setOf("geo.action", "geo.action.offline")
        private const val SEARCH_ID_KEY = "id"

        const val DEFAULT_SEARCH_ID: Int = -1

        fun fromIntent(intent: Intent): GeoActionIntent? {
            if (Intent.ACTION_VIEW != intent.action) return null

            if (!SUPPORTED_SCHEMES.contains(intent.scheme)) return null

            val queryParams = getQueryParameters(intent)

            val actionType = Enums.fromRaw<ActionType>(queryParams[ActionType.KEY])
            if (actionType == null) {
                Logger.w(TAG, "Unknown action type: ${queryParams[ActionType.KEY]}")
                return null
            }

            val searchIdRaw = queryParams[SEARCH_ID_KEY]
            val searchId = searchIdRaw?.toIntOrNull()?.takeIf { it >= 0 } ?: run {
                if (searchIdRaw != null && actionType == ActionType.SelectSearchResult) {
                    Logger.w(TAG, "Invalid search id: $searchIdRaw")
                }
                DEFAULT_SEARCH_ID
            }

            return GeoActionIntent(
                actionType,
                Enums.fromRaw<AccidentType>(queryParams[AccidentType.KEY]),
                Enums.fromRaw<HazardType>(queryParams[HazardType.KEY]),
                Enums.fromRaw<LocationOnRoad>(queryParams[LocationOnRoad.KEY]),
                Enums.fromRaw<RoadClosureType>(queryParams[RoadClosureType.KEY]),
                Enums.fromRaw<TrafficType>(queryParams[TrafficType.KEY]),
                Enums.fromRaw<RoadDirection>(queryParams[RoadDirection.KEY]),
                searchId,
            )
        }
    }
}
