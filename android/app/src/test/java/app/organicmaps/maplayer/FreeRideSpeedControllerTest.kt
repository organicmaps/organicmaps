package app.organicmaps.maplayer

import app.organicmaps.sdk.Framework
import app.organicmaps.sdk.widgets.speedlimit.Shape
import kotlin.math.roundToInt
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

class FreeRideSpeedControllerTest {
    private class FakeDisplay : FreeRideSpeedController.SpeedDisplay {
        var displayedSpeed: Int = 0
            private set
        var displayedSpeedLimit: Int? = null
            private set
        var displayedUnits: String = ""
            private set
        var displayedShape: Shape = Shape.Circle
            private set

        override fun setSpeed(speed: Int) {
            displayedSpeed = speed
        }

        override fun setSpeedLimit(limit: Int?) {
            displayedSpeedLimit = limit
        }

        override fun setUnits(units: String) {
            displayedUnits = units
        }

        override fun setShape(shape: Shape) {
            displayedShape = shape
        }
    }

    private class FakeBridge : FreeRideSpeedController.NativeBridge {
        var enabled: Boolean = true
        var navigatingOrPlanning: Boolean = false
        var speedLimitMps: Double = -1.0
        var signType: Int = Framework.SpeedLimitSignType.VIENNA
        var units: String = "km/h"
        var imperial: Boolean = false

        var speedLimitQueryCount: Int = 0
        var lastQueryHasBearing: Boolean = false

        override fun isEnabled(): Boolean = enabled

        override fun isNavigatingOrPlanning(): Boolean = navigatingOrPlanning

        override fun getSpeedLimitMps(lat: Double, lon: Double, bearing: Double, hasBearing: Boolean): Double {
            speedLimitQueryCount++
            lastQueryHasBearing = hasBearing
            return speedLimitMps
        }

        override fun getSpeedLimitSignType(lat: Double, lon: Double): Int = signType

        override fun formatSpeed(metersPerSecond: Double): Int {
            val kmph = metersPerSecond * 3.6
            return if (imperial) (kmph / 1.609344).roundToInt() else kmph.roundToInt()
        }

        override fun getLocalizedSpeedUnits(): String = units
    }

    private class ImmediateScheduler : FreeRideSpeedController.Scheduler {
        var scheduledStaleRunnable: Runnable? = null

        override fun runBackground(task: Runnable) {
            task.run()
        }

        override fun runMain(task: Runnable) {
            task.run()
        }

        override fun postDelayedMain(task: Runnable, delayMs: Long) {
            scheduledStaleRunnable = task
        }

        override fun cancelMain(task: Runnable) {
            if (scheduledStaleRunnable === task) {
                scheduledStaleRunnable = null
            }
        }

        override fun shutdown() = Unit
    }

    private class Harness {
        val display = FakeDisplay()
        val bridge = FakeBridge()
        val scheduler = ImmediateScheduler()
        var visible: Boolean = false
        var nowMs: Long = 10_000L

        val controller = FreeRideSpeedController(
            setVisible = { visible = it },
            speedDisplay = display,
            bridge = bridge,
            clockMs = { nowMs },
            scheduler = scheduler,
        )
    }

    private fun movingFix(speedMps: Float = 15f, hasSpeed: Boolean = true, hasBearing: Boolean = true) =
        FreeRideSpeedController.Fix(
            lat = 44.8,
            lon = 20.4,
            speedMps = speedMps,
            hasSpeed = hasSpeed,
            bearingDeg = 90f,
            hasBearing = hasBearing,
        )

    @Test
    fun hiddenWhenDisabledByPreference() {
        val h = Harness()
        h.bridge.enabled = false
        h.bridge.speedLimitMps = 50.0 / 3.6
        h.controller.onStart()

        h.controller.onFixReceived(movingFix())

        assertFalse(h.visible)
        assertEquals(0, h.bridge.speedLimitQueryCount)
    }

