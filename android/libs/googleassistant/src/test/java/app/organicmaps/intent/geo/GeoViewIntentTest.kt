package app.organicmaps.intent.geo

import android.content.Intent
import org.junit.Assert.assertEquals
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
class GeoViewIntentTest {
    @Before
    fun setUp() {
        ApiControllerMock.reset()
    }

    @Test
    fun fromIntent_parsesDocumentedSample() {
        ApiControllerMock.mockLatLon(
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
        assertTrue(parsed.lat.isNaN())
        assertTrue(parsed.lon.isNaN())
        assertEquals("restaurants nearby", parsed.query)
    }

    @Test
    fun fromIntent_requiresSupportedInput() {
        ApiControllerMock.mockLatLon(
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
