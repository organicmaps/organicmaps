package app.organicmaps.intent.geo

import android.content.Intent
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertNull
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner
import org.robolectric.annotation.Config

@RunWith(RobolectricTestRunner::class)
@Config(shadows = [ShadowApiController::class])
class GeoViewIntentTest {
    @Before
    fun setUp() {
        TestApiControllerHelper.reset()
    }

    @Test
    fun fromIntent_parsesDocumentedSample() {
        TestApiControllerHelper.stubLatLon(
            TestUriSamples.SEARCH_NEARBY,
            listOf(0.0, 0.0).toDoubleArray(),
        )
        val parsed = GeoViewIntent.fromIntent(
            Intent(Intent.ACTION_VIEW).apply {
                data = android.net.Uri.parse(TestUriSamples.SEARCH_NEARBY)
            },
        )

        assertNotNull(parsed)
        parsed!!
        assertFalse(parsed.lat.isNaN())
        assertFalse(parsed.lon.isNaN())
        assertEquals("restaurants nearby", parsed.query)
    }

    @Test
    fun fromIntent_requiresSupportedInput() {
        TestApiControllerHelper.stubLatLon(
            TestUriSamples.SEARCH_NEARBY,
            listOf(0.0, 0.0).toDoubleArray(),
        )
        assertNull(
            GeoViewIntent.fromIntent(
                Intent("android.intent.action.NAVIGATE").apply {
                    data = android.net.Uri.parse(TestUriSamples.SEARCH_NEARBY)
                },
            ),
        )
        assertNull(
            GeoViewIntent.fromIntent(
                Intent(Intent.ACTION_VIEW).apply {
                    data = android.net.Uri.parse("geo:0,0")
                },
            ),
        )
        assertNull(
            GeoViewIntent.fromIntent(
                Intent(Intent.ACTION_VIEW).apply {
                    data = android.net.Uri.parse("http://example.com?q=restaurants+nearby")
                },
            ),
        )
    }
}
