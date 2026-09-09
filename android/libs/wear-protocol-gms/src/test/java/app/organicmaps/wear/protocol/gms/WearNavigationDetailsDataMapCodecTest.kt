package app.organicmaps.wear.protocol.gms

import app.organicmaps.wear.protocol.WearDistance
import app.organicmaps.wear.protocol.WearDistanceUnit
import app.organicmaps.wear.protocol.WearNavigationData
import app.organicmaps.wear.protocol.WearNavigationDetails
import com.google.android.gms.wearable.DataMap
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

class WearNavigationDetailsDataMapCodecTest {
    @Test
    fun roundTripDetails() {
        val details =
            WearNavigationDetails(
                distanceToTurn = WearDistance("150", WearDistanceUnit.METERS),
                nextStreet = "Rue Victor Hugo",
                remainingDistance = WearDistance("4.2", WearDistanceUnit.KILOMETERS),
                remainingTimeSeconds = 1080,
            )
        val dataMap = DataMap()

        WearNavigationDetailsDataMapCodec.encode(dataMap, details)

        assertEquals(details, WearNavigationDetailsDataMapCodec.decode(dataMap))
    }

    @Test
    fun encodeUsesStableSchema() {
        assertEquals(
            "/organicmaps/navigation/details",
            WearNavigationData.PATH_NAVIGATION_DETAILS,
        )

        val dataMap = DataMap()
        WearNavigationDetailsDataMapCodec.encode(
            dataMap,
            WearNavigationDetails(
                distanceToTurn = WearDistance("150", WearDistanceUnit.METERS),
                nextStreet = "Rue Victor Hugo",
                remainingDistance = WearDistance("4.2", WearDistanceUnit.KILOMETERS),
                remainingTimeSeconds = 1080,
            ),
        )

        assertEquals(WearNavigationData.VERSION, dataMap.getInt("version", -1))
        assertEquals("150", dataMap.getString("distance_to_turn_value"))
        assertEquals("METERS", dataMap.getString("distance_to_turn_unit"))
        assertEquals("Rue Victor Hugo", dataMap.getString("next_street"))
        assertEquals("4.2", dataMap.getString("remaining_distance_value"))
        assertEquals("KILOMETERS", dataMap.getString("remaining_distance_unit"))
        assertEquals(1080, dataMap.getInt("remaining_time_seconds", -1))
    }

    @Test
    fun versionMismatchReturnsNull() {
        val dataMap = DataMap()
        dataMap.putInt(WearNavigationData.KEY_VERSION, 999)

        assertNull(WearNavigationDetailsDataMapCodec.decode(dataMap))
    }

    @Test
    fun missingVersionReturnsNull() {
        assertNull(WearNavigationDetailsDataMapCodec.decode(DataMap()))
    }

    @Test
    fun emptyStringsAreOmitted() {
        val dataMap = DataMap()

        WearNavigationDetailsDataMapCodec.encode(
            dataMap,
            WearNavigationDetails(
                distanceToTurn = WearDistance("", WearDistanceUnit.METERS),
                nextStreet = "",
                remainingDistance = WearDistance("", WearDistanceUnit.KILOMETERS),
            ),
        )

        assertNull(dataMap.getString(WearNavigationData.KEY_DISTANCE_TO_TURN_VALUE))
        assertNull(dataMap.getString(WearNavigationData.KEY_DISTANCE_TO_TURN_UNIT))
        assertNull(dataMap.getString(WearNavigationData.KEY_NEXT_STREET))
        assertNull(dataMap.getString(WearNavigationData.KEY_REMAINING_DISTANCE_VALUE))
        assertNull(dataMap.getString(WearNavigationData.KEY_REMAINING_DISTANCE_UNIT))
    }

    @Test
    fun emptyStringsDecodeAsAbsent() {
        val dataMap = versionedDataMap()
        dataMap.putString(WearNavigationData.KEY_DISTANCE_TO_TURN_VALUE, "")
        dataMap.putString(WearNavigationData.KEY_DISTANCE_TO_TURN_UNIT, "METERS")
        dataMap.putString(WearNavigationData.KEY_NEXT_STREET, "")
        dataMap.putString(WearNavigationData.KEY_REMAINING_DISTANCE_VALUE, "")
        dataMap.putString(WearNavigationData.KEY_REMAINING_DISTANCE_UNIT, "KILOMETERS")

        val decoded = WearNavigationDetailsDataMapCodec.decode(dataMap)

        assertNull(decoded?.distanceToTurn)
        assertNull(decoded?.nextStreet)
        assertNull(decoded?.remainingDistance)
    }

    @Test
    fun incompleteDistanceDecodesAsAbsent() {
        val dataMap = versionedDataMap()
        dataMap.putString(WearNavigationData.KEY_DISTANCE_TO_TURN_VALUE, "150")
        dataMap.putString(WearNavigationData.KEY_REMAINING_DISTANCE_UNIT, "KILOMETERS")

        val decoded = WearNavigationDetailsDataMapCodec.decode(dataMap)

        assertNull(decoded?.distanceToTurn)
        assertNull(decoded?.remainingDistance)
    }

    @Test
    fun unknownDistanceUnitDecodesAsAbsent() {
        val dataMap = versionedDataMap()
        dataMap.putString(WearNavigationData.KEY_DISTANCE_TO_TURN_VALUE, "150")
        dataMap.putString(WearNavigationData.KEY_DISTANCE_TO_TURN_UNIT, "YARDS")

        assertNull(WearNavigationDetailsDataMapCodec.decode(dataMap)?.distanceToTurn)
    }

    @Test
    fun negativeRemainingTimeIsOmitted() {
        val dataMap = DataMap()

        WearNavigationDetailsDataMapCodec.encode(
            dataMap,
            WearNavigationDetails(remainingTimeSeconds = -1),
        )

        assertEquals(
            -1,
            dataMap.getInt(WearNavigationData.KEY_REMAINING_TIME_SECONDS, -1),
        )
    }

    @Test
    fun negativeRemainingTimeDecodesAsAbsent() {
        val dataMap = versionedDataMap()
        dataMap.putInt(WearNavigationData.KEY_REMAINING_TIME_SECONDS, -1)

        assertNull(WearNavigationDetailsDataMapCodec.decode(dataMap)?.remainingTimeSeconds)
    }

    @Test
    fun zeroRemainingTimeIsPreserved() {
        val dataMap = DataMap()

        WearNavigationDetailsDataMapCodec.encode(
            dataMap,
            WearNavigationDetails(remainingTimeSeconds = 0),
        )

        assertEquals(0, WearNavigationDetailsDataMapCodec.decode(dataMap)?.remainingTimeSeconds)
    }

    @Test
    fun unknownFieldIsIgnored() {
        val dataMap = versionedDataMap()
        dataMap.putBoolean("unknown", true)

        assertEquals(
            WearNavigationDetails(),
            WearNavigationDetailsDataMapCodec.decode(dataMap),
        )
    }

    private fun versionedDataMap() = DataMap().apply {
        putInt(WearNavigationData.KEY_VERSION, WearNavigationData.VERSION)
    }
}
