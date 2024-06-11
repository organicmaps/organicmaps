package app.tourism

import android.Manifest.permission
import android.content.Context
import android.content.pm.PackageManager
import android.text.TextUtils
import android.widget.Toast
import androidx.core.app.ActivityCompat
import androidx.core.app.NotificationCompat
import androidx.core.app.NotificationManagerCompat
import app.organicmaps.R
import app.organicmaps.downloader.CountryItem
import app.organicmaps.downloader.MapManager
import app.organicmaps.location.LocationHelper
import app.organicmaps.util.Config
import app.organicmaps.util.ConnectionState
import app.organicmaps.util.log.Logger
import kotlin.math.roundToInt

class MapDownloader(
    private val context: Context,
    private val notificationManager: NotificationManagerCompat,
    private val inProgressNotificationBuilder: NotificationCompat.Builder,
    private val notInProgressNotificationBuilder: NotificationCompat.Builder,
) {
    private var mStorageSubscriptionSlot = 0
    private var progress = 0
    private var mCurrentCountry = CountryItem.fill("Tajikistan")

    private val mStorageCallback: MapManager.StorageCallback = object :
        MapManager.StorageCallback {
        override fun onStatusChanged(data: List<MapManager.StorageCallbackData>) {
            for (item in data) {
                if (!item.isLeafNode) continue

                if (item.newStatus == CountryItem.STATUS_FAILED)
                    Toast.makeText(context, "failure", Toast.LENGTH_SHORT).show();

                if (mCurrentCountry.id == item.countryId) {
                    updateProgressState()
                    return
                }
            }
        }

        override fun onProgress(countryId: String, localSize: Long, remoteSize: Long) {
            if (mCurrentCountry.id == countryId) {
                updateProgressState()
            }
        }
    }

    private fun updateProgressState() {
        updateStateInternal()
    }

    private fun updateStateInternal() {
        val inProgress = (mCurrentCountry.status == CountryItem.STATUS_PROGRESS ||
                mCurrentCountry.status == CountryItem.STATUS_APPLYING)
        val failed = (mCurrentCountry.status == CountryItem.STATUS_FAILED)
        val success = (mCurrentCountry.status == CountryItem.STATUS_DONE)

        if (success) {
            notInProgressNotificationBuilder.setContentTitle(context.getString(R.string.map_downloaded))
            if (ActivityCompat.checkSelfPermission(
                    context,
                    permission.POST_NOTIFICATIONS
                ) == PackageManager.PERMISSION_GRANTED
            ) {
                notificationManager.notify(
                    DownloaderService.NOTIFICATION_ID,
                    notInProgressNotificationBuilder.build()
                )
            }
        }

        if (failed) {
            notInProgressNotificationBuilder.setContentTitle(context.getString(R.string.map_download_error))
            if (ActivityCompat.checkSelfPermission(
                    context,
                    permission.POST_NOTIFICATIONS
                ) == PackageManager.PERMISSION_GRANTED
            ) {
                notificationManager.notify(
                    DownloaderService.NOTIFICATION_ID,
                    notInProgressNotificationBuilder.build()
                )
            }
        }

        if (inProgress) {
            val progress = mCurrentCountry.progress.roundToInt()
            Logger.d("progress", progress.toString())
            inProgressNotificationBuilder.setProgress(100, progress, false)
            if (
                ActivityCompat.checkSelfPermission(context, permission.POST_NOTIFICATIONS)
                == PackageManager.PERMISSION_GRANTED
            ) {
                notificationManager.notify(
                    DownloaderService.NOTIFICATION_ID,
                    inProgressNotificationBuilder.build()
                )
            }
        } else {
            if (Config.isAutodownloadEnabled() &&
                !sAutodownloadLocked &&
                !failed &&
                ConnectionState.INSTANCE.isWifiConnected
            ) {
                val loc = LocationHelper.from(context).savedLocation
                if (loc != null) {
                    val country =
                        MapManager.nativeFindCountry(loc.latitude, loc.longitude)
                    if (TextUtils.equals(mCurrentCountry.id, country) &&
                        MapManager.nativeHasSpaceToDownloadCountry(country)
                    ) {
                        MapManager.nativeDownload(mCurrentCountry.id)
                    }
                }
            }
        }
    }

    public fun downloadTjkMap() {
        if (mCurrentCountry.present) return

        download()
    }

    public fun download() {
        onResume()
        val failed = mCurrentCountry.status == CountryItem.STATUS_FAILED
        if (failed) {
            MapManager.nativeRetry(mCurrentCountry.id)
        } else {
            MapManager.nativeDownload(mCurrentCountry.id)
        }
    }

    public fun cancel() {
        MapManager.nativeCancel(mCurrentCountry.id)
        setAutodownloadLocked(true)
    }

    fun onPause() {
        if (mStorageSubscriptionSlot > 0) {
            MapManager.nativeUnsubscribe(mStorageSubscriptionSlot)
            mStorageSubscriptionSlot = 0
        }
    }

    fun onResume() {
        if (mStorageSubscriptionSlot == 0) {
            mStorageSubscriptionSlot = MapManager.nativeSubscribe(mStorageCallback)
        }
    }

    companion object {
        private var sAutodownloadLocked = false

        fun setAutodownloadLocked(locked: Boolean) {
            sAutodownloadLocked = locked
        }
    }
}