    @Test
    fun visibleWhenStationaryWithFreshFix() {
        val h = Harness()
        h.bridge.speedLimitMps = 50.0 / 3.6
        h.controller.onStart()

        h.controller.onFixReceived(movingFix(speedMps = 0f, hasSpeed = false, hasBearing = true))

        assertTrue(h.visible)
        assertEquals(0, h.display.displayedSpeed)
        assertEquals(50, h.display.displayedSpeedLimit)
        assertFalse(h.bridge.lastQueryHasBearing)
    }

    @Test
    fun bearingIgnoredBelowMinimumSpeed() {
        val h = Harness()
        h.bridge.speedLimitMps = 60.0 / 3.6
        h.controller.onStart()

        h.controller.onFixReceived(movingFix(speedMps = 1.0f, hasSpeed = true, hasBearing = true))
        assertFalse(h.bridge.lastQueryHasBearing)

        h.nowMs += FreeRideSpeedController.QUERY_INTERVAL_MS
        h.controller.onFixReceived(movingFix(speedMps = 5.0f, hasSpeed = true, hasBearing = true))
        assertTrue(h.bridge.lastQueryHasBearing)
    }

    @Test
    fun queriesAreThrottledToInterval() {
        val h = Harness()
        h.bridge.speedLimitMps = 80.0 / 3.6
        h.controller.onStart()

        h.controller.onFixReceived(movingFix(speedMps = 20f))
        assertEquals(1, h.bridge.speedLimitQueryCount)

        h.nowMs += 500L
        h.controller.onFixReceived(movingFix(speedMps = 22f))
        assertEquals(1, h.bridge.speedLimitQueryCount)
        assertEquals(79, h.display.displayedSpeed)

        h.nowMs += 500L
        h.controller.onFixReceived(movingFix(speedMps = 25f))
        assertEquals(2, h.bridge.speedLimitQueryCount)
    }

    @Test
    fun unlimitedZeroLimitClearsImmediatelyAndNeverShowsZero() {
        val h = Harness()
        h.bridge.speedLimitMps = 120.0 / 3.6
        h.controller.onStart()

        h.controller.onFixReceived(movingFix(speedMps = 30f))
        assertEquals(120, h.display.displayedSpeedLimit)

        h.bridge.speedLimitMps = 0.0
        h.nowMs += FreeRideSpeedController.QUERY_INTERVAL_MS
        h.controller.onFixReceived(movingFix(speedMps = 35f))

        assertTrue(h.visible)
        assertNull(h.display.displayedSpeedLimit)
    }

    @Test
    fun negativeLimitUsesAntiFlickerGraceBeforeClearing() {
        val h = Harness()
        h.bridge.speedLimitMps = 50.0 / 3.6
        h.controller.onStart()

        h.controller.onFixReceived(movingFix())
        assertEquals(50, h.display.displayedSpeedLimit)

        // First invalid sample -> kept to avoid flicker.
        h.bridge.speedLimitMps = -1.0
        h.nowMs += FreeRideSpeedController.QUERY_INTERVAL_MS
        h.controller.onFixReceived(movingFix())
        assertEquals(50, h.display.displayedSpeedLimit)

        // Second invalid sample still inside grace window -> kept.
        h.nowMs += FreeRideSpeedController.QUERY_INTERVAL_MS
        h.controller.onFixReceived(movingFix())
        assertEquals(50, h.display.displayedSpeedLimit)

        // Sustained invalid beyond grace window -> cleared cleanly.
        h.nowMs += FreeRideSpeedController.QUERY_INTERVAL_MS
        h.controller.onFixReceived(movingFix())
        assertNull(h.display.displayedSpeedLimit)
    }

