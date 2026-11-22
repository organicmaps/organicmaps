package app.organicmaps.maplayer

import android.location.Location
import android.os.Handler
import android.os.Looper
import android.os.SystemClock
import androidx.annotation.VisibleForTesting
import app.organicmaps.sdk.Framework
import app.organicmaps.sdk.location.LocationListener
import app.organicmaps.sdk.routing.RoutingController
import app.organicmaps.sdk.util.Config
import app.organicmaps.sdk.util.StringUtils
import app.organicmaps.sdk.widgets.speedlimit.CombinedSpeedView
import app.organicmaps.sdk.widgets.speedlimit.CombinedSpeedViewController
import app.organicmaps.sdk.widgets.speedlimit.Shape
import app.organicmaps.util.UiUtils
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors
import java.util.concurrent.RejectedExecutionException

/**
 * Displays current speed and offline road speed limit on the map in free-ride mode
 * (when navigation, route planning, search, place page, and fullscreen overlays are inactive).
 */
class FreeRideSpeedController internal constructor(
    private val setVisible: (Boolean) -> Unit,
    private val speedDisplay: SpeedDisplay,
    private val bridge: NativeBridge,
    private val clockMs: () -> Long,
    private val scheduler: Scheduler,
) : LocationListener {
    internal interface SpeedDisplay {
        fun setSpeed(speed: Int)
        fun setSpeedLimit(limit: Int?)
        fun setUnits(units: String)
        fun setShape(shape: Shape)
    }

    internal interface NativeBridge {
        fun isEnabled(): Boolean
        fun isNavigatingOrPlanning(): Boolean
        fun getSpeedLimitMps(lat: Double, lon: Double, bearing: Double, hasBearing: Boolean): Double
        fun getSpeedLimitSignType(lat: Double, lon: Double): Int
        fun formatSpeed(metersPerSecond: Double): Int
        fun getLocalizedSpeedUnits(): String
    }

    internal interface Scheduler {
        fun runBackground(task: Runnable)
        fun runMain(task: Runnable)
        fun postDelayedMain(task: Runnable, delayMs: Long)
        fun cancelMain(task: Runnable)
        fun shutdown()
    }

    internal data class Fix(
        val lat: Double,
        val lon: Double,
        val speedMps: Float,
        val hasSpeed: Boolean,
        val bearingDeg: Float,
        val hasBearing: Boolean,
    )

    private var enabled: Boolean = bridge.isEnabled()
    private var layoutMode: MapButtonsController.LayoutMode = MapButtonsController.LayoutMode.regular
    private var buttonsHidden: Boolean = false
    private var searchActive: Boolean = false
    private var placePageActive: Boolean = false
    private var placePageMapObjectPresent: Boolean = false
    private var pointChooserActive: Boolean = false

    private var active: Boolean = false
    private var destroyed: Boolean = false
    private var generation: Long = 0L
    private var queryInFlight: Boolean = false

    private var lastFix: Fix? = null
    private var lastFixReceivedElapsedMs: Long = 0L
    private var lastSpeedMps: Double = 0.0
    private var lastQueryElapsedMs: Long = 0L
    private var lastSignTypeElapsedMs: Long = 0L
    private var hasSignType: Boolean = false

    private var lastValidLimitMps: Double? = null
    private var pendingInvalidCount: Int = 0
    private var firstInvalidElapsedMs: Long = 0L

    private val staleLocationRunnable = Runnable {
        if (!hasFreshFix(clockMs())) {
            clearLocationState()
        }
    }

    init {
        setVisible(false)
    }

    fun onStart() {
        if (destroyed) {
            return
        }
        active = true
        enabled = bridge.isEnabled()
        val now = clockMs()
        if (lastFix != null) {
            if (!hasFreshFix(now)) {
                clearLocationState()
            } else {
                rescheduleStaleTimer(now)
            }
        }
        updateVisibilityAndRefresh()
    }

    fun refresh() {
        if (destroyed) {
            return
        }
        enabled = bridge.isEnabled()
        if (shouldShow(clockMs())) {
            setVisible(true)
            applyCurrentUnitsAndCachedValues()
            lastFix?.let { fix -> maybeScheduleSpeedLimitQuery(fix, force = true) }
        } else {
            setVisible(false)
        }
    }

    fun onStop() {
        active = false
        generation++
        queryInFlight = false
        scheduler.cancelMain(staleLocationRunnable)
    }

    fun destroy() {
        if (destroyed) {
            return
        }
        onStop()
        destroyed = true
        setVisible(false)
        scheduler.shutdown()
    }

    fun setLayoutMode(mode: MapButtonsController.LayoutMode?) {
        layoutMode = mode ?: MapButtonsController.LayoutMode.regular
        updateVisibilityAndRefresh()
    }

    fun setButtonsHidden(hidden: Boolean) {
        buttonsHidden = hidden
        updateVisibilityAndRefresh()
    }

    fun setSearchActive(activeSearch: Boolean) {
        searchActive = activeSearch
        updateVisibilityAndRefresh()
    }

    fun setPlacePageActive(activePlacePage: Boolean) {
        placePageActive = activePlacePage
        updateVisibilityAndRefresh()
    }

    fun setPlacePageMapObjectPresent(present: Boolean) {
        placePageMapObjectPresent = present
        updateVisibilityAndRefresh()
    }

    fun setPointChooserActive(activeChooser: Boolean) {
        pointChooserActive = activeChooser
        updateVisibilityAndRefresh()
    }

    override fun onLocationUpdated(location: Location) {
        val ageMs = computeLocationAgeMs(location)
        onFixReceived(
            fix = Fix(
                lat = location.latitude,
                lon = location.longitude,
                speedMps = if (location.hasSpeed()) location.speed else 0f,
                hasSpeed = location.hasSpeed(),
                bearingDeg = if (location.hasBearing()) location.bearing else 0f,
                hasBearing = location.hasBearing(),
            ),
            ageMs = ageMs,
        )
    }

    override fun onLocationUpdateTimeout() {
        clearLocationState()
    }

    override fun onLocationUnavailable() {
        clearLocationState()
    }

    override fun onLocationDisabled() {
        clearLocationState()
    }

    @VisibleForTesting
    internal fun onFixReceived(fix: Fix, ageMs: Long = 0L) {
        if (destroyed) {
            return
        }
        val now = clockMs()
        if (ageMs > LOCATION_STALE_TIMEOUT_MS) {
            clearLocationState()
            return
        }
        if (lastFix != null && !hasFreshFix(now)) {
            clearLocationState()
        }

        lastFix = fix
        lastFixReceivedElapsedMs = now - ageMs.coerceAtLeast(0L)
        lastSpeedMps = if (fix.hasSpeed && fix.speedMps >= 0f) fix.speedMps.toDouble() else 0.0

        rescheduleStaleTimer(now)

        if (!shouldShow(now)) {
            setVisible(false)
            return
        }

        setVisible(true)
        speedDisplay.setUnits(bridge.getLocalizedSpeedUnits())
        speedDisplay.setSpeed(bridge.formatSpeed(lastSpeedMps))
        maybeScheduleSpeedLimitQuery(fix, force = false)
    }

    private fun updateVisibilityAndRefresh() {
        if (destroyed) {
            return
        }
        val now = clockMs()
        val visible = shouldShow(now)
        setVisible(visible)
        if (visible) {
            applyCurrentUnitsAndCachedValues()
            lastFix?.let { fix -> maybeScheduleSpeedLimitQuery(fix, force = false) }
        }
    }

    private fun applyCurrentUnitsAndCachedValues() {
        speedDisplay.setUnits(bridge.getLocalizedSpeedUnits())
        speedDisplay.setSpeed(bridge.formatSpeed(lastSpeedMps))
        val cachedLimit = lastValidLimitMps
        speedDisplay.setSpeedLimit(cachedLimit?.let { formatPositiveLimit(it) })
    }

    private fun clearLocationState() {
        generation++
        queryInFlight = false
        lastFix = null
        lastFixReceivedElapsedMs = 0L
        lastSpeedMps = 0.0
        lastQueryElapsedMs = 0L
        resetSpeedLimitState()
        scheduler.cancelMain(staleLocationRunnable)
        setVisible(false)
    }

    private fun resetSpeedLimitState() {
        lastValidLimitMps = null
        pendingInvalidCount = 0
        firstInvalidElapsedMs = 0L
        speedDisplay.setSpeedLimit(null)
    }

    private fun shouldShow(now: Long): Boolean {
        if (!active || destroyed || !enabled) {
            return false
        }
        if (layoutMode != MapButtonsController.LayoutMode.regular || bridge.isNavigatingOrPlanning()) {
            return false
        }
        if (hasBlockingOverlay()) {
            return false
        }
        return hasFreshFix(now)
    }

    private fun hasBlockingOverlay(): Boolean {
        if (buttonsHidden || searchActive || pointChooserActive) {
            return true
        }
        return placePageActive || placePageMapObjectPresent
    }

    private fun hasFreshFix(now: Long): Boolean {
        if (lastFix == null) {
            return false
        }
        val elapsed = now - lastFixReceivedElapsedMs
        return elapsed in 0L..LOCATION_STALE_TIMEOUT_MS
    }

    private fun rescheduleStaleTimer(now: Long) {
        scheduler.cancelMain(staleLocationRunnable)
        val remainingMs = (LOCATION_STALE_TIMEOUT_MS - (now - lastFixReceivedElapsedMs)).coerceAtLeast(1L)
        scheduler.postDelayedMain(staleLocationRunnable, remainingMs)
    }

    private fun maybeScheduleSpeedLimitQuery(fix: Fix, force: Boolean) {
        if (queryInFlight || !active || destroyed) {
            return
        }
        val now = clockMs()
        if (!force && lastQueryElapsedMs > 0L && now - lastQueryElapsedMs < QUERY_INTERVAL_MS) {
            return
        }

        val queryGeneration = generation
        val needSignType = !hasSignType || (now - lastSignTypeElapsedMs >= SIGN_TYPE_REFRESH_MS)
        val effectiveHasBearing = fix.hasBearing && fix.hasSpeed && fix.speedMps >= MIN_BEARING_SPEED_MPS
        queryInFlight = true
        lastQueryElapsedMs = now

        scheduler.runBackground(
            Runnable {
                val limitMps = bridge.getSpeedLimitMps(
                    fix.lat,
                    fix.lon,
                    fix.bearingDeg.toDouble(),
                    effectiveHasBearing,
                )
                val signType = if (needSignType) bridge.getSpeedLimitSignType(fix.lat, fix.lon) else null
                scheduler.runMain(
                    Runnable {
                        onSpeedLimitResult(queryGeneration, limitMps, signType)
                    },
                )
            },
        )
    }

    private fun onSpeedLimitResult(queryGeneration: Long, limitMps: Double, signType: Int?) {
        if (destroyed || !active || queryGeneration != generation) {
            return
        }
        queryInFlight = false
        val now = clockMs()

        if (signType != null) {
            hasSignType = true
            lastSignTypeElapsedMs = now
            speedDisplay.setShape(
                if (signType == Framework.SpeedLimitSignType.MUTCD) Shape.Square else Shape.Circle,
            )
        }

        if (!shouldShow(now)) {
            setVisible(false)
            return
        }

        applySpeedLimitMps(limitMps, now)
    }

    private fun applySpeedLimitMps(limitMps: Double, now: Long) {
        if (limitMps > 0.0) {
            val formatted = formatPositiveLimit(limitMps)
            if (formatted != null) {
                lastValidLimitMps = limitMps
                pendingInvalidCount = 0
                firstInvalidElapsedMs = 0L
                speedDisplay.setSpeedLimit(formatted)
                return
            }
        }

        // Explicit unlimited ("none", limitMps == 0.0) clears immediately without displaying "0",
        // matching NavigationController behavior.
        if (limitMps == 0.0) {
            resetSpeedLimitState()
            return
        }

        // Unknown / off-road (< 0.0): hold last valid limit briefly to avoid flicker on short untagged nodes.
        if (lastValidLimitMps == null) {
            speedDisplay.setSpeedLimit(null)
            return
        }

        pendingInvalidCount++
        if (firstInvalidElapsedMs == 0L) {
            firstInvalidElapsedMs = now
        }
        val elapsedInvalid = now - firstInvalidElapsedMs
        if (pendingInvalidCount >= INVALID_GRACE_SAMPLES && elapsedInvalid >= INVALID_GRACE_MS) {
            resetSpeedLimitState()
        }
    }

    private fun formatPositiveLimit(limitMps: Double): Int? {
        val formatted = bridge.formatSpeed(limitMps)
        return if (formatted > 0) formatted else null
    }

    private class CombinedSpeedDisplay(view: CombinedSpeedView) : SpeedDisplay {
        private val controller = CombinedSpeedViewController(view)

        override fun setSpeed(speed: Int) {
            controller.speed = speed
        }

        override fun setSpeedLimit(limit: Int?) {
            controller.speedLimit = limit
        }

        override fun setUnits(units: String) {
            controller.units = units
        }

        override fun setShape(shape: Shape) {
            controller.shape = shape
        }
    }

    private class DefaultScheduler : Scheduler {
        private val mainHandler = Handler(Looper.getMainLooper())
        private val executor: ExecutorService = Executors.newSingleThreadExecutor { runnable ->
            Thread(runnable, "FreeRideSpeedLimit").apply { isDaemon = true }
        }

        override fun runBackground(task: Runnable) {
            if (!executor.isShutdown) {
                try {
                    executor.execute(task)
                } catch (_: RejectedExecutionException) {
                    // Controller is being torn down.
                }
            }
        }

        override fun runMain(task: Runnable) {
            mainHandler.post(task)
        }

        override fun postDelayedMain(task: Runnable, delayMs: Long) {
            mainHandler.postDelayed(task, delayMs)
        }

        override fun cancelMain(task: Runnable) {
            mainHandler.removeCallbacks(task)
        }

        override fun shutdown() {
            executor.shutdownNow()
        }
    }

    private object DefaultNativeBridge : NativeBridge {
        override fun isEnabled(): Boolean = Config.isFreeRideSpeedLimitEnabled()

        override fun isNavigatingOrPlanning(): Boolean {
            val routing = RoutingController.get()
            return routing.isNavigating || routing.isPlanning
        }

        override fun getSpeedLimitMps(lat: Double, lon: Double, bearing: Double, hasBearing: Boolean): Double =
            Framework.nativeGetSpeedLimitMps(lat, lon, bearing, hasBearing)

        override fun getSpeedLimitSignType(lat: Double, lon: Double): Int =
            Framework.nativeGetSpeedLimitSignType(lat, lon)

        override fun formatSpeed(metersPerSecond: Double): Int = StringUtils.nativeFormatSpeed(metersPerSecond)

        override fun getLocalizedSpeedUnits(): String = StringUtils.nativeGetLocalizedSpeedUnits()
    }

    companion object {
        internal const val QUERY_INTERVAL_MS = 1_000L
        internal const val SIGN_TYPE_REFRESH_MS = 30_000L
        internal const val INVALID_GRACE_MS = 2_000L
        internal const val INVALID_GRACE_SAMPLES = 2
        internal const val LOCATION_STALE_TIMEOUT_MS = 15_000L
        internal const val MIN_BEARING_SPEED_MPS = 1.5f

        @JvmStatic
        fun create(speedView: CombinedSpeedView): FreeRideSpeedController = FreeRideSpeedController(
            setVisible = { visible -> UiUtils.showIf(visible, speedView) },
            speedDisplay = CombinedSpeedDisplay(speedView),
            bridge = DefaultNativeBridge,
            clockMs = SystemClock::elapsedRealtime,
            scheduler = DefaultScheduler(),
        )

        private fun computeLocationAgeMs(location: Location): Long {
            val fixNanos = location.elapsedRealtimeNanos
            if (fixNanos <= 0L) {
                return 0L
            }
            val deltaMs = (SystemClock.elapsedRealtimeNanos() - fixNanos) / 1_000_000L
            return deltaMs.coerceAtLeast(0L)
        }
    }
}
