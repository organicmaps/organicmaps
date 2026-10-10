package app.organicmaps.intent.geo.handlers

import android.content.Intent
import android.net.Uri
import app.organicmaps.intent.geo.FakeReportActionHandler
import app.organicmaps.intent.geo.action.AccidentType
import app.organicmaps.intent.geo.action.HazardType
import app.organicmaps.intent.geo.action.LocationOnRoad
import app.organicmaps.intent.geo.action.RoadClosureType
import app.organicmaps.intent.geo.action.RoadDirection
import app.organicmaps.intent.geo.action.TrafficType
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner

/**
 * [ReportActionHandler] methods all take optional descriptive parameters (accident/hazard/road-closure/
 * traffic type, location on road, road direction). Each of these tests covers both the fully-populated
 * case and the case(s) where individual optional parameters are missing from the intent.
 */
@RunWith(RobolectricTestRunner::class)
class ReportActionHandlerTest {
    @Test
    fun reportCrash_withAccidentTypeAndRoadDirection() {
        val handler = process("geo.action:?act=report_crash&accident_type=major&road_direction=other_side")

        assertEquals(listOf("crash:${AccidentType.Major},${RoadDirection.OtherSide}"), handler.events)
    }

    @Test
    fun reportCrash_withoutAccidentType() {
        val handler = process("geo.action:?act=report_crash&road_direction=this_side")

        assertEquals(listOf("crash:null,${RoadDirection.ThisSide}"), handler.events)
    }

    @Test
    fun reportCrash_withoutRoadDirection() {
        val handler = process("geo.action:?act=report_crash&accident_type=minor")

        assertEquals(listOf("crash:${AccidentType.Minor},null"), handler.events)
    }

    @Test
    fun reportCrash_withoutAnyOptionalParameters() {
        val handler = process("geo.action:?act=report_crash")

        assertEquals(listOf("crash:null,null"), handler.events)
    }

    @Test
    fun reportHazard_withAllOptionalParameters() {
        val handler = process(
            "geo.action:?act=report_hazard&hazard_type=fog&location_on_road=on_shoulder&road_direction=this_side",
        )

        assertEquals(
            listOf("hazard:${HazardType.Fog},${LocationOnRoad.OnShoulder},${RoadDirection.ThisSide}"),
            handler.events,
        )
    }

    @Test
    fun reportHazard_withoutHazardType() {
        val handler = process("geo.action:?act=report_hazard&location_on_road=on_road&road_direction=other_side")

        assertEquals(listOf("hazard:null,${LocationOnRoad.OnRoad},${RoadDirection.OtherSide}"), handler.events)
    }

    @Test
    fun reportHazard_withoutLocationOnRoad() {
        val handler = process("geo.action:?act=report_hazard&hazard_type=ice&road_direction=other_side")

        assertEquals(listOf("hazard:${HazardType.Ice},null,${RoadDirection.OtherSide}"), handler.events)
    }

    @Test
    fun reportHazard_withoutRoadDirection() {
        val handler = process("geo.action:?act=report_hazard&hazard_type=snow&location_on_road=on_road")

        assertEquals(listOf("hazard:${HazardType.Snow},${LocationOnRoad.OnRoad},null"), handler.events)
    }

    @Test
    fun reportHazard_withoutAnyOptionalParameters() {
        val handler = process("geo.action:?act=report_hazard")

        assertEquals(listOf("hazard:null,null,null"), handler.events)
    }

    @Test
    fun reportPolice_withRoadDirection() {
        val handler = process("geo.action:?act=report_police&road_direction=this_side")

        assertEquals(listOf("police:${RoadDirection.ThisSide}"), handler.events)
    }

    @Test
    fun reportPolice_withoutRoadDirection() {
        val handler = process("geo.action:?act=report_police")

        assertEquals(listOf("police:null"), handler.events)
    }

    @Test
    fun reportRoadClosure_withRoadClosureType() {
        val handler = process("geo.action:?act=report_road_closure&road_closure_type=partial")

        assertEquals(listOf("roadClosure:${RoadClosureType.Partial}"), handler.events)
    }

    @Test
    fun reportRoadClosure_withoutRoadClosureType() {
        val handler = process("geo.action:?act=report_road_closure")

        assertEquals(listOf("roadClosure:null"), handler.events)
    }

    @Test
    fun reportTraffic_withTrafficTypeAndRoadDirection() {
        val handler = process("geo.action:?act=report_traffic&traffic_type=heavy&road_direction=other_side")

        assertEquals(listOf("traffic:${TrafficType.Heavy},${RoadDirection.OtherSide}"), handler.events)
    }

    @Test
    fun reportTraffic_withoutTrafficType() {
        val handler = process("geo.action:?act=report_traffic&road_direction=this_side")

        assertEquals(listOf("traffic:null,${RoadDirection.ThisSide}"), handler.events)
    }

    @Test
    fun reportTraffic_withoutRoadDirection() {
        val handler = process("geo.action:?act=report_traffic&traffic_type=standstill")

        assertEquals(listOf("traffic:${TrafficType.Standstill},null"), handler.events)
    }

    @Test
    fun reportTraffic_withoutAnyOptionalParameters() {
        val handler = process("geo.action:?act=report_traffic")

        assertEquals(listOf("traffic:null,null"), handler.events)
    }

    private fun process(uri: String): FakeReportActionHandler {
        val handler = FakeReportActionHandler()
        val processor = createProcessor(reportHandler = handler)

        assertTrue(processor.processIntent(Intent(Intent.ACTION_VIEW).apply { data = Uri.parse(uri) }))

        return handler
    }
}
