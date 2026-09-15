package app.organicmaps.intent.geo

import android.content.Intent
import app.organicmaps.intent.geo.action.AccidentType
import app.organicmaps.intent.geo.action.HazardType
import app.organicmaps.intent.geo.action.LocationOnRoad
import app.organicmaps.intent.geo.action.RoadClosureType
import app.organicmaps.intent.geo.action.RoadDirection
import app.organicmaps.intent.geo.action.TrafficType
import app.organicmaps.intent.geo.handlers.AvoidActionHandler
import app.organicmaps.intent.geo.handlers.ControlActionHandler
import app.organicmaps.intent.geo.handlers.NavigationActionHandler
import app.organicmaps.intent.geo.handlers.ReportActionHandler
import app.organicmaps.intent.geo.handlers.SearchActionHandler
import app.organicmaps.intent.geo.handlers.VoiceActionHandler
import org.junit.Assert.assertTrue
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class GeoIntentProcessorHandlersTest {
    @Test
    fun processIntent_routesAvoidActions() {
        val avoidHandler = FakeAvoidActionHandler()
        val processor = createProcessor(avoidHandler = avoidHandler)

        listOf(
            "geo.action:?act=allow_ferries" to "ferries:true",
            "geo.action:?act=allow_highways" to "highways:true",
            "geo.action:?act=allow_tolls" to "tolls:true",
            "geo.action:?act=avoid_ferries" to "ferries:false",
            "geo.action:?act=avoid_highways" to "highways:false",
            "geo.action:?act=avoid_tolls" to "tolls:false",
        ).forEach { (uri, expected) ->
            assertTrue(processor.processIntent(Intent(Intent.ACTION_VIEW).apply { data = android.net.Uri.parse(uri) }))
            assertTrue(avoidHandler.events.contains(expected))
        }
    }

    @Test
    fun processIntent_routesControlActions() {
        val controlHandler = FakeControlActionHandler()
        val processor = createProcessor(controlHandler = controlHandler)

        listOf(
            TestUriSamples.ACTION_EXIT_NAVIGATION to "exitNavigation",
            "geo.action:?act=follow_mode" to "followMode",
            "geo.action:?act=go_back" to "goBack",
            "geo.action:?act=hide_satellite" to "satellite:false",
            "geo.action:?act=hide_traffic" to "traffic:false",
            "geo.action:?act=route_overview" to "routeOverview",
            "geo.action:?act=show_alternative_routes" to "alternativeRoutes",
            "geo.action:?act=show_alternates" to "alternativeRoutes",
            "geo.action:?act=show_directions_list" to "directionsList",
            "geo.action:?act=show_satellite" to "satellite:true",
            "geo.action:?act=show_traffic" to "traffic:true",
            "geo.action:?act=my_location" to "myLocation",
            "geo.action:?act=show_map" to "showMap",
        ).forEach { (uri, expected) ->
            assertTrue(processor.processIntent(Intent(Intent.ACTION_VIEW).apply { data = android.net.Uri.parse(uri) }))
            assertTrue(controlHandler.events.contains(expected))
        }
    }

    @Test
    fun processIntent_routesVoiceActions() {
        val voiceHandler = FakeVoiceActionHandler()
        val processor = createProcessor(voiceHandler = voiceHandler)

        listOf(
            "geo.action:?act=distance_to_destination" to "distanceToDestination",
            "geo.action:?act=distance_to_next_turn" to "distanceToNextTurn",
            "geo.action:?act=eta" to "eta",
            TestUriSamples.ACTION_MUTE to "voice:false",
            "geo.action:?act=query_current_road" to "currentRoad",
            "geo.action:?act=query_destination" to "destination",
            "geo.action:?act=query_next_turn" to "nextTurn",
            "geo.action:?act=time_to_destination" to "timeToDestination",
            "geo.action:?act=time_to_next_turn" to "timeToNextTurn",
            "geo.action:?act=unmute" to "voice:true",
            "geo.action:?act=traffic_report" to "trafficReport",
        ).forEach { (uri, expected) ->
            assertTrue(processor.processIntent(Intent(Intent.ACTION_VIEW).apply { data = android.net.Uri.parse(uri) }))
            assertTrue(voiceHandler.events.contains(expected))
        }
    }

    @Test
    fun processIntent_routesReportAndSearchActions() {
        val reportHandler = FakeReportActionHandler()
        val searchHandler = FakeSearchActionHandler()
        val processor = createProcessor(reportHandler = reportHandler, searchHandler = searchHandler)

        assertTrue(
            processor.processIntent(
                Intent(Intent.ACTION_VIEW).apply {
                    data = android.net.Uri.parse(
                        "geo.action:?act=report_crash&accident_type=major&road_direction=other_side",
                    )
                },
            ),
        )
        assertTrue(
            processor.processIntent(
                Intent(Intent.ACTION_VIEW).apply {
                    data = android.net.Uri.parse(
                        "geo.action:?act=report_hazard&hazard_type=fog&location_on_road=on_shoulder&road_direction=this_side",
                    )
                },
            ),
        )
        assertTrue(
            processor.processIntent(
                Intent(Intent.ACTION_VIEW).apply {
                    data = android.net.Uri.parse("geo.action:?act=report_police&road_direction=this_side")
                },
            ),
        )
        assertTrue(
            processor.processIntent(
                Intent(Intent.ACTION_VIEW).apply {
                    data = android.net.Uri.parse("geo.action:?act=report_road_closure&road_closure_type=partial")
                },
            ),
        )
        assertTrue(
            processor.processIntent(
                Intent(Intent.ACTION_VIEW).apply {
                    data =
                        android.net.Uri.parse(
                            "geo.action:?act=report_traffic&traffic_type=heavy&road_direction=other_side",
                        )
                },
            ),
        )

        listOf(
            "geo.action:?act=clear_search_results" to "clear",
            "geo.action:?act=select_search_result&id=4" to "select:4",
            "geo.action:?act=apply_electric_vehicle_connector_filter" to "connector:true",
            "geo.action:?act=remove_electric_vehicle_connector_filter" to "connector:false",
            "geo.action:?act=apply_electric_vehicle_payment_filter" to "payment:true",
            "geo.action:?act=remove_electric_vehicle_payment_filter" to "payment:false",
            "geo.action:?act=apply_electric_vehicle_fast_charging_filter" to "fast:true",
            "geo.action:?act=remove_electric_vehicle_fast_charging_filter" to "fast:false",
        ).forEach { (uri, expected) ->
            assertTrue(processor.processIntent(Intent(Intent.ACTION_VIEW).apply { data = android.net.Uri.parse(uri) }))
            assertTrue(searchHandler.events.contains(expected))
        }

        assertTrue(reportHandler.events.contains("crash:${AccidentType.Major},${RoadDirection.OtherSide}"))
        assertTrue(
            reportHandler.events.contains(
                "hazard:${HazardType.Fog},${LocationOnRoad.OnShoulder},${RoadDirection.ThisSide}",
            ),
        )
        assertTrue(reportHandler.events.contains("police:${RoadDirection.ThisSide}"))
        assertTrue(reportHandler.events.contains("roadClosure:${RoadClosureType.Partial}"))
        assertTrue(reportHandler.events.contains("traffic:${TrafficType.Heavy},${RoadDirection.OtherSide}"))
    }

    private fun createProcessor(
        avoidHandler: AvoidActionHandler = FakeAvoidActionHandler(),
        controlHandler: ControlActionHandler = FakeControlActionHandler(),
        navigationHandler: NavigationActionHandler = FakeNavigationActionHandler(),
        reportHandler: ReportActionHandler = FakeReportActionHandler(),
        searchHandler: SearchActionHandler = FakeSearchActionHandler(),
        voiceHandler: VoiceActionHandler = FakeVoiceActionHandler(),
    ) = GeoIntentProcessor(
        avoidHandler,
        controlHandler,
        navigationHandler,
        reportHandler,
        searchHandler,
        voiceHandler,
    )
}
