package app.organicmaps.syria.tracking

import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class SpeedWatchTest {
    private val t0 = 1_700_000_000_000L

    // 0.001 deg of latitude is ~111.2 m.
    private val metersPerDegree = 111_195.0

    /** Feeds one fix per second at [kmh] (as derived speed, no reported speed), returns the results. */
    private fun drive(watch: SpeedWatch, kmh: Double, seconds: Int, startS: Int = 0, startLat: Double = 36.2) =
        (0 until seconds).map { i ->
            val s = startS + i
            val lat = startLat + kmh / 3.6 * i / metersPerDegree
            watch.offer(t0 + s * 1000L, lat, 37.15, null, 5f)
        }

    @Test
    fun walkingNeverTriggers() {
        assertFalse(drive(SpeedWatch(), kmh = 5.0, seconds = 120).any { it })
    }

    @Test
    fun drivingTriggersOnlyAfterTwentySeconds() {
        val watch = SpeedWatch()
        val results = (0..30).map { watch.offer(t0 + it * 1000L, 36.2, 37.15, 40 / 3.6f, 5f) }
        assertFalse(results.take(20).any { it })
        assertTrue(results[20])
    }

    @Test
    fun derivedSpeedIsUsedWhenTheFixHasNone() {
        val results = drive(SpeedWatch(), kmh = 40.0, seconds = 30)
        // The first fix has no previous one; the fast stretch starts with the second.
        assertFalse(results.take(21).any { it })
        assertTrue(results[21])
    }

    @Test
    fun aSlowFixRestartsTheCount() {
        val watch = SpeedWatch()
        for (s in 0..15) assertFalse(watch.offer(t0 + s * 1000L, 36.2, 37.15, 12f, 5f))
        assertFalse(watch.offer(t0 + 16_000, 36.2, 37.15, 1f, 5f))
        for (s in 17..36) assertFalse(watch.offer(t0 + s * 1000L, 36.2, 37.15, 12f, 5f))
        assertTrue(watch.offer(t0 + 37_000, 36.2, 37.15, 12f, 5f))
    }

    @Test
    fun inaccurateFixesAreIgnored() {
        val watch = SpeedWatch()
        val results = (0..40).map { watch.offer(t0 + it * 1000L, 36.2, 37.15, 20f, 80f) }
        assertFalse(results.any { it })
    }

    @Test
    fun aLongGapRestartsTheCount() {
        val watch = SpeedWatch()
        for (s in 0..15) assertFalse(watch.offer(t0 + s * 1000L, 36.2, 37.15, 12f, 5f))
        // 60 s without fixes (app was in the background): the next fast fix starts over.
        assertFalse(watch.offer(t0 + 75_000, 36.2, 37.15, 12f, 5f))
        assertFalse(watch.offer(t0 + 90_000, 36.2, 37.15, 12f, 5f))
        assertTrue(watch.offer(t0 + 95_000, 36.2, 37.15, 12f, 5f))
    }

    @Test
    fun theThresholdIsTenKilometresPerHour() {
        val slow = SpeedWatch()
        assertFalse((0..40).map { slow.offer(t0 + it * 1000L, 36.2, 37.15, (9.5 / 3.6).toFloat(), 5f) }.any { it })
        val fast = SpeedWatch()
        assertTrue((0..40).map { fast.offer(t0 + it * 1000L, 36.2, 37.15, (10.5 / 3.6).toFloat(), 5f) }.any { it })
    }
}
