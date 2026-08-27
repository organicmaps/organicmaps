package app.organicmaps.intent.geo.handlers

import android.content.Intent
import android.net.Uri
import app.organicmaps.intent.geo.ApiControllerMock
import app.organicmaps.intent.geo.FakeSearchActionHandler
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner
import org.robolectric.annotation.Config

/**
 * [SearchActionHandler.search] is reached via a view (`geo:...?q=`) intent and can carry either
 * resolved coordinates (search biased nearby) or no coordinates (plain query search). The remaining
 * methods are reached via `geo.action:` intents and take no coordinates.
 */
@RunWith(RobolectricTestRunner::class)
@Config(shadows = [ApiControllerMock::class])
class SearchActionHandlerTest {
    @Before
    fun setUp() {
        ApiControllerMock.reset()
    }

    @Test
    fun search_queryOnly_coordinatesNotResolved() {
        val uri = "geo:0,0?q=restaurants+nearby"
        ApiControllerMock.mockLatLon(uri, doubleArrayOf(Double.NaN, Double.NaN))
        val handler = FakeSearchActionHandler()

        assertTrue(processView(handler, uri))

        assertEquals(listOf("search:NaN,NaN,restaurants nearby"), handler.events)
    }

    @Test
    fun search_queryWithCoordinates_biasesSearchToCoordinates() {
        val uri = "geo:1.1,2.2?q=coffee+shop"
        ApiControllerMock.mockLatLon(uri, doubleArrayOf(1.1, 2.2))
        val handler = FakeSearchActionHandler()

        assertTrue(processView(handler, uri))

        assertEquals(listOf("search:1.1,2.2,coffee shop"), handler.events)
    }

    @Test
    fun clearSearchResults() {
        val handler = FakeSearchActionHandler()

        assertTrue(processAction(handler, "geo.action:?act=clear_search_results"))

        assertEquals(listOf("clear"), handler.events)
    }

    @Test
    fun selectSearchResult_withExplicitId() {
        val handler = FakeSearchActionHandler()

        assertTrue(processAction(handler, "geo.action:?act=select_search_result&id=4"))

        assertEquals(listOf("select:4"), handler.events)
    }

    @Test
    fun selectSearchResult_withoutId_usesDefault() {
        val handler = FakeSearchActionHandler()

        assertTrue(processAction(handler, "geo.action:?act=select_search_result"))

        assertEquals(listOf("select:0"), handler.events)
    }

    @Test
    fun electricVehicleConnectorFilter_applyAndRemove() {
        val handler = FakeSearchActionHandler()

        assertTrue(processAction(handler, "geo.action:?act=apply_electric_vehicle_connector_filter"))
        assertTrue(processAction(handler, "geo.action:?act=remove_electric_vehicle_connector_filter"))

        assertEquals(listOf("connector:true", "connector:false"), handler.events)
    }

    @Test
    fun electricVehiclePaymentFilter_applyAndRemove() {
        val handler = FakeSearchActionHandler()

        assertTrue(processAction(handler, "geo.action:?act=apply_electric_vehicle_payment_filter"))
        assertTrue(processAction(handler, "geo.action:?act=remove_electric_vehicle_payment_filter"))

        assertEquals(listOf("payment:true", "payment:false"), handler.events)
    }

    @Test
    fun electricVehicleFastChargingFilter_applyAndRemove() {
        val handler = FakeSearchActionHandler()

        assertTrue(processAction(handler, "geo.action:?act=apply_electric_vehicle_fast_charging_filter"))
        assertTrue(processAction(handler, "geo.action:?act=remove_electric_vehicle_fast_charging_filter"))

        assertEquals(listOf("fast:true", "fast:false"), handler.events)
    }

    private fun processView(handler: FakeSearchActionHandler, uri: String): Boolean {
        val processor = createProcessor(searchHandler = handler)
        return processor.processIntent(Intent(Intent.ACTION_VIEW).apply { data = Uri.parse(uri) })
    }

    private fun processAction(handler: FakeSearchActionHandler, uri: String): Boolean {
        val processor = createProcessor(searchHandler = handler)
        return processor.processIntent(Intent(Intent.ACTION_VIEW).apply { data = Uri.parse(uri) })
    }
}
