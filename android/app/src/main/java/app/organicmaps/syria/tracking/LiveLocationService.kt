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
import app.organicmaps.sdk.util.log.Logger
import app.organicmaps.syria.ApiClient
import app.organicmaps.syria.SyriaPrefs
import app.organicmaps.syria.SyriaStatus
import java.io.IOException
import java.util.UUID
import java.util.concurrent.Executors
import org.json.JSONObject

/**
 * Live location, separate from recordings and their offline outbox: while the user has it switched on, the
 * position goes to the server every 5 seconds, with or without a recording. It is sent only while online;
 * a position that cannot be sent right away is dropped, as it would no longer be live.
 */
class LiveLocationService : Service(), LocationListener {
    private val sender = Executors.newSingleThreadExecutor()
    private var lastSentAt = 0L

    override fun onBind(intent: Intent?): IBinder? = null

    override fun onCreate() {
        super.onCreate()
        createNotificationChannel(this)
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        if (intent?.action == ACTION_STOP || !SyriaPrefs.isLiveEnabled(this) || !SyriaPrefs.isLoggedIn(this) ||
            !LocationUtils.checkFineLocationPermission(this)
        ) {
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
        if (running.value != true) {
            running.value = true
            val helper = MwmApplication.from(this).locationHelper
            LocationHelper.setExternalTrackingInterval(INTERVAL_MS)
            helper.addListener(this)
            helper.restartWithNewMode()
            Logger.i(TAG, "Live location started")
        }
        return START_STICKY
    }

    override fun onDestroy() {
        if (running.value == true) {
            running.value = false
            val app = MwmApplication.from(this)
            app.locationHelper.removeListener(this)
            // A recording keeps the 5-second interval for itself.
            if (!ActivityTrackingService.isRecording()) LocationHelper.setExternalTrackingInterval(0)
            if (app.organicMaps.arePlatformAndCoreInitialized()) app.onNavigationOrRecordingStopped()
            Logger.i(TAG, "Live location stopped")
        }
        sender.shutdownNow()
        super.onDestroy()
    }

    override fun onLocationUpdated(location: Location) {
        val now = System.currentTimeMillis()
        if (now - lastSentAt < MIN_GAP_MS || !SyriaStatus.isOnline()) return
        lastSentAt = now
        val point = JSONObject()
            .put("id", UUID.randomUUID().toString())
            .put("recordedAt", SyncWorker.iso(if (location.time > 0) location.time else now))
            .put("lat", location.latitude)
            .put("lon", location.longitude)
            .put("alt", if (location.hasAltitude()) location.altitude else JSONObject.NULL)
            .put("accuracyM", if (location.hasAccuracy()) location.accuracy.toDouble() else JSONObject.NULL)
            .put("speedMps", if (location.hasSpeed()) location.speed.toDouble() else JSONObject.NULL)
            .put("bearingDeg", if (location.hasBearing()) location.bearing.toDouble() else JSONObject.NULL)
            .put("isMock", isMock(location))
            .put("batteryPct", batteryPercent() ?: JSONObject.NULL)
        val appContext = applicationContext
        sender.execute {
            try {
                ApiClient.sendLivePoints(appContext, listOf(point))
            } catch (e: IOException) {
                // Not queued: a late position is not live any more.
                Logger.w(TAG, "Live point not sent: ${e.message}")
            }
        }
    }

    private fun batteryPercent(): Int? {
        val manager = getSystemService(Context.BATTERY_SERVICE) as? BatteryManager ?: return null
        val level = manager.getIntProperty(BatteryManager.BATTERY_PROPERTY_CAPACITY)
        return if (level in 1..100) level else null
    }

    @Suppress("DEPRECATION")
    private fun isMock(location: Location): Boolean =
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) location.isMock else location.isFromMockProvider

    private fun buildNotification(): android.app.Notification {
        val open = packageManager.getLaunchIntentForPackage(packageName)?.let {
            PendingIntent.getActivity(this, 0, it, PendingIntent.FLAG_UPDATE_CURRENT or immutableFlag())
        }
        val stop = PendingIntent.getService(
            this,
            1,
            Intent(this, LiveLocationService::class.java).setAction(ACTION_STOP),
            PendingIntent.FLAG_UPDATE_CURRENT or immutableFlag(),
        )
        return NotificationCompat.Builder(this, CHANNEL_ID)
            .setSmallIcon(R.drawable.ic_syria_live)
            .setContentTitle(getString(R.string.syria_live_notification_title))
            .setContentText(getString(R.string.syria_live_notification_text))
            .setOngoing(true)
            .setOnlyAlertOnce(true)
            .setContentIntent(open)
            .addAction(0, getString(R.string.syria_stop), stop)
            .build()
    }

    private fun immutableFlag(): Int =
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) PendingIntent.FLAG_IMMUTABLE else 0

    companion object {
        private const val TAG = "SyriaLive"
        private const val CHANNEL_ID = "SYRIA_LIVE_LOCATION"
        private const val NOTIFICATION_ID = 54323
        private const val ACTION_STOP = "app.organicmaps.syria.live.STOP"
        const val INTERVAL_MS = 5000L

        // Location updates may come a little early; this keeps about one point per 5 seconds.
        private const val MIN_GAP_MS = 4500L

        private val running = MutableLiveData(false)

        @JvmStatic
        fun running(): LiveData<Boolean> = running

        @JvmStatic
        fun isRunning(): Boolean = running.value == true

        /** Switches live location on (remembered) and starts it; call from a visible screen. */
        @JvmStatic
        fun enable(context: Context) {
            SyriaPrefs.setLiveEnabled(context, true)
            startIfEnabled(context)
        }

        @JvmStatic
        fun disable(context: Context) {
            SyriaPrefs.setLiveEnabled(context, false)
            context.stopService(Intent(context, LiveLocationService::class.java))
        }

        /** Brings live location back after the app was closed; only from a visible screen (Android rules). */
        @JvmStatic
        fun startIfEnabled(context: Context) {
            if (isRunning() || !SyriaPrefs.isLiveEnabled(context) || !SyriaPrefs.isLoggedIn(context)) return
            if (!LocationUtils.checkFineLocationPermission(context)) return
            ContextCompat.startForegroundService(context, Intent(context, LiveLocationService::class.java))
        }

        @JvmStatic
        fun createNotificationChannel(context: Context) {
            val channel = NotificationChannelCompat.Builder(CHANNEL_ID, NotificationManagerCompat.IMPORTANCE_LOW)
                .setName(context.getString(R.string.syria_live_off))
                .setLightsEnabled(false)
                .setVibrationEnabled(false)
                .build()
            NotificationManagerCompat.from(context).createNotificationChannel(channel)
        }
    }
}
