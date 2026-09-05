package app.organicmaps.wear.protocol.gms

import app.organicmaps.wear.protocol.WearNavigationData
import app.organicmaps.wear.protocol.WearNavigationMode
import app.organicmaps.wear.protocol.WearNavigationState
import com.google.android.gms.wearable.DataMap
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

class WearNavigationDataMapCodecTest {
    @Test
    fun roundTripNavigation() {
        val dataMap = DataMap()
        WearNavigationDataMapCodec.encode(dataMap, WearNavigationState.navigation())

        assertEquals(WearNavigationMode.NAVIGATION, WearNavigationDataMapCodec.decode(dataMap)?.mode)
    }

    @Test
    fun roundTripNormal() {
        val dataMap = DataMap()
        WearNavigationDataMapCodec.encode(dataMap, WearNavigationState.normal())

        assertEquals(WearNavigationMode.NORMAL, WearNavigationDataMapCodec.decode(dataMap)?.mode)
    }

    @Test
    fun encodeUsesStableSchema() {
        val normalDataMap = DataMap()
        WearNavigationDataMapCodec.encode(normalDataMap, WearNavigationState.normal())

        assertEquals(WearNavigationData.VERSION, normalDataMap.getInt("version", -1))
        assertEquals("NORMAL", normalDataMap.getString("mode"))

        val navigationDataMap = DataMap()
        WearNavigationDataMapCodec.encode(navigationDataMap, WearNavigationState.navigation())

        assertEquals(WearNavigationData.VERSION, navigationDataMap.getInt("version", -1))
        assertEquals("NAVIGATION", navigationDataMap.getString("mode"))
    }

    @Test
    fun versionMismatchReturnsNull() {
        assertNull(WearNavigationDataMapCodec.decode(dataMap(999, "NAVIGATION")))
    }

    @Test
    fun missingVersionReturnsNull() {
        val dataMap = DataMap()
        dataMap.putString(WearNavigationData.KEY_MODE, "NAVIGATION")

        assertNull(WearNavigationDataMapCodec.decode(dataMap))
    }

    @Test
    fun missingModeReturnsNull() {
        val dataMap = DataMap()
        dataMap.putInt(WearNavigationData.KEY_VERSION, WearNavigationData.VERSION)

        assertNull(WearNavigationDataMapCodec.decode(dataMap))
    }

    @Test
    fun unknownModeReturnsNull() {
        assertNull(WearNavigationDataMapCodec.decode(dataMap(WearNavigationData.VERSION, "BOGUS")))
    }

    @Test
    fun unknownFieldIsIgnored() {
        val dataMap = dataMap(WearNavigationData.VERSION, "NAVIGATION")
        dataMap.putBoolean("unknown", true)

        assertEquals(WearNavigationMode.NAVIGATION, WearNavigationDataMapCodec.decode(dataMap)?.mode)
    }

    private fun dataMap(version: Int, mode: String) = DataMap().apply {
        putInt(WearNavigationData.KEY_VERSION, version)
        putString(WearNavigationData.KEY_MODE, mode)
    }
}
