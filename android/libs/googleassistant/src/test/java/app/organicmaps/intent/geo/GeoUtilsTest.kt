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
@Config(shadows = [ApiControllerMock::class])
class GeoUtilsTest {
    @Before
    fun setUp() {
        ApiControllerMock.reset()
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
    fun getQueryParameters_skipsNameWhenValueLookupReturnsNull() {
        // A literal (unescaped) '+' in a parameter name is listed by queryParameterNames as-is, but
        // getQueryParameter() re-encodes the key before matching it against the raw query, turning '+'
        // into "%2B" and failing to find a match, so it returns null for a name that IS present.
        val params = getQueryParameters(
            Intent(Intent.ACTION_VIEW).apply {
                data = Uri.parse("geo:0,0?q=Main%20Street&a+b=1")
            },
        )

        assertEquals("Main Street", params["q"])
        assertTrue(!params.containsKey("a+b"))
    }

    @Test
    fun getCoordinates_returnsValidCoordinatesWhenOnlyLatIsZero() {
        // 0 latitude (the equator) is a legitimate coordinate on its own and must not be treated as unresolved.
        val uri = Uri.parse("geo:0,1.1")
        ApiControllerMock.mockLatLon(
            uri.toString(),
            listOf(0.0, 1.1).toDoubleArray(),
        )
        val coordinates = getCoordinates(
            Intent(Intent.ACTION_VIEW).apply { data = uri },
        )

        assertEquals(0.0, coordinates[0], 0.0)
        assertEquals(1.1, coordinates[1], 0.0)
    }

    @Test
    fun getCoordinates_returnsValidCoordinatesWhenOnlyLonIsZero() {
        // 0 longitude (the prime meridian) is a legitimate coordinate on its own and must not be treated as unresolved.
        val uri = Uri.parse("geo:1.1,0")
        ApiControllerMock.mockLatLon(
            uri.toString(),
            listOf(1.1, 0.0).toDoubleArray(),
        )
        val coordinates = getCoordinates(
            Intent(Intent.ACTION_VIEW).apply { data = uri },
        )

        assertEquals(1.1, coordinates[0], 0.0)
        assertEquals(0.0, coordinates[1], 0.0)
    }

    @Test
    fun getCoordinates_returnsInvalidCoordinatesWhenLatLonAreZeros() {
        // (0, 0) is the sentinel used for "no coordinates provided", e.g. `geo:0,0?q=...`.
        val uri = Uri.parse("geo:0,0")
        ApiControllerMock.mockLatLon(
            uri.toString(),
            listOf(0.0, 0.0).toDoubleArray(),
        )
        val coordinates = getCoordinates(
            Intent(Intent.ACTION_VIEW).apply { data = uri },
        )

        assertTrue(coordinates[0].isNaN())
        assertTrue(coordinates[1].isNaN())
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
        ApiControllerMock.mockLatLon(
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

    @Test
    fun getCoordinates_returnsInvalidCoordinatesWhenNativeParsingReturnsNull() {
        val uri = Uri.parse("geo:unparseable")
        ApiControllerMock.mockLatLon(uri.toString(), null)
        val coordinates = getCoordinates(
            Intent(Intent.ACTION_VIEW).apply { data = uri },
        )

        assertTrue(coordinates[0].isNaN())
        assertTrue(coordinates[1].isNaN())
    }

    @Test
    fun getCoordinates_returnsInvalidCoordinatesWhenArraySizeIsNotTwo() {
        val emptyUri = Uri.parse("geo:empty")
        ApiControllerMock.mockLatLon(emptyUri.toString(), doubleArrayOf())
        val fromEmpty = getCoordinates(
            Intent(Intent.ACTION_VIEW).apply { data = emptyUri },
        )

        val singleUri = Uri.parse("geo:single")
        ApiControllerMock.mockLatLon(singleUri.toString(), doubleArrayOf(1.1))
        val fromSingle = getCoordinates(
            Intent(Intent.ACTION_VIEW).apply { data = singleUri },
        )

        val tooManyUri = Uri.parse("geo:toomany")
        ApiControllerMock.mockLatLon(tooManyUri.toString(), doubleArrayOf(1.1, 2.2, 3.3))
        val fromTooMany = getCoordinates(
            Intent(Intent.ACTION_VIEW).apply { data = tooManyUri },
        )

        assertTrue(fromEmpty[0].isNaN())
        assertTrue(fromEmpty[1].isNaN())
        assertTrue(fromSingle[0].isNaN())
        assertTrue(fromSingle[1].isNaN())
        assertTrue(fromTooMany[0].isNaN())
        assertTrue(fromTooMany[1].isNaN())
    }
}
