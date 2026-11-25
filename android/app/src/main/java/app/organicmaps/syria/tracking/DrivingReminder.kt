package app.organicmaps.syria.tracking

import android.annotation.SuppressLint
import android.app.PendingIntent
import android.content.Context
import android.content.Intent
import android.location.Location
import android.os.Build
import androidx.core.app.NotificationChannelCompat
import androidx.core.app.NotificationCompat
import androidx.core.app.NotificationManagerCompat
import androidx.core.content.ContextCompat
import app.organicmaps.MwmApplication
import app.organicmaps.R
import app.organicmaps.sdk.location.LocationListener
import app.organicmaps.sdk.util.log.Logger
import app.organicmaps.syria.SyriaPrefs

/**
 * "Are you driving?": a notification when the user moves at driving speed without recording. Uses only the
 * fixes the app already receives (map open, navigation, live location), so it costs no extra battery.
 * Tapping it opens the recording screen with the start and live location buttons highlighted.
 */
object DrivingReminder : LocationListener {
    private const val TAG = "SyriaDriving"
    private const val CHANNEL_ID = "SYRIA_DRIVING_REMINDER"
    private const val NOTIFICATION_ID = 54324
    private const val COOLDOWN_MS = 60 * 60 * 1000L

    private val watch = SpeedWatch()
    private lateinit var app: Context

    @JvmStatic
    fun init(application: MwmApplication) {
        if (::app.isInitialized) return
        app = application
        val channel = NotificationChannelCompat.Builder(CHANNEL_ID, NotificationManagerCompat.IMPORTANCE_HIGH)
            .setName(application.getString(R.string.syria_driving_channel))
            .build()
        NotificationManagerCompat.from(application).createNotificationChannel(channel)
        application.locationHelper.addListener(this)
    }

    override fun onLocationUpdated(location: Location) {
        if (ActivityTrackingService.isActive()) {
            watch.reset()
            return
        }
        val now = System.currentTimeMillis()
        if (now - SyriaPrefs.drivingReminderAt(app) < COOLDOWN_MS) return
        val time = if (location.time > 0) location.time else now
        val speed = if (location.hasSpeed()) location.speed else null
        val accuracy = if (location.hasAccuracy()) location.accuracy else null
        if (!watch.offer(time, location.latitude, location.longitude, speed, accuracy)) return
        watch.reset()
        // Not counted as shown while notifications are off: it can come once they are allowed.
        if (!NotificationManagerCompat.from(app).areNotificationsEnabled()) return
        SyriaPrefs.setDrivingReminderAt(app, now)
        show()
    }

    @SuppressLint("MissingPermission") // areNotificationsEnabled() was checked.
    private fun show() {
        Logger.i(TAG, "Driving without recording, reminding")
        val open = Intent(app, TrackingActivity::class.java)
            .putExtra(TrackingActivity.EXTRA_DRIVING_GUIDE, true)
            .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_CLEAR_TOP)
        val immutable = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) PendingIntent.FLAG_IMMUTABLE else 0
        val notification = NotificationCompat.Builder(app, CHANNEL_ID)
            .setSmallIcon(R.drawable.ic_track_recording_off)
            .setColor(ContextCompat.getColor(app, R.color.notification))
            .setContentTitle(app.getString(R.string.syria_driving_title))
            .setContentText(app.getString(R.string.syria_driving_text))
            .setStyle(NotificationCompat.BigTextStyle().bigText(app.getString(R.string.syria_driving_text)))
            .setPriority(NotificationCompat.PRIORITY_HIGH)
            .setCategory(NotificationCompat.CATEGORY_REMINDER)
            .setAutoCancel(true)
            .setContentIntent(
                PendingIntent.getActivity(app, 0, open, PendingIntent.FLAG_UPDATE_CURRENT or immutable),
            )
            .build()
        NotificationManagerCompat.from(app).notify(NOTIFICATION_ID, notification)
    }

    /** The reminder is no longer needed once a recording starts, however it was started. */
    @JvmStatic
    fun cancel(context: Context) {
        NotificationManagerCompat.from(context).cancel(NOTIFICATION_ID)
    }
}
