package app.organicmaps.routing

import android.app.Application
import android.view.ContextThemeWrapper
import android.view.LayoutInflater
import android.view.View
import android.widget.TextView
import app.organicmaps.R
import app.organicmaps.sdk.Framework
import app.organicmaps.sdk.routing.RouteAltitudeData
import app.organicmaps.sdk.settings.UnitLocale
import app.organicmaps.sdk.util.Distance
import app.organicmaps.sdk.util.StringUtils
import app.organicmaps.widget.placepage.ElevationProfileChart
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner
import org.robolectric.RuntimeEnvironment
import org.robolectric.annotation.Config
import org.robolectric.annotation.Implementation
import org.robolectric.annotation.Implements

@RunWith(RobolectricTestRunner::class)
@Config(
    sdk = [34],
    application = Application::class,
    shadows = [
        RouteElevationChartControllerTest.Units::class,
        RouteElevationChartControllerTest.Formatting::class,
        RouteElevationChartControllerTest.AltitudeFormatting::class,
    ],
)
class RouteElevationChartControllerTest {
    private lateinit var root: View
    private lateinit var chart: ElevationProfileChart
    private lateinit var controller: RouteElevationChartController
    private var deselections = 0

    @Before
    fun showChart() {
        val context = ContextThemeWrapper(RuntimeEnvironment.getApplication(), R.style.MwmTheme)
        root = LayoutInflater.from(context).inflate(R.layout.route_elevation_profile, null)
        chart = root.findViewById(R.id.elevation_profile_chart)
        root.measure(exactly(400), exactly(200))
        root.layout(0, 0, 400, 200)
        controller = RouteElevationChartController(root)
        controller.setListener(
            object : RouteElevationChartController.ElevationSelectionListener {
                override fun onElevationPointSelected(distanceMeters: Double) = Unit

                override fun onElevationPointDeselected() {
                    ++deselections
                }

                override fun onElevationSelectionFinished(distanceMeters: Double) = Unit
            },
        )
    }

    @Test
    fun replayedProfileKeepsItsSelection() {
        controller.setData(profile(0.0, 100.0))
        controller.select(30.0)
        controller.setData(profile(0.0, 100.0))
        assertEquals(30.0, controller.selectedDistance, 0.0)
        assertEquals(0, deselections)
    }

    @Test
    fun anotherRouteDropsTheSelection() {
        controller.setData(profile(0.0, 100.0))
        assertEquals(0, deselections)
        controller.select(30.0)
        controller.setData(profile(0.0, 80.0))
        assertTrue(controller.selectedDistance.isNaN())
        // The map marker goes with the chart selection.
        assertEquals(1, deselections)
    }

    @Test
    fun chartIncludesTheHillBeforeTheEntry() {
        controller.setData(
            RouteAltitudeData(doubleArrayOf(0.0, 400.0, 1000.0), intArrayOf(100, 300, 120), 0, 45, 1.75, 2),
        )
        val followed = chart.data.getDataSetByIndex(1)
        assertEquals(850f, followed.getEntryForIndex(0).x, 0f)
        assertEquals(165f, followed.getEntryForIndex(0).y, 0f)
        assertEquals(1000f, followed.xMax, 0f)
        assertTrue(chart.axisLeft.axisMaximum >= 300f)
        assertTrue(chart.axisLeft.axisMinimum <= 100f)
        assertEquals("300 m", root.findViewById<TextView>(R.id.highest_altitude).text)
        assertEquals("100 m", root.findViewById<TextView>(R.id.lowest_altitude).text)
    }

    @Test
    fun reverseAtAGapEndsOnTheEarlierSection() {
        controller.setData(
            RouteAltitudeData(doubleArrayOf(0.0, 100.0, 100.0, 200.0), intArrayOf(100, 130, 210, 190), 0, 30, 1.0, 0),
        )
        val followed = chart.data.getDataSetByIndex(1)
        assertEquals(2, followed.entryCount)
        assertEquals(100f, followed.xMax, 0f)
        assertEquals(130f, followed.yMax, 0f)
    }

    @Test
    fun entryAfterAGapKeepsItsOwnHeight() {
        controller.setData(
            RouteAltitudeData(doubleArrayOf(0.0, 100.0, 100.0, 200.0), intArrayOf(100, 130, 210, 190), 0, 20, 2.0, 3),
        )
        controller.select(100.0)
        assertEquals(210f, chart.highlighted.single().y, 0f)
        controller.select(50.0)
        assertEquals(115f, chart.highlighted.single().y, 0f)
    }

    private fun profile(vararg distances: Double) =
        RouteAltitudeData(distances, IntArray(distances.size) { it * 10 }, 0, 0, 0.0, -1)

    private fun exactly(size: Int) = View.MeasureSpec.makeMeasureSpec(size, View.MeasureSpec.EXACTLY)

    @Implements(UnitLocale::class)
    object Units {
        @JvmStatic
        @Implementation
        fun getCurrentUnits() = UnitLocale.UNITS_METRIC
    }

    @Implements(Framework::class)
    object AltitudeFormatting {
        @JvmStatic
        @Implementation
        fun nativeFormatAltitude(altitude: Double) = "${altitude.toInt()} m"
    }

    @Implements(StringUtils::class)
    object Formatting {
        @JvmStatic
        @Implementation
        fun nativeFormatDistance(meters: Double) = Distance(meters, meters.toString(), 0)
    }
}
