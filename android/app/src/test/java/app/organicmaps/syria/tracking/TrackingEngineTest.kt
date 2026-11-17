package app.organicmaps.syria.tracking

import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

class TrackingEngineTest {
    private val t0 = 1_700_000_000_000L

    private fun fix(lat: Double, lon: Double, timeMs: Long, accuracy: Float = 5f) =
        TrackingEngine.Fix(lat, lon, null, accuracy, null, null, timeMs, "gps", false)

    private fun stored(decision: TrackingEngine.Decision): TrackingEngine.Decision.Stored {
        assertTrue("expected Stored but got $decision", decision is TrackingEngine.Decision.Stored)
        return decision as TrackingEngine.Decision.Stored
    }

    @Test
    fun haversineMatchesKnownDistance() {
        // 0.001 deg of latitude is ~111.2 m anywhere on the globe.
        assertEquals(111.2, TrackingEngine.haversineM(33.5, 36.3, 33.501, 36.3), 0.3)
    }

    @Test
    fun samplesEveryFiveSecondsAndSumsDistance() {
        val engine = TrackingEngine()
        engine.start("s", "g", t0)
        var total = 0.0
        // 100 ms updates while navigating: only one point per ~5 s is stored.
        for (i in 0 until 100) {
            val d = engine.offer(fix(33.5 + i * 0.00001, 36.3, t0 + i * 100))
            if (d is TrackingEngine.Decision.Stored) total += d.distanceDeltaM
        }
        assertEquals(3, engine.pointCount) // t = 0 s, 4.5 s, 9 s
        assertEquals(total, engine.distanceM, 1e-9)
        assertEquals(100.1, engine.distanceM, 0.5) // 0.0009 deg of latitude between the first and last stored point
    }

    @Test
    fun rejectsInaccurateFixesAndJumps() {
        val engine = TrackingEngine()
        engine.start("s", "g", t0)
        stored(engine.offer(fix(33.5, 36.3, t0)))
        assertTrue(engine.offer(fix(33.5001, 36.3, t0 + 5000, accuracy = 80f)) is TrackingEngine.Decision.Skipped)
        // 1 degree (~111 km) in 5 s is far above 70 m/s.
        assertTrue(engine.offer(fix(34.5, 36.3, t0 + 5000)) is TrackingEngine.Decision.Skipped)
        assertEquals(1, engine.pointCount)
        assertEquals(0.0, engine.distanceM, 0.0)
    }

    @Test
    fun pauseDoesNotCountDistanceOrTimeAndResumeStartsNewSegment() {
        val engine = TrackingEngine()
        engine.start("s", "g1", t0)
        stored(engine.offer(fix(33.5, 36.3, t0)))
        stored(engine.offer(fix(33.501, 36.3, t0 + 5000)))
        engine.pause(t0 + 10_000)
        assertEquals(TrackingEngine.Status.PAUSED, engine.status)
        assertEquals(10, engine.movingTimeMs(t0 + 60_000) / 1000)
        assertTrue(engine.offer(fix(33.6, 36.3, t0 + 30_000)) is TrackingEngine.Decision.Skipped)

        engine.resume("g2", t0 + 60_000)
        // First point after a resume carries no distance from the pre-pause position.
        assertEquals(0.0, stored(engine.offer(fix(33.6, 36.3, t0 + 60_000))).distanceDeltaM, 0.0)
        stored(engine.offer(fix(33.601, 36.3, t0 + 65_000)))
        val result = engine.stop(t0 + 70_000)
        assertEquals(20, result.movingTimeS)
        assertEquals(222.4, result.distanceM, 1.0)
        assertEquals(4, result.pointCount)
        assertEquals(TrackingEngine.Status.IDLE, engine.status)
    }

    @Test
    fun restoreContinuesCounting() {
        val engine = TrackingEngine()
        val last = fix(33.5, 36.3, t0 + 100_000)
        engine.restore(
            TrackingEngine.SavedState(
                "s", "g2", TrackingEngine.Status.ACTIVE, t0, 500.0, 60_000, 12, t0 + 90_000, last,
            ),
        )
        assertEquals(70, engine.movingTimeMs(t0 + 100_000) / 1000)
        assertTrue(engine.offer(fix(33.5, 36.3, t0 + 101_000)) is TrackingEngine.Decision.Skipped) // too soon
        val d = stored(engine.offer(fix(33.501, 36.3, t0 + 105_000)))
        assertEquals(111.2, d.distanceDeltaM, 0.3)
        assertEquals(13, engine.pointCount)
        assertEquals(611.2, engine.distanceM, 0.3)
    }
}
