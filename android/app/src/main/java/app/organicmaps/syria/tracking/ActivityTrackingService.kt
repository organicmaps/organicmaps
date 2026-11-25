package app.organicmaps.syria.tracking

import android.app.PendingIntent
import android.app.Service
import android.content.Context
import android.content.Intent
import android.content.pm.ServiceInfo
import android.location.Location
import android.os.BatteryManager
import android.os.Build
import android.os.IBinder
import android.text.format.DateUtils
import androidx.core.app.NotificationChannelCompat
import androidx.core.app.NotificationCompat
import androidx.core.app.NotificationManagerCompat
import androidx.core.app.ServiceCompat
import androidx.core.content.ContextCompat
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import app.organicmaps.MwmApplication
import app.organicmaps.R
import app.organicmaps.sdk.location.LocationHelper
import app.organicmaps.sdk.location.LocationListener
import app.organicmaps.sdk.location.LocationUtils
import app.organicmaps.sdk.location.TrackRecorder
import app.organicmaps.sdk.util.log.Logger
import java.util.UUID
import org.json.JSONObject

/**
 * Foreground service that records an activity: one point every 5 s into the local outbox, with
 * pause/resume, and the Organic Maps track recorder mirrored so the route is drawn on the map.
 */
class ActivityTrackingService : Service(), LocationListener {
    private val engine = TrackingEngine()
    private lateinit var db: TrackingDb
    private var backgroundMode: BackgroundMode? = null
    private var backgroundModeCheckedAt = 0L

    override fun onBind(intent: Intent?): IBinder? = null

    override fun onCreate() {
        super.onCreate()
        db = TrackingDb.get(this)
        createNotificationChannel(this)
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        if (!LocationUtils.checkFineLocationPermission(this)) {
            Logger.w(TAG, "Location permission is missing, not tracking")
            stopSelf()
            return START_NOT_STICKY
        }
        try {
            val type =
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) ServiceInfo.FOREGROUND_SERVICE_TYPE_LOCATION else 0
            ServiceCompat.startForeground(this, NOTIFICATION_ID, buildNotification(), type)
        } catch (e: IllegalStateException) {
            Logger.e(TAG, "The system refused the location foreground service", e)
            stopSelf()
            return START_NOT_STICKY
        } catch (e: SecurityException) {
            Logger.e(TAG, "The system refused the location foreground service", e)
            stopSelf()
            return START_NOT_STICKY
        }

        when (intent?.action) {
            ACTION_START -> startSession()
            ACTION_PAUSE -> pauseSession()
            ACTION_RESUME -> resumeSession()
            ACTION_STOP -> {
                stopSession()
                stopSelf()
                return START_NOT_STICKY
            }
            // Null intent: Android restarted the sticky service after the process died.
            else -> restoreSession()
        }

