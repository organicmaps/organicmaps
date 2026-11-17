package app.organicmaps.syria.tracking

import android.content.Context
import app.organicmaps.BuildConfig
import app.organicmaps.sdk.downloader.CountryItem
import app.organicmaps.sdk.downloader.MapManager
import app.organicmaps.syria.SyriaPrefs
import java.util.UUID
import org.json.JSONObject

/**
 * Writes a `map_download` / `map_update` activity event when a map finishes downloading, if the
 * user enabled it in settings. Subscribed once the core is initialized.
 */
object MapUpdateLogger : MapManager.StorageCallback {
    private var slot = 0
    private lateinit var appContext: Context

    // Status seen before a map entered the download queue, to tell an update from a first download.
    private val statusBeforeDownload = HashMap<String, Int>()
    private val lastStatus = HashMap<String, Int>()

    @JvmStatic
    fun init(context: Context) {
        if (slot != 0) return
        appContext = context.applicationContext
        slot = MapManager.nativeSubscribe(this)
    }

    override fun onStatusChanged(data: List<MapManager.StorageCallbackData>) {
        for (item in data) {
            if (!item.isLeafNode) continue
            val previous = lastStatus.put(item.countryId, item.newStatus)
            val inProgress = item.newStatus in IN_PROGRESS
            if (inProgress && (previous == null || previous !in IN_PROGRESS)) {
                statusBeforeDownload[item.countryId] = previous ?: CountryItem.STATUS_UNKNOWN
            } else if (item.newStatus == CountryItem.STATUS_DONE && previous != null && previous in IN_PROGRESS) {
                val before = statusBeforeDownload.remove(item.countryId)
                record(item.countryId, if (before == CountryItem.STATUS_UPDATABLE) "map_update" else "map_download")
            }
        }
    }

    override fun onProgress(countryId: String, progress: MapManager.Progress) = Unit

    private fun record(countryId: String, type: String) {
        if (!SyriaPrefs.isLogMapUpdates(appContext) || !SyriaPrefs.isLoggedIn(appContext)) return
        val payload = JSONObject().put("countryId", countryId).put("appVersion", BuildConfig.VERSION_NAME)
        Thread {
            TrackingDb.get(appContext).insertEvent(
                TrackEvent(UUID.randomUUID().toString(), type, System.currentTimeMillis(), null, payload.toString()),
            )
            SyncWorker.enqueueNow(appContext)
        }.start()
    }

    private val IN_PROGRESS =
        setOf(CountryItem.STATUS_ENQUEUED, CountryItem.STATUS_PROGRESS, CountryItem.STATUS_APPLYING)
}
