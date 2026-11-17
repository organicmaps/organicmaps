package app.organicmaps.syria.tracking

import kotlin.math.atan2
import kotlin.math.cos
import kotlin.math.sin
import kotlin.math.sqrt

/**
 * Pure tracking logic: session state, 5 s sampling, outlier filtering and distance/moving time.
 * The service feeds it location fixes and persists what it accepts; nothing here touches Android.
 */
class TrackingEngine(private val config: Config = Config()) {
    data class Config(
        /** Minimum spacing between stored points; slightly under 5 s so 5 s provider updates are never skipped. */
        val sampleIntervalMs: Long = 4500,
        val maxAccuracyM: Float = 30f,
        val maxSpeedMps: Double = 70.0,
    )

    data class Fix(
        val lat: Double,
        val lon: Double,
        val alt: Double?,
        val accuracyM: Float?,
        val speedMps: Float?,
        val bearingDeg: Float?,
        val timeMs: Long,
        val provider: String?,
        val isMock: Boolean,
    )

    enum class Status { IDLE, ACTIVE, PAUSED }

    sealed class Decision {
        data class Stored(val distanceDeltaM: Double) : Decision()
        data class Skipped(val reason: String) : Decision()
    }

    data class Snapshot(
        val status: Status,
        val sessionId: String?,
        val segmentId: String?,
        val startedAtMs: Long,
        val distanceM: Double,
        val movingTimeS: Long,
        val pointCount: Int,
    )

    var status = Status.IDLE
        private set
    var sessionId: String? = null
        private set
    var segmentId: String? = null
        private set
    var startedAtMs = 0L
        private set
    var distanceM = 0.0
        private set
    var pointCount = 0
        private set

    private var completedMovingMs = 0L
    private var segmentStartMs = 0L
    private var lastFix: Fix? = null
    private var lastStoredMs: Long? = null

    fun start(sessionId: String, segmentId: String, nowMs: Long) {
        check(status == Status.IDLE) { "already tracking" }
        this.sessionId = sessionId
        this.segmentId = segmentId
        startedAtMs = nowMs
        segmentStartMs = nowMs
        distanceM = 0.0
        pointCount = 0
        completedMovingMs = 0L
        lastFix = null
        lastStoredMs = null
        status = Status.ACTIVE
    }

    fun pause(nowMs: Long) {
        check(status == Status.ACTIVE) { "not active" }
        completedMovingMs += (nowMs - segmentStartMs).coerceAtLeast(0)
        segmentId = null
        lastFix = null
        status = Status.PAUSED
    }

    /** Opens a new segment: distance is never counted across a pause. */
    fun resume(segmentId: String, nowMs: Long) {
        check(status == Status.PAUSED) { "not paused" }
        this.segmentId = segmentId
        segmentStartMs = nowMs
        lastStoredMs = null
        status = Status.ACTIVE
    }

    fun stop(nowMs: Long): Snapshot {
        check(status != Status.IDLE) { "not tracking" }
        if (status == Status.ACTIVE) completedMovingMs += (nowMs - segmentStartMs).coerceAtLeast(0)
        status = Status.IDLE
        val result =
            Snapshot(Status.IDLE, sessionId, segmentId, startedAtMs, distanceM, completedMovingMs / 1000, pointCount)
        segmentId = null
        lastFix = null
        return result
    }

    /** State of an open session as persisted by the service, used to continue after the process was killed. */
    data class SavedState(
        val sessionId: String,
        val segmentId: String?,
        val status: Status,
        val startedAtMs: Long,
        val distanceM: Double,
        val completedMovingMs: Long,
        val pointCount: Int,
        val segmentStartMs: Long,
        val lastFix: Fix?,
    )

    /** Restores an open session after the process was killed. */
    fun restore(state: SavedState) {
        require(state.status != Status.IDLE) { "nothing to restore" }
        sessionId = state.sessionId
        segmentId = state.segmentId
        status = state.status
        startedAtMs = state.startedAtMs
        distanceM = state.distanceM
        completedMovingMs = state.completedMovingMs
        pointCount = state.pointCount
        segmentStartMs = state.segmentStartMs
        lastFix = state.lastFix
        lastStoredMs = state.lastFix?.timeMs
    }

    fun offer(fix: Fix): Decision {
        if (status != Status.ACTIVE) return Decision.Skipped("not active")
        fix.accuracyM?.let { if (it > config.maxAccuracyM) return Decision.Skipped("accuracy ${it}m") }
        val previousStoredMs = lastStoredMs
        if (previousStoredMs != null && fix.timeMs - previousStoredMs < config.sampleIntervalMs) {
            return Decision.Skipped("sampling")
        }

        var delta = 0.0
        lastFix?.let { prev ->
            val dt = (fix.timeMs - prev.timeMs) / 1000.0
            if (dt <= 0) return Decision.Skipped("time went backwards")
            val d = haversineM(prev.lat, prev.lon, fix.lat, fix.lon)
            if (d / dt > config.maxSpeedMps) return Decision.Skipped("jump ${d.toInt()}m in ${dt}s")
            delta = d
        }
        distanceM += delta
        pointCount++
        lastFix = fix
        lastStoredMs = fix.timeMs
        return Decision.Stored(delta)
    }

    fun movingTimeMs(nowMs: Long): Long =
        completedMovingMs + if (status == Status.ACTIVE) (nowMs - segmentStartMs).coerceAtLeast(0) else 0L

    fun snapshot(nowMs: Long): Snapshot =
        Snapshot(status, sessionId, segmentId, startedAtMs, distanceM, movingTimeMs(nowMs) / 1000, pointCount)

    companion object {
        private const val EARTH_RADIUS_M = 6371008.8

        @JvmStatic
        fun haversineM(lat1: Double, lon1: Double, lat2: Double, lon2: Double): Double {
            val dLat = Math.toRadians(lat2 - lat1)
            val dLon = Math.toRadians(lon2 - lon1)
            val a = sin(dLat / 2) * sin(dLat / 2) +
                cos(Math.toRadians(lat1)) * cos(Math.toRadians(lat2)) * sin(dLon / 2) * sin(dLon / 2)
            return 2 * EARTH_RADIUS_M * atan2(sqrt(a), sqrt(1 - a))
        }
    }
}
