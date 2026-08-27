package app.organicmaps.intent.geo

import android.content.Intent
import android.net.Uri
import app.organicmaps.intent.geo.action.AccidentType
import app.organicmaps.intent.geo.action.ActionType
import app.organicmaps.intent.geo.action.HazardType
import app.organicmaps.intent.geo.action.LocationOnRoad
import app.organicmaps.intent.geo.action.RoadClosureType
import app.organicmaps.intent.geo.action.RoadDirection
import app.organicmaps.intent.geo.action.TrafficType
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertNull
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class GeoActionIntentTest {
    @Test
    fun fromIntent_parsesDocumentedSamples() {
        val reportCrash = GeoActionIntent.fromIntent(
            Intent(Intent.ACTION_VIEW).apply {
                data = Uri.parse(TestUriSamples.ACTION_REPORT_CRASH)!!
            },
        )
        val mute = GeoActionIntent.fromIntent(
            Intent(Intent.ACTION_VIEW).apply {
                data = Uri.parse(TestUriSamples.ACTION_MUTE)
            },
        )
        val exitNavigation = GeoActionIntent.fromIntent(
            Intent(Intent.ACTION_VIEW).apply {
                data = Uri.parse(TestUriSamples.ACTION_EXIT_NAVIGATION)
            },
        )

        assertNotNull(reportCrash)
        reportCrash!!
        assertEquals(ActionType.ReportCrash, reportCrash.actionType)
        assertEquals(AccidentType.Major, reportCrash.accidentType)
        assertEquals(ActionType.Mute, mute?.actionType)
        assertEquals(ActionType.ExitNavigation, exitNavigation?.actionType)
    }

    @Test
    fun fromIntent_parsesOptionalPayloads() {
        val hazard = GeoActionIntent.fromIntent(
            Intent(Intent.ACTION_VIEW).apply {
                data = Uri.parse(
                    "geo.action:?act=report_hazard&hazard_type=fog&location_on_road=on_road&road_direction=other_side",
                )
            },
        )
        val search = GeoActionIntent.fromIntent(
            Intent(Intent.ACTION_VIEW).apply {
                data = Uri.parse("geo.action.offline:?act=select_search_result&id=3")
            },
        )
        val traffic = GeoActionIntent.fromIntent(
            Intent(Intent.ACTION_VIEW).apply {
                data = Uri.parse(
                    "geo.action:?act=report_traffic&traffic_type=standstill&road_direction=this_side" +
                        "&road_closure_type=full",
                )
            },
        )

        assertEquals(ActionType.ReportHazard, hazard?.actionType)
        assertEquals(HazardType.Fog, hazard?.hazardType)
        assertEquals(LocationOnRoad.OnRoad, hazard?.locationOnRoad)
        assertEquals(RoadDirection.OtherSide, hazard?.roadDirection)
        assertEquals(ActionType.SelectSearchResult, search?.actionType)
        assertEquals(3, search?.searchId)
        assertEquals(TrafficType.Standstill, traffic?.trafficType)
        assertEquals(RoadClosureType.Full, traffic?.roadClosureType)
    }

    @Test
    fun fromIntent_defaultsSearchIdWhenMissingOrInvalid() {
        val missing = GeoActionIntent.fromIntent(
            Intent(Intent.ACTION_VIEW).apply {
                data = Uri.parse("geo.action:?act=select_search_result")
            },
        )
        val negative = GeoActionIntent.fromIntent(
            Intent(Intent.ACTION_VIEW).apply {
                data = Uri.parse("geo.action:?act=select_search_result&id=-1")
            },
        )
        val invalid = GeoActionIntent.fromIntent(
            Intent(Intent.ACTION_VIEW).apply {
                data = Uri.parse("geo.action:?act=select_search_result&id=abc")
            },
        )

        assertEquals(GeoActionIntent.DEFAULT_SEARCH_ID, missing?.searchId)
        assertEquals(GeoActionIntent.DEFAULT_SEARCH_ID, negative?.searchId)
        assertEquals(GeoActionIntent.DEFAULT_SEARCH_ID, invalid?.searchId)
    }

    @Test
    fun fromIntent_defaultsSearchIdSilentlyForNonSelectSearchResultActions() {
        // An "id" param is only meaningful for select_search_result; for any other action type an
        // invalid/present id should still default silently (no "Invalid search id" warning branch).
        val parsed = GeoActionIntent.fromIntent(
            Intent(Intent.ACTION_VIEW).apply {
                data = Uri.parse("geo.action:?act=exit_navigation&id=abc")
            },
        )

        assertEquals(ActionType.ExitNavigation, parsed?.actionType)
        assertEquals(GeoActionIntent.DEFAULT_SEARCH_ID, parsed?.searchId)
    }

    @Test
    fun fromIntent_rejectsUnsupportedInputAndInvalidAction() {
        assertNull(
            GeoActionIntent.fromIntent(
                Intent("android.intent.action.NAVIGATE").apply {
                    data = Uri.parse(TestUriSamples.ACTION_REPORT_CRASH)
                },
            ),
        )
        assertNull(
            GeoActionIntent.fromIntent(
                Intent(Intent.ACTION_VIEW).apply {
                    data = Uri.parse("geo:0,0?act=report_crash")
                },
            ),
        )
        assertNull(
            GeoActionIntent.fromIntent(
                Intent(Intent.ACTION_VIEW).apply {
                    data = Uri.parse("geo.action:?act=unknown")
                },
            ),
        )
    }
}
