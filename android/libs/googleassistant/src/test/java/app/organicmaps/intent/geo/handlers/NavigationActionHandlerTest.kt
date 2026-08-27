package app.organicmaps.intent.geo.handlers

import android.content.Intent
import android.net.Uri
import app.organicmaps.intent.geo.ApiControllerMock
import app.organicmaps.intent.geo.FakeNavigationActionHandler
import app.organicmaps.intent.geo.navigation.Avoid
import app.organicmaps.intent.geo.navigation.TravelMode
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner
import org.robolectric.annotation.Config

/**
 * [NavigationActionHandler] methods can be reached with three different parameter shapes, per the
 * documented contract on [NavigationActionHandler.navigate]:
 *  1. `query` is null/empty -> the destination is the coordinates.
 *  2. `query` is set, coordinates are not resolved -> the destination is the place resolved from the query.
 *  3. `query` is set, coordinates are resolved -> the destination is the place resolved from the query, biased to
 *     the coordinates.
 *
 * These tests exercise all three shapes for `navigate`, `addStop` and `showDirections`, plus the optional
 * `avoid` and `travelMode` parameters.
 */
@RunWith(RobolectricTestRunner::class)
@Config(shadows = [ApiControllerMock::class])
class NavigationActionHandlerTest {
    @Before
    fun setUp() {
        ApiControllerMock.reset()
    }

    @Test
    fun navigate_coordinatesOnly_noQuery() {
        val uri = "geo:12.3,45.6?intent=navigation"
        ApiControllerMock.mockLatLon(uri, doubleArrayOf(12.3, 45.6))
        val handler = FakeNavigationActionHandler()

        assertTrue(process(handler, uri))

        assertEquals(listOf("navigate:12.3,45.6,null,[],null"), handler.events)
    }

    @Test
    fun navigate_queryOnly_coordinatesNotResolved() {
        val uri = "geo:0,0?q=Coffee+Shop&intent=navigation"
        ApiControllerMock.mockLatLon(uri, doubleArrayOf(Double.NaN, Double.NaN))
        val handler = FakeNavigationActionHandler()

        assertTrue(process(handler, uri))

        assertEquals(listOf("navigate:NaN,NaN,Coffee Shop,[],null"), handler.events)
    }

    @Test
    fun navigate_queryAndCoordinates_biasesSearchToCoordinates() {
        val uri = "geo:1.1,2.2?q=Starbucks&intent=navigation"
        ApiControllerMock.mockLatLon(uri, doubleArrayOf(1.1, 2.2))
        val handler = FakeNavigationActionHandler()

        assertTrue(process(handler, uri))

        assertEquals(listOf("navigate:1.1,2.2,Starbucks,[],null"), handler.events)
    }

    @Test
    fun navigate_withAvoidListAndTravelMode() {
        val uri = "geo:1.1,2.2?q=Starbucks&intent=navigation&avoid=tf&mode=w"
        ApiControllerMock.mockLatLon(uri, doubleArrayOf(1.1, 2.2))
        val handler = FakeNavigationActionHandler()

        assertTrue(process(handler, uri))

        assertEquals(
            listOf("navigate:1.1,2.2,Starbucks,[${Avoid.Tolls}, ${Avoid.Ferries}],${TravelMode.Walk}"),
            handler.events,
        )
    }

    @Test
    fun addStop_coordinatesOnly_noQuery() {
        val uri = "geo:12.3,45.6?intent=add_a_stop"
        ApiControllerMock.mockLatLon(uri, doubleArrayOf(12.3, 45.6))
        val handler = FakeNavigationActionHandler()

        assertTrue(process(handler, uri))

        assertEquals(listOf("addStop:12.3,45.6,null"), handler.events)
    }

    @Test
    fun addStop_queryOnly_coordinatesNotResolved() {
        val uri = "geo:0,0?q=1600+Amphitheatre+parkway&intent=add_a_stop"
        ApiControllerMock.mockLatLon(uri, doubleArrayOf(Double.NaN, Double.NaN))
        val handler = FakeNavigationActionHandler()

        assertTrue(process(handler, uri))

        assertEquals(listOf("addStop:NaN,NaN,1600 Amphitheatre parkway"), handler.events)
    }

    @Test
    fun addStop_queryAndCoordinates_biasesSearchToCoordinates() {
        val uri = "geo:1.1,2.2?q=Gas+Station&intent=add_a_stop"
        ApiControllerMock.mockLatLon(uri, doubleArrayOf(1.1, 2.2))
        val handler = FakeNavigationActionHandler()

        assertTrue(process(handler, uri))

        assertEquals(listOf("addStop:1.1,2.2,Gas Station"), handler.events)
    }

    @Test
    fun showDirections_coordinatesOnly_noQuery() {
        val uri = "geo:12.3,45.6?intent=directions"
        ApiControllerMock.mockLatLon(uri, doubleArrayOf(12.3, 45.6))
        val handler = FakeNavigationActionHandler()

        assertTrue(process(handler, uri))

        assertEquals(listOf("directions:12.3,45.6,null,[],null"), handler.events)
    }

    @Test
    fun showDirections_queryOnly_coordinatesNotResolved() {
        val uri = "geo:0,0?q=Museum&intent=directions"
        ApiControllerMock.mockLatLon(uri, doubleArrayOf(Double.NaN, Double.NaN))
        val handler = FakeNavigationActionHandler()

        assertTrue(process(handler, uri))

        assertEquals(listOf("directions:NaN,NaN,Museum,[],null"), handler.events)
    }

    @Test
    fun showDirections_queryAndCoordinates_withAvoidAndTravelMode() {
        val uri = "geo:1.1,2.2?q=Museum&intent=directions&avoid=h&mode=d"
        ApiControllerMock.mockLatLon(uri, doubleArrayOf(1.1, 2.2))
        val handler = FakeNavigationActionHandler()

        assertTrue(process(handler, uri))

        assertEquals(
            listOf("directions:1.1,2.2,Museum,[${Avoid.Highways}],${TravelMode.Drive}"),
            handler.events,
        )
    }

    private fun process(handler: FakeNavigationActionHandler, uri: String): Boolean {
        val processor = createProcessor(navigationHandler = handler)
        return processor.processIntent(
            Intent("android.intent.action.NAVIGATE").apply { data = Uri.parse(uri) },
        )
    }
}
