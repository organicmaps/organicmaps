package app.organicmaps.intent.geo

import android.content.Intent
import app.organicmaps.intent.geo.navigation.Avoid
import app.organicmaps.intent.geo.navigation.TravelMode
import app.organicmaps.intent.geo.navigation.UserIntent
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner
import org.robolectric.annotation.Config

@RunWith(RobolectricTestRunner::class)
@Config(shadows = [ApiControllerMock::class])
class GeoNavigationIntentTest {
    @Before
    fun setUp() {
        ApiControllerMock.reset()
    }

    @Test
    fun fromIntent_parsesDocumentedSample() {
        ApiControllerMock.mockLatLon(
            TestUriSamples.NAVIGATION_WITH_COORDINATES,
            listOf(1.1, 2.2).toDoubleArray(),
        )
        val parsed = GeoNavigationIntent.fromIntent(
            Intent("android.intent.action.NAVIGATE").apply {
                data = android.net.Uri.parse(TestUriSamples.NAVIGATION_WITH_COORDINATES)
            },
        )

        assertNotNull(parsed)
        parsed!!
        assertFalse(parsed.lat.isNaN())
        assertFalse(parsed.lon.isNaN())
        assertEquals("Starbucks on Main Street", parsed.query)
        assertEquals(UserIntent.Navigation, parsed.intent)
        assertEquals(emptyList<Avoid>(), parsed.avoidList)
        assertEquals(TravelMode.Walk, parsed.travelMode)
    }

    @Test
    fun fromIntent_parsesAddStopDocumentedSample() {
        ApiControllerMock.mockLatLon(
            TestUriSamples.NAVIGATION_ADD_STOP,
            listOf(0.0, 0.0).toDoubleArray(),
        )
        val parsed = GeoNavigationIntent.fromIntent(
            Intent("android.intent.action.NAVIGATE").apply {
                data = android.net.Uri.parse(TestUriSamples.NAVIGATION_ADD_STOP)
            },
        )

        assertNotNull(parsed)
        parsed!!
        assertTrue(parsed.lat.isNaN())
        assertTrue(parsed.lon.isNaN())
        assertEquals(UserIntent.AddStop, parsed.intent)
        assertEquals(TravelMode.Bicycle, parsed.travelMode)
        assertEquals("1600 Amphitheatre parkway", parsed.query)
    }

    @Test
    fun fromIntent_defaultsOnInvalidOptionalValues() {
        val uri = "geo:0,0?q=Googleplex&intent=unknown&avoid=tf,x&mode=drive"
        ApiControllerMock.mockLatLon(uri, doubleArrayOf(0.0, 0.0))
        val parsed = GeoNavigationIntent.fromIntent(
            Intent("android.intent.action.NAVIGATE").apply {
                data = android.net.Uri.parse(uri)
            },
        )

        assertNotNull(parsed)
        parsed!!
        assertTrue(parsed.lat.isNaN())
        assertTrue(parsed.lon.isNaN())
        assertEquals(UserIntent.Navigation, parsed.intent)
        assertEquals(listOf(Avoid.Tolls, Avoid.Ferries), parsed.avoidList)
        assertNull(parsed.travelMode)
    }

    @Test
    fun fromIntent_defaultsTravelModeWhenSingleCharIsUnknown() {
        // "mode=z" is a single character (passes the length check) but does not match any TravelMode raw value,
        // unlike "mode=drive" which is rejected earlier for having more than one character.
        val uri = "geo:0,0?q=Googleplex&mode=z"
        ApiControllerMock.mockLatLon(uri, doubleArrayOf(0.0, 0.0))
        val parsed = GeoNavigationIntent.fromIntent(
            Intent("android.intent.action.NAVIGATE").apply {
                data = android.net.Uri.parse(uri)
            },
        )

        assertNotNull(parsed)
        assertNull(parsed!!.travelMode)
    }

    @Test
    fun fromIntent_defaultsTravelModeWhenMissing() {
        val uri = "geo:0,0?q=Googleplex"
        ApiControllerMock.mockLatLon(uri, doubleArrayOf(0.0, 0.0))
        val parsed = GeoNavigationIntent.fromIntent(
            Intent("android.intent.action.NAVIGATE").apply {
                data = android.net.Uri.parse(uri)
            },
        )

        assertNotNull(parsed)
        assertNull(parsed!!.travelMode)
    }

    @Test
    fun fromIntent_defaultsUserIntentWhenMissing() {
        val uri = "geo:0,0?q=Googleplex"
        ApiControllerMock.mockLatLon(uri, doubleArrayOf(0.0, 0.0))
        val parsed = GeoNavigationIntent.fromIntent(
            Intent("android.intent.action.NAVIGATE").apply {
                data = android.net.Uri.parse(uri)
            },
        )

        assertNotNull(parsed)
        assertEquals(UserIntent.Navigation, parsed!!.intent)
    }

    @Test
    fun fromIntent_defaultsUserIntentWhenEmpty() {
        val uri = "geo:0,0?q=Googleplex&intent="
        ApiControllerMock.mockLatLon(uri, doubleArrayOf(0.0, 0.0))
        val parsed = GeoNavigationIntent.fromIntent(
            Intent("android.intent.action.NAVIGATE").apply {
                data = android.net.Uri.parse(uri)
            },
        )

        assertNotNull(parsed)
        assertEquals(UserIntent.Navigation, parsed!!.intent)
    }

    @Test
    fun fromIntent_rejectsUnsupportedActionOrScheme() {
        assertNull(
            GeoNavigationIntent.fromIntent(
                Intent(Intent.ACTION_VIEW).apply {
                    data = android.net.Uri.parse(TestUriSamples.NAVIGATION_GOOGLEPLEX)
                },
            ),
        )
        assertNull(
            GeoNavigationIntent.fromIntent(
                Intent("android.intent.action.NAVIGATE").apply {
                    data = android.net.Uri.parse("http://example.com")
                },
            ),
        )
    }
}
