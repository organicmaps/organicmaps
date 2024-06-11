package app.tourism

import android.app.Notification
import android.app.Service
import android.content.Intent
import android.os.IBinder
import androidx.core.app.NotificationCompat
import androidx.core.app.NotificationManagerCompat
import app.tourism.di.InProgressNotificationCompatBuilder
import app.tourism.di.NotInProgressNotificationCompatBuilder
import dagger.hilt.android.AndroidEntryPoint
import javax.inject.Inject

@AndroidEntryPoint
class DownloaderService : Service() {
    @Inject
    lateinit var st: String

    @Inject
    lateinit var notificationManager: NotificationManagerCompat

    @Inject
    @InProgressNotificationCompatBuilder
    lateinit var inProgressNotificationBuilder: NotificationCompat.Builder

    @Inject
    @NotInProgressNotificationCompatBuilder
    lateinit var notInProgressNotificationBuilder: NotificationCompat.Builder

    override fun onBind(intent: Intent?): IBinder? {
        return null
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        when (intent?.action) {
            Actions.START.toString() -> start()
            Actions.STOP.toString() -> stopSelf()
        }

        return super.onStartCommand(intent, flags, startId)
    }

    private fun start() {
        notificationManager.notificationChannels
        val mapDownloader = MapDownloader(
            this,
            notificationManager,
            inProgressNotificationBuilder,
            notInProgressNotificationBuilder,
        )
        mapDownloader.downloadTjkMap()
        val notification = inProgressNotificationBuilder.build()
        notification.flags = Notification.FLAG_ONGOING_EVENT
        startForeground(NOTIFICATION_ID, notification)
    }

    enum class Actions {
        START, STOP
    }

    companion object {
        const val NOTIFICATION_ID = 1
        const val DOWNLOADING_CHANNEL = "Downloading channel"
        const val NOTIFICATION_CHANNEL = "Notification channel"
    }
}
