package app.tourism

import android.Manifest
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.os.Build
import androidx.core.app.ActivityCompat
import androidx.core.app.NotificationCompat
import androidx.core.app.NotificationManagerCompat
import androidx.lifecycle.LifecycleService
import androidx.lifecycle.lifecycleScope
import app.organicmaps.R
import app.tourism.data.prefs.UserPreferences
import app.tourism.data.repositories.PlacesRepository
import app.tourism.domain.models.resource.DownloadProgress
import app.tourism.utils.LocaleHelper
import dagger.hilt.android.AndroidEntryPoint
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.collectLatest
import kotlinx.coroutines.launch
import javax.inject.Inject

private const val STOP_SERVICE_ACTION = "app.tourism.STOP_SERVICE"
private const val CHANNEL_ID = "images_download_channel"
private const val PROGRESS_NOTIFICATION_ID = 1
private const val SUMMARY_NOTIFICATION_ID = 2

@AndroidEntryPoint
class ImagesDownloadService : LifecycleService() {


    @Inject
    lateinit var placesRepository: PlacesRepository
    private lateinit var notificationManager: NotificationManagerCompat

    override fun attachBaseContext(newBase: Context) {
        val languageCode = UserPreferences(newBase).getLanguage()?.code
        super.attachBaseContext(LocaleHelper.localeUpdateResources(newBase, languageCode ?: "ru"))
    }

    override fun onCreate() {
        super.onCreate()

        notificationManager = NotificationManagerCompat.from(this)
        createNotificationChannel()
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        super.onStartCommand(intent, flags, startId)

        // Stops the service when cancel button is clicked
        if (intent?.action == STOP_SERVICE_ACTION) {
            stopSelf()
            return START_NOT_STICKY
        }

        // downloading all images
        lifecycleScope.launch(Dispatchers.IO) {
            placesRepository.downloadAllImages().collectLatest { progress ->
                updateNotification(progress)
            }
        }

        return START_STICKY
    }

    private fun createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val name = getString(R.string.channel_name)
            val descriptionText = getString(R.string.channel_description)
            val importance = NotificationManager.IMPORTANCE_HIGH
            val channel = NotificationChannel(CHANNEL_ID, name, importance).apply {
                description = descriptionText
            }

            notificationManager.createNotificationChannel(channel)
        }
    }

    private fun updateNotification(downloadProgress: DownloadProgress) {
        var shouldStopSelf = false

        val stopIntent = Intent(this, ImagesDownloadService::class.java).apply {
            action = STOP_SERVICE_ACTION
        }
        val stopPendingIntent = PendingIntent.getService(
            this, 0, stopIntent, PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
        )

        val builder = NotificationCompat.Builder(this, CHANNEL_ID)
            .setSmallIcon(R.drawable.ic_download)
            .setContentTitle(getString(R.string.downloading_images))
            .setSilent(true)
            .addAction(R.drawable.ic_cancel, getString(R.string.cancel), stopPendingIntent)

        val groupKey = "images_download_group"
        val summaryNotification = NotificationCompat.Builder(this, CHANNEL_ID)
            .setSmallIcon(R.drawable.ic_download)
            .setContentTitle(getString(R.string.downloading_images))
            .setStyle(NotificationCompat.InboxStyle())
            .setGroup(groupKey)
            .setGroupSummary(true)

        when (downloadProgress) {
            is DownloadProgress.Loading -> {
                downloadProgress.stats?.let { stats ->
                    val statsInString =
                        "${stats.filesDownloaded}/${stats.filesTotalNum} (${stats.percentagesCompleted}%)"

                    builder.setContentText("${getString(R.string.images_downloaded)}: $statsInString")
                    builder.setProgress(100, stats.percentagesCompleted, false)
                }
            }

            is DownloadProgress.Finished -> {
                downloadProgress.stats?.let { stats ->
                    val statsInString =
                        "${stats.filesDownloaded}/${stats.filesTotalNum} (${stats.percentagesCompleted}%)"

                    if (stats.percentagesCompleted == 100) {
                        summaryNotification.setContentTitle("${getString(R.string.all_images_were_downloaded)}: $statsInString")
                        summaryNotification.setContentText(null)
                    } else if (stats.percentagesCompleted >= 95) {
                        summaryNotification.setContentTitle("${getString(R.string.most_images_were_downloaded)}: $statsInString")
                        summaryNotification.setContentText(null)
                    } else {
                        summaryNotification.setContentTitle("${getString(R.string.not_all_images_were_downloaded)}: $statsInString")
                        summaryNotification.setContentText(null)
                    }
                }
                notificationManager.cancel(PROGRESS_NOTIFICATION_ID)
                shouldStopSelf = true
            }

            is DownloadProgress.Error -> {
                summaryNotification.setContentTitle(
                    downloadProgress.message ?: getString(R.string.smth_went_wrong)
                )
                summaryNotification.setContentText("")
                summaryNotification.setProgress(0, 0, false)

                notificationManager.cancel(PROGRESS_NOTIFICATION_ID)
                shouldStopSelf = true
            }

            else -> {}
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            val notificationPermission =
                ActivityCompat.checkSelfPermission(this, Manifest.permission.POST_NOTIFICATIONS)

            if (notificationPermission != PackageManager.PERMISSION_GRANTED)
                return
        }

        if (shouldStopSelf) {
            lifecycleScope.launch {
                notificationManager.notify(SUMMARY_NOTIFICATION_ID, summaryNotification.build())
                delay(1000L) // Delay to ensure notification is shown
                stopSelf()
            }
        } else {
            notificationManager.notify(PROGRESS_NOTIFICATION_ID, builder.build())
        }
    }
}
