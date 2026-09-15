package app.organicmaps.intent.geo

import android.content.Intent
import android.net.Uri
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner
import org.robolectric.annotation.Config

@RunWith(RobolectricTestRunner::class)
@Config(shadows = [ShadowApiController::class])
class GeoUtilsTest {
    @Before
    fun setUp() {
        TestApiControllerHelper.reset()
    }

    @Test
    fun getQueryParameters_decodesAndIgnoresInvalidPairs() {
        val params = getQueryParameters(
            Intent(Intent.ACTION_VIEW).apply {
                data = Uri.parse("geo:0,0?q=Main%20Street&empty=&flag&encoded=a%2Bb")
            },
        )

        assertEquals("Main Street", params["q"])
        assertEquals("", params["empty"])
        assertEquals("a+b", params["encoded"])
        assertTrue(params.containsKey("flag"))
    }

    @Test
    fun getQueryParameters_returnsEmptyForMissingQueryOrData() {
        assertTrue(getQueryParameters(Intent(Intent.ACTION_VIEW)).isEmpty())
        assertTrue(
            getQueryParameters(
                Intent(Intent.ACTION_VIEW).apply {
                    data = Uri.parse("geo:0,0")
                },
            ).isEmpty(),
        )
    }

    @Test
    fun getCoordinates_returnsInvalidCoordinatesWhenNoData() {
        val coordinates = getCoordinates(
            Intent(Intent.ACTION_VIEW),
        )

        assertTrue(coordinates[0].isNaN())
        assertTrue(coordinates[1].isNaN())
    }

    @Test
    fun getCoordinates_returnsInvalidCoordinatesForBadInput() {
        val uri = Uri.parse("geo:broken")
        TestApiControllerHelper.stubLatLon(
            uri.toString(),
            listOf(Double.NaN, Double.NaN).toDoubleArray(),
        )
        val invalid = getCoordinates(
            Intent(Intent.ACTION_VIEW).apply {
                data = uri
            },
        )

        assertTrue(invalid[0].isNaN())
        assertTrue(invalid[1].isNaN())
    }
}