        if (engine.status == TrackingEngine.Status.IDLE) {
            stopSelf()
            return START_NOT_STICKY
        }
        updateLocationMode()
        publish()
        NotificationManagerCompat.from(this).notify(NOTIFICATION_ID, buildNotification())
        return START_STICKY
    }

    override fun onDestroy() {
        // Live location keeps the 5-second interval for itself.
        if (!LiveLocationService.isRunning()) LocationHelper.setExternalTrackingInterval(0)
        val app = MwmApplication.from(this)
        app.locationHelper.removeListener(this)
        if (app.organicMaps.arePlatformAndCoreInitialized()) app.onNavigationOrRecordingStopped()
        state.postValue(if (engine.status == TrackingEngine.Status.IDLE) null else engine.snapshot(now()))
        super.onDestroy()
    }

    // ---- session transitions

    private fun startSession() {
        if (engine.status != TrackingEngine.Status.IDLE) return
        val now = now()
        val sessionId = UUID.randomUUID().toString()
        val segmentId = UUID.randomUUID().toString()
        engine.start(sessionId, segmentId, now)
        db.insertSession(Session(sessionId, TrackingDb.STATUS_ACTIVE, now, null, null, 0.0, 0, 0, false, now))
        db.insertSegment(Segment(segmentId, sessionId, 0, now, null))
        backgroundMode = BackgroundMode.current(this)
        logEvent("tracking_start", BackgroundMode.payload(this, backgroundMode!!))
        mirrorMapLine(true)
        SyncWorker.schedulePeriodic(this)
        DrivingReminder.cancel(this)
        Logger.i(TAG, "Session $sessionId started")
    }

    private fun pauseSession() {
        if (engine.status != TrackingEngine.Status.ACTIVE) return
        val now = now()
        val sessionId = engine.sessionId ?: return
        engine.segmentId?.let { db.closeSegment(it, now) }
        engine.pause(now)
        db.updateSessionStatus(sessionId, TrackingDb.STATUS_PAUSED, null)
        persistProgress()
        logEvent("tracking_pause", null)
        mirrorMapLine(false)
        SyncWorker.enqueueNow(this)
    }

    private fun resumeSession() {
        if (engine.status != TrackingEngine.Status.PAUSED) return
        val now = now()
        val sessionId = engine.sessionId ?: return
        val segmentId = UUID.randomUUID().toString()
        val seq = db.segmentsOf(sessionId).size
        engine.resume(segmentId, now)
        db.insertSegment(Segment(segmentId, sessionId, seq, now, null))
        db.updateSessionStatus(sessionId, TrackingDb.STATUS_ACTIVE, null)
        logEvent("tracking_resume", null)
        checkBackgroundMode(force = true)
        mirrorMapLine(true)
    }

    private fun stopSession() {
        if (engine.status == TrackingEngine.Status.IDLE) return
        val now = now()
        val sessionId = engine.sessionId ?: return
        engine.segmentId?.let { db.closeSegment(it, now) }
        val result = engine.stop(now)
        db.updateSessionStatus(sessionId, TrackingDb.STATUS_STOPPED, now)
        db.updateSessionProgress(sessionId, result.distanceM, result.movingTimeS, result.pointCount)
        val payload = JSONObject()
            .put("distanceM", result.distanceM)
            .put("movingTimeS", result.movingTimeS)
            .put("points", result.pointCount)
            .put("batteryMode", BackgroundMode.current(this).key)
        db.insertEvent(TrackEvent(UUID.randomUUID().toString(), "tracking_stop", now, sessionId, payload.toString()))
        saveMapLine()
        SyncWorker.enqueueNow(this)
        Logger.i(TAG, "Session $sessionId stopped: ${result.distanceM} m, ${result.movingTimeS} s")
    }

    private fun restoreSession() {
        if (engine.status != TrackingEngine.Status.IDLE) return
        val open = db.getOpenSession() ?: return
        val segments = db.segmentsOf(open.id)
        val last = segments.lastOrNull()
        val activeSegment = if (open.status == TrackingDb.STATUS_ACTIVE && last?.endedAt == null) last else null
        val completedMovingMs = segments.filter { it.endedAt != null }.sumOf { it.endedAt!! - it.startedAt }
        val lastFix = activeSegment?.let { db.lastPoint(it.id) }?.toFix()
        engine.restore(
            TrackingEngine.SavedState(
                sessionId = open.id,
                segmentId = activeSegment?.id,
                status = if (activeSegment != null) TrackingEngine.Status.ACTIVE else TrackingEngine.Status.PAUSED,
                startedAtMs = open.startedAt,
                distanceM = open.distanceM,
                completedMovingMs = completedMovingMs,
                pointCount = open.pointCount,
                segmentStartMs = activeSegment?.startedAt ?: 0L,
                lastFix = lastFix,
            ),
        )
        if (activeSegment != null) mirrorMapLine(true)
        // Android killed the process and restarted the service: a likely gap in the route.
        backgroundMode = BackgroundMode.current(this)
        logEvent("tracking_restored", BackgroundMode.payload(this, backgroundMode!!).put("status", engine.status.name))
        Logger.i(TAG, "Session ${open.id} restored as ${engine.status}")
    }

    // ---- location

    override fun onLocationUpdated(location: Location) {
        val fix = TrackingEngine.Fix(
            lat = location.latitude,
            lon = location.longitude,
            alt = if (location.hasAltitude()) location.altitude else null,
            accuracyM = if (location.hasAccuracy()) location.accuracy else null,
            speedMps = if (location.hasSpeed()) location.speed else null,
            bearingDeg = if (location.hasBearing()) location.bearing else null,
            timeMs = if (location.time > 0) location.time else now(),
            provider = location.provider,
            isMock = isMock(location),
        )
        val sessionId = engine.sessionId ?: return
        val segmentId = engine.segmentId ?: return
        when (engine.offer(fix)) {
            is TrackingEngine.Decision.Stored -> {
                db.insertPoint(
                    TrackPoint(
                        UUID.randomUUID().toString(), sessionId, segmentId, fix.timeMs, fix.lat, fix.lon, fix.alt,
                        fix.accuracyM, fix.speedMps, fix.bearingDeg, fix.provider, fix.isMock, batteryPercent(),
                    ),
                )
                persistProgress()
                checkBackgroundMode(force = false)
                publish()
                NotificationManagerCompat.from(this).notify(NOTIFICATION_ID, buildNotification())
            }
            is TrackingEngine.Decision.Skipped -> Unit
        }
    }

    private fun updateLocationMode() {
        val active = engine.status == TrackingEngine.Status.ACTIVE
        LocationHelper.setExternalTrackingInterval(if (active || LiveLocationService.isRunning()) INTERVAL_MS else 0)
        val helper = MwmApplication.from(this).locationHelper
        if (active) helper.addListener(this) else helper.removeListener(this)
        if (LocationUtils.checkLocationPermission(this)) helper.restartWithNewMode()
    }

    // ---- helpers

    private fun persistProgress() {
        val sessionId = engine.sessionId ?: return
        db.updateSessionProgress(sessionId, engine.distanceM, engine.movingTimeMs(now()) / 1000, engine.pointCount)
    }

    private fun publish() {
        state.postValue(engine.snapshot(now()))
    }

    /** Logs a change of the battery setting during the recording; checked at most once a minute. */
    private fun checkBackgroundMode(force: Boolean) {
        val now = now()
        if (!force && now - backgroundModeCheckedAt < MODE_CHECK_MS) return
        backgroundModeCheckedAt = now
        val mode = BackgroundMode.current(this)
        val previous = backgroundMode
        backgroundMode = mode
        if (previous == null || previous == mode) return
        logEvent("battery_mode_changed", BackgroundMode.payload(this, mode).put("previous", previous.key))
        Logger.i(TAG, "Battery mode changed from $previous to $mode")
    }

    private fun logEvent(type: String, payload: JSONObject?) {
        db.insertEvent(TrackEvent(UUID.randomUUID().toString(), type, now(), engine.sessionId, payload?.toString()))
    }

    /** Organic Maps' own recorder only draws the live line; our outbox is the record of truth. */
    private fun mirrorMapLine(on: Boolean) {
        if (!MwmApplication.from(this).organicMaps.arePlatformAndCoreInitialized()) return
        val enabled = TrackRecorder.nativeIsTrackRecordingEnabled()
        if (on && !enabled) TrackRecorder.nativeStartTrackRecording()
        if (!on && enabled) TrackRecorder.nativeStopTrackRecording()
    }

    /** Keeps the route as a bookmark track so the user can look at it later in Organic Maps. */
    private fun saveMapLine() {
        if (!MwmApplication.from(this).organicMaps.arePlatformAndCoreInitialized()) return
        if (!TrackRecorder.saveAndStop()) TrackRecorder.nativeDiscardTrackRecording()
    }

    private fun batteryPercent(): Int? {
        val manager = getSystemService(Context.BATTERY_SERVICE) as? BatteryManager ?: return null
        val level = manager.getIntProperty(BatteryManager.BATTERY_PROPERTY_CAPACITY)
        return if (level in 1..100) level else null
    }

    private fun buildNotification(): android.app.Notification {
        val snapshot = engine.snapshot(now())
        val statusText = getString(
            when (snapshot.status) {
                TrackingEngine.Status.ACTIVE -> R.string.syria_status_active
                TrackingEngine.Status.PAUSED -> R.string.syria_status_paused
                TrackingEngine.Status.IDLE -> R.string.syria_status_idle
            },
        )
        val km = formatKm(this, snapshot.distanceM)
        val text = "$statusText · $km · ${DateUtils.formatElapsedTime(snapshot.movingTimeS)}"
        val builder = NotificationCompat.Builder(this, CHANNEL_ID)
            .setCategory(NotificationCompat.CATEGORY_SERVICE)
            .setPriority(NotificationCompat.PRIORITY_LOW)
            .setVisibility(NotificationCompat.VISIBILITY_PUBLIC)
            .setOngoing(true)
            .setOnlyAlertOnce(true)
            .setShowWhen(false)
            .setSmallIcon(app.organicmaps.branding.R.drawable.ic_splash)
            .setColor(ContextCompat.getColor(this, R.color.notification))
            .setContentTitle(getString(R.string.syria_tracking_title))
            .setContentText(text)
            .setContentIntent(
                PendingIntent.getActivity(
                    this,
                    0,
                    Intent(this, TrackingActivity::class.java),
                    PendingIntent.FLAG_UPDATE_CURRENT or immutableFlag(),
                ),
            )
        when (snapshot.status) {
            TrackingEngine.Status.ACTIVE ->
                builder.addAction(0, getString(R.string.syria_pause), actionIntent(ACTION_PAUSE, 1))
            TrackingEngine.Status.PAUSED ->
                builder.addAction(0, getString(R.string.syria_resume), actionIntent(ACTION_RESUME, 2))
            TrackingEngine.Status.IDLE -> Unit
        }
        builder.addAction(0, getString(R.string.syria_stop), actionIntent(ACTION_STOP, 3))
        return builder.build()
    }

    private fun actionIntent(action: String, requestCode: Int): PendingIntent {
        val intent = Intent(this, ActivityTrackingService::class.java).setAction(action)
        return PendingIntent.getService(this, requestCode, intent, PendingIntent.FLAG_UPDATE_CURRENT or immutableFlag())
    }

    private fun immutableFlag(): Int =
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) PendingIntent.FLAG_IMMUTABLE else 0

    private fun now(): Long = System.currentTimeMillis()

    private fun TrackPoint.toFix() =
        TrackingEngine.Fix(lat, lon, alt, accuracyM, speedMps, bearingDeg, recordedAt, provider, isMock)

    @Suppress("DEPRECATION")
    private fun isMock(location: Location): Boolean =
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) location.isMock else location.isFromMockProvider

    companion object {
        private const val TAG = "ActivityTracking"
        const val CHANNEL_ID = "SYRIA_ACTIVITY_TRACKING"
        const val NOTIFICATION_ID = 54322
        const val ACTION_START = "app.organicmaps.syria.tracking.START"
        const val ACTION_PAUSE = "app.organicmaps.syria.tracking.PAUSE"
        const val ACTION_RESUME = "app.organicmaps.syria.tracking.RESUME"
        const val ACTION_STOP = "app.organicmaps.syria.tracking.STOP"
        const val ACTION_RESTORE = "app.organicmaps.syria.tracking.RESTORE"

        /** Requested refresh interval while a session is active. */
        const val INTERVAL_MS = 5000L
        private const val MODE_CHECK_MS = 60_000L

        private val state = MutableLiveData<TrackingEngine.Snapshot?>()

        /** Null while no session is active or paused. */
        @JvmStatic
        fun state(): LiveData<TrackingEngine.Snapshot?> = state

        /** A recording is running (not paused). */
        @JvmStatic
        fun isRecording(): Boolean = state.value?.status == TrackingEngine.Status.ACTIVE

        @JvmStatic
        fun isActive(): Boolean {
            val snapshot = state.value ?: return false
            return snapshot.status != TrackingEngine.Status.IDLE
        }

        @JvmStatic fun start(context: Context) = send(context, ACTION_START)

        @JvmStatic fun pause(context: Context) = send(context, ACTION_PAUSE)

        @JvmStatic fun resume(context: Context) = send(context, ACTION_RESUME)

        @JvmStatic fun stop(context: Context) = send(context, ACTION_STOP)

        /** Brings a session that was open when the process died back into the service. */
        @JvmStatic
        fun resumeIfNeeded(context: Context) {
            if (isActive() || !LocationUtils.checkFineLocationPermission(context)) return
            if (TrackingDb.get(context).getOpenSession() != null) send(context, ACTION_RESTORE)
        }

        private fun send(context: Context, action: String) {
            val intent = Intent(context, ActivityTrackingService::class.java).setAction(action)
            ContextCompat.startForegroundService(context, intent)
        }

        @JvmStatic
        fun createNotificationChannel(context: Context) {
            val channel = NotificationChannelCompat.Builder(CHANNEL_ID, NotificationManagerCompat.IMPORTANCE_LOW)
                .setName(context.getString(R.string.syria_tracking_title))
                .setLightsEnabled(false)
                .setVibrationEnabled(false)
                .build()
            NotificationManagerCompat.from(context).createNotificationChannel(channel)
        }

        @JvmStatic
        fun formatKm(context: Context, meters: Double): String =
            context.getString(R.string.syria_km_format, meters / 1000.0)
    }
}
