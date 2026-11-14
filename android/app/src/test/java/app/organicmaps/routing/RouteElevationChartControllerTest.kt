package app.organicmaps.routing

import android.app.Application
import android.view.ContextThemeWrapper
import android.view.View
import android.widget.FrameLayout
import app.organicmaps.R
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
    shadows = [RouteElevationChartControllerTest.Units::class, RouteElevationChartControllerTest.Formatting::class],
)
class RouteElevationChartControllerTest {
    private lateinit var controller: RouteElevationChartController
    private var deselections = 0

    @Before
    fun showChart() {
        val context = ContextThemeWrapper(RuntimeEnvironment.getApplication(), R.style.MwmTheme)
        val root = FrameLayout(context)
        val chart = ElevationProfileChart(context).apply { id = R.id.elevation_profile_chart }
        root.addView(chart)
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

    private fun profile(vararg distances: Double) =
        RouteAltitudeData(distances, IntArray(distances.size) { it * 10 }, 0, 0, 0, 10)

    private fun exactly(size: Int) = View.MeasureSpec.makeMeasureSpec(size, View.MeasureSpec.EXACTLY)

    @Implements(UnitLocale::class)
    object Units {
        @JvmStatic
        @Implementation
        fun getCurrentUnits() = UnitLocale.UNITS_METRIC
    }

    @Implements(StringUtils::class)
    object Formatting {
        @JvmStatic
        @Implementation
        fun nativeFormatDistance(meters: Double) = Distance(meters, meters.toString(), 0)
    }
}
