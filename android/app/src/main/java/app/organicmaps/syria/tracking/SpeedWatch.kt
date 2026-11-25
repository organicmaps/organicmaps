package app.organicmaps.syria.tracking

/**
 * Tells when the user has been moving at driving speed for a while, from the location fixes the app
 * already receives. A single fast fix (GPS jitter) is not enough: the speed must last [sustainMs].
 */
class SpeedWatch(
    private val minSpeedMps: Double = 10 / 3.6,
    private val sustainMs: Long = 20_000,
    private val maxGapMs: Long = 30_000,
    private val maxAccuracyM: Float = 50f,
) {
    private var lastTimeMs = 0L
    private var lastLat = 0.0
    private var lastLon = 0.0
    private var hasLast = false
    private var fastSinceMs: Long? = null

    /** Returns true when the speed has stayed at or above the threshold for the whole sustain time. */
    fun offer(timeMs: Long, lat: Double, lon: Double, speedMps: Float?, accuracyM: Float?): Boolean {
        if (accuracyM != null && accuracyM > maxAccuracyM) return false
        val gap = timeMs - lastTimeMs
        if (hasLast && gap <= 0) return false
        if (hasLast && gap > maxGapMs) fastSinceMs = null
        // Mock locations and some phones report no speed: use the distance from the previous fix instead.
        val speed = speedMps?.toDouble()
            ?: if (hasLast && gap <= maxGapMs) {
                TrackingEngine.haversineM(lastLat, lastLon, lat, lon) / (gap / 1000.0)
            } else {
                null
            }
        hasLast = true
        lastTimeMs = timeMs
        lastLat = lat
        lastLon = lon
        if (speed == null) return false
        if (speed < minSpeedMps) {
            fastSinceMs = null
            return false
        }
        val since = fastSinceMs ?: timeMs.also { fastSinceMs = it }
        return timeMs - since >= sustainMs
    }

    fun reset() {
        hasLast = false
        fastSinceMs = null
    }
}