    @Test
    fun hiddenDuringOverlaysAndNavigation() {
        val h = Harness()
        h.bridge.speedLimitMps = 50.0 / 3.6
        h.controller.onStart()
        h.controller.onFixReceived(movingFix())
        assertTrue(h.visible)

        h.controller.setSearchActive(true)
        assertFalse(h.visible)
        h.controller.setSearchActive(false)
        assertTrue(h.visible)

        h.controller.setPlacePageActive(true)
        assertFalse(h.visible)
        h.controller.setPlacePageActive(false)
        assertTrue(h.visible)

        h.controller.setLayoutMode(MapButtonsController.LayoutMode.planning)
        assertFalse(h.visible)
        h.controller.setLayoutMode(MapButtonsController.LayoutMode.navigation)
        assertFalse(h.visible)
        h.controller.setLayoutMode(MapButtonsController.LayoutMode.regular)
        assertTrue(h.visible)

        h.bridge.navigatingOrPlanning = true
        h.controller.refresh()
        assertFalse(h.visible)
    }

    @Test
    fun refreshReformatsWhenUnitsChangeAndAppliesMutcdShape() {
        val h = Harness()
        h.bridge.signType = Framework.SpeedLimitSignType.MUTCD
        h.bridge.speedLimitMps = 65.0 * 1.609344 / 3.6
        h.controller.onStart()

        h.controller.onFixReceived(movingFix(speedMps = 20f))
        assertEquals(Shape.Square, h.display.displayedShape)
        assertEquals("km/h", h.display.displayedUnits)

        h.bridge.imperial = true
        h.bridge.units = "mph"
        h.controller.refresh()

        assertEquals("mph", h.display.displayedUnits)
        assertEquals(65, h.display.displayedSpeedLimit)
    }

    @Test
    fun staleFixHidesHud() {
        val h = Harness()
        h.bridge.speedLimitMps = 50.0 / 3.6
        h.controller.onStart()

        h.controller.onFixReceived(movingFix())
        assertTrue(h.visible)

        h.nowMs += FreeRideSpeedController.LOCATION_STALE_TIMEOUT_MS + 1L
        h.scheduler.scheduledStaleRunnable?.run()
        assertFalse(h.visible)
    }

    @Test
    fun refreshRestoresVisibilityWhenNavigationEndsWithFreshFix() {
        val h = Harness()
        h.bridge.speedLimitMps = 50.0 / 3.6
        h.controller.onStart()
        h.controller.onFixReceived(movingFix())
        assertTrue(h.visible)

        h.bridge.navigatingOrPlanning = true
        h.controller.refresh()
        assertFalse(h.visible)

        h.bridge.navigatingOrPlanning = false
        h.controller.refresh()
        assertTrue(h.visible)
        assertEquals(50, h.display.displayedSpeedLimit)
    }

    @Test
    fun staleStateAfterLongStopDoesNotRetainOldSpeedLimitInGracePeriod() {
        val h = Harness()
        h.bridge.speedLimitMps = 80.0 / 3.6
        h.controller.onStart()
        h.controller.onFixReceived(movingFix())
        assertTrue(h.visible)
        assertEquals(80, h.display.displayedSpeedLimit)

        h.controller.onStop()
        h.nowMs += FreeRideSpeedController.LOCATION_STALE_TIMEOUT_MS + 5_000L
        h.controller.onStart()
        assertFalse(h.visible)
        assertNull(h.display.displayedSpeedLimit)

        h.bridge.speedLimitMps = -1.0
        h.controller.onFixReceived(movingFix())
        assertTrue(h.visible)
        assertNull(h.display.displayedSpeedLimit)
    }

    @Test
    fun togglingDrivingModeWithRefreshShowsAndHidesImmediately() {
        val h = Harness()
        h.bridge.enabled = false
        h.bridge.speedLimitMps = 50.0 / 3.6
        h.controller.onStart()
        h.controller.onFixReceived(movingFix(speedMps = 15f))
        assertFalse(h.visible)
        assertEquals(0, h.bridge.speedLimitQueryCount)

        h.bridge.enabled = true
        h.controller.refresh()
        assertTrue(h.visible)
        assertEquals(54, h.display.displayedSpeed)
        assertEquals(50, h.display.displayedSpeedLimit)
        assertEquals(1, h.bridge.speedLimitQueryCount)

        h.bridge.enabled = false
        h.controller.refresh()
        assertFalse(h.visible)
    }
}

