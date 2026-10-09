package app.organicmaps.wear.protocol.gms

import app.organicmaps.wear.protocol.WearDistance
import app.organicmaps.wear.protocol.WearDistanceUnit
import app.organicmaps.wear.protocol.WearNavigationData
import app.organicmaps.wear.protocol.WearNavigationDetails
import com.google.android.gms.wearable.DataMap

/** Encodes and decodes ephemeral Wear navigation details using the DataMap format. */
object WearNavigationDetailsDataMapCodec {
    fun encode(dataMap: DataMap, details: WearNavigationDetails) {
        dataMap.putInt(WearNavigationData.KEY_VERSION, WearNavigationData.VERSION)

        details.distanceToTurn
            ?.takeIf { it.value.isNotEmpty() }
            ?.let { distance ->
                dataMap.putString(WearNavigationData.KEY_DISTANCE_TO_TURN_VALUE, distance.value)
                dataMap.putString(WearNavigationData.KEY_DISTANCE_TO_TURN_UNIT, distance.unit.name)
            }

        details.nextStreet
            ?.takeIf { it.isNotEmpty() }
            ?.let { dataMap.putString(WearNavigationData.KEY_NEXT_STREET, it) }

        details.remainingDistance
            ?.takeIf { it.value.isNotEmpty() }
            ?.let { distance ->
                dataMap.putString(WearNavigationData.KEY_REMAINING_DISTANCE_VALUE, distance.value)
                dataMap.putString(WearNavigationData.KEY_REMAINING_DISTANCE_UNIT, distance.unit.name)
            }

        details.remainingTimeSeconds
            ?.takeIf { it >= 0 }
            ?.let { dataMap.putInt(WearNavigationData.KEY_REMAINING_TIME_SECONDS, it) }
    }

    fun decode(dataMap: DataMap): WearNavigationDetails? {
        if (dataMap.getInt(WearNavigationData.KEY_VERSION, -1) != WearNavigationData.VERSION) {
            return null
        }

        return WearNavigationDetails(
            distanceToTurn =
                decodeDistance(
                    dataMap,
                    WearNavigationData.KEY_DISTANCE_TO_TURN_VALUE,
                    WearNavigationData.KEY_DISTANCE_TO_TURN_UNIT,
                ),
            nextStreet =
                dataMap
                    .getString(WearNavigationData.KEY_NEXT_STREET)
                    ?.takeIf { it.isNotEmpty() },
            remainingDistance =
                decodeDistance(
                    dataMap,
                    WearNavigationData.KEY_REMAINING_DISTANCE_VALUE,
                    WearNavigationData.KEY_REMAINING_DISTANCE_UNIT,
                ),
            remainingTimeSeconds =
                dataMap
                    .getInt(WearNavigationData.KEY_REMAINING_TIME_SECONDS, -1)
                    .takeIf { it >= 0 },
        )
    }

    private fun decodeDistance(dataMap: DataMap, valueKey: String, unitKey: String): WearDistance? {
        val value = dataMap.getString(valueKey)?.takeIf { it.isNotEmpty() } ?: return null
        val unitName = dataMap.getString(unitKey) ?: return null
        val unit = WearDistanceUnit.entries.find { it.name == unitName } ?: return null

        return WearDistance(value, unit)
    }
}
