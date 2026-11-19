package app.organicmaps.syria.tracking

import android.content.Context
import android.os.SystemClock
import app.organicmaps.BuildConfig
import app.organicmaps.sdk.downloader.CountryItem
import app.organicmaps.sdk.downloader.MapManager
import app.organicmaps.syria.SyriaPrefs
import app.organicmaps.syria.SyriaServer
import java.util.UUID
import org.json.JSONObject

/**
 * Activity events about map downloads, if the user keeps them on in Settings:
 * - `map_download` / `map_update` when a map finished downloading,
 * - `map_download_failed` when it stopped with an error,
 * with how long it took (`durationMs`, from the start of the download to the end, pauses included) and how many
 * bytes came down (`bytes`; `totalBytes` is the map size). Like searches, they are queued on the phone while
 * signed out and uploaded under the account that signs in next.
 *
 * Subscribed as soon as the core has loaded, so the first-launch download of the country map is covered too;
 * the World maps of the first launch use another downloader and are reported by [recordFirstLaunchWorld].
 */
object MapUpdateLogger : MapManager.StorageCallback {
    private var slot = 0
    private lateinit var appContext: Context

    private class Download(val statusBefore: Int, val startedAt: Long) {
        var bytes = 0L
        var totalBytes = 0L
    }

    private val downloads = HashMap<String, Download>()
    private val lastStatus = HashMap<String, Int>()

    // Countries downloaded by the first-launch screen: their event says firstLaunch=true.
    private val firstLaunch = HashSet<String>()

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
            val wasInProgress = previous != null && previous in IN_PROGRESS
            when {
                inProgress && !wasInProgress -> downloads[item.countryId] =
                    Download(previous ?: CountryItem.STATUS_UNKNOWN, SystemClock.elapsedRealtime())
                wasInProgress && item.newStatus == CountryItem.STATUS_DONE -> finished(item.countryId, null)
                wasInProgress && item.newStatus == CountryItem.STATUS_FAILED -> finished(item.countryId, item.errorCode)
                !inProgress -> downloads.remove(item.countryId) // cancelled or deleted
            }
        }
    }

    override fun onProgress(countryId: String, progress: MapManager.Progress) {
        val download = downloads[countryId] ?: return
        // The map file only: its terrain (contour lines) downloads on its own and may follow later.
        download.bytes = progress.mwmDownloaded
        download.totalBytes = progress.mwmTotal
    }

    private fun finished(countryId: String, errorCode: Int?) {
        val download = downloads.remove(countryId) ?: return
        // Only terrain came down (the map was already there): not a map download.
        if (download.totalBytes == 0L) return
        val type = when {
            errorCode != null -> "map_download_failed"
            download.statusBefore == CountryItem.STATUS_UPDATABLE -> "map_update"
            else -> "map_download"
        }
        val payload = basePayload(countryId, SystemClock.elapsedRealtime() - download.startedAt, download.bytes)
            .put("totalBytes", download.totalBytes)
        errorCode?.let { payload.put("errorCode", it) }
        // A failed first download is retried from the downloader later: only the outcome of this one is tagged.
        if (firstLaunch.remove(countryId)) payload.put("firstLaunch", true)
        record(type, payload)
    }

    /** The first-launch screen starts downloading this country: its event gets firstLaunch=true. */
    @JvmStatic
    fun markFirstLaunch(countryId: String) {
        firstLaunch.add(countryId)
    }

    /** First launch: World + WorldCoasts, fetched before the map opens by DownloadResourcesLegacyActivity. */
    @JvmStatic
    fun recordFirstLaunchWorld(context: Context, bytes: Long, durationMs: Long) {
        if (!::appContext.isInitialized) appContext = context.applicationContext
        record("map_download", basePayload("World", durationMs, bytes).put("firstLaunch", true))
    }

    private fun basePayload(countryId: String, durationMs: Long, bytes: Long): JSONObject =
        JSONObject()
            .put("countryId", countryId)
            .put("durationMs", durationMs)
            .put("bytes", bytes)
            .put("server", SyriaServer.host(SyriaServer.current()))
            .put("appVersion", BuildConfig.VERSION_NAME)

    private fun record(type: String, payload: JSONObject) {
        if (!SyriaPrefs.isLogMapUpdates(appContext)) return
        Thread {
            TrackingDb.get(appContext).insertEvent(
                TrackEvent(UUID.randomUUID().toString(), type, System.currentTimeMillis(), null, payload.toString()),
            )
            if (SyriaPrefs.isLoggedIn(appContext)) SyncWorker.enqueueNow(appContext)
        }.start()
    }

    private val IN_PROGRESS =
        setOf(CountryItem.STATUS_ENQUEUED, CountryItem.STATUS_PROGRESS, CountryItem.STATUS_APPLYING)
}
