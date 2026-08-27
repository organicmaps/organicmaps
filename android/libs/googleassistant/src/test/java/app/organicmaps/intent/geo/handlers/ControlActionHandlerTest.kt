package app.organicmaps.intent.geo.handlers

import android.content.Intent
import android.net.Uri
import app.organicmaps.intent.geo.FakeControlActionHandler
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner

/** [ControlActionHandler] is reached via `geo.action:` intents, one per supported `act` value. */
@RunWith(RobolectricTestRunner::class)
class ControlActionHandlerTest {
    @Test
    fun exitNavigation() = assertAction("exit_navigation", "exitNavigation")

    @Test
    fun followMode() = assertAction("follow_mode", "followMode")

    @Test
    fun goBack() = assertAction("go_back", "goBack")

    @Test
    fun hideSatellite() = assertAction("hide_satellite", "satellite:false")

    @Test
    fun hideTraffic() = assertAction("hide_traffic", "traffic:false")

    @Test
    fun routeOverview() = assertAction("route_overview", "routeOverview")

    @Test
    fun showAlternativeRoutes() = assertAction("show_alternative_routes", "alternativeRoutes")

    @Test
    fun showAlternates_isAliasForShowAlternativeRoutes() = assertAction("show_alternates", "alternativeRoutes")

    @Test
    fun showDirectionsList() = assertAction("show_directions_list", "directionsList")

    @Test
    fun showSatellite() = assertAction("show_satellite", "satellite:true")

    @Test
    fun showTraffic() = assertAction("show_traffic", "traffic:true")

    @Test
    fun myLocation() = assertAction("my_location", "myLocation")

    @Test
    fun showMap() = assertAction("show_map", "showMap")

    private fun assertAction(act: String, expectedEvent: String) {
        val handler = FakeControlActionHandler()
        val processor = createProcessor(controlHandler = handler)

        assertTrue(
            processor.processIntent(
                Intent(Intent.ACTION_VIEW).apply { data = Uri.parse("geo.action:?act=$act") },
            ),
        )

        assertEquals(listOf(expectedEvent), handler.events)
    }
}
