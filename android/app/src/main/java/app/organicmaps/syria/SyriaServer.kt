package app.organicmaps.syria

import android.app.Activity
import android.content.Context
import android.net.Uri
import app.organicmaps.BuildConfig
import app.organicmaps.sdk.OrganicMaps
import app.organicmaps.syria.tracking.ActivityTrackingService
import app.organicmaps.syria.tracking.LiveLocationService
import org.json.JSONArray
import org.json.JSONObject

/**
 * The Syria Organic Maps server the app talks to: tracking API and map files. Public builds (Google Play, APK)
 * always use organicmaps.openstreetmap.sy; development builds (BuildConfig.SYRIA_DEV_SETTINGS) can switch to the
 * local server in Settings. The choice is read once at start: switching signs out and restarts the app.
 */
object SyriaServer {
    const val PRODUCTION = "production"
    const val LOCAL = "local"

    @Volatile
    private var current = PRODUCTION

    /** Whether Settings offers the choice (development builds only). */
    @JvmStatic
    val isSelectable: Boolean
        get() = BuildConfig.SYRIA_DEV_SETTINGS

    /** Reads the choice and points the map downloader at that server. Call before the core starts. */
    @JvmStatic
    fun init(context: Context) {
        current = if (isSelectable) SyriaPrefs.server(context) ?: LOCAL else PRODUCTION
        val maps = JSONArray().put(mapsUrl())
        OrganicMaps.nativeSetDefaultUrls(JSONObject().put("servers", maps).put("terrainServers", maps).toString())
    }

    @JvmStatic
    fun current(): String = current

    val apiUrl: String
        get() = (if (current == LOCAL) BuildConfig.SYRIA_LOCAL_API_URL else BuildConfig.SYRIA_API_URL).trimEnd('/')

    /** Latest app release for [app.organicmaps.syria.update.ServerUpdateCheck]: on the website, or the local map server. */
    val appVersionUrl: String
        get() = (if (current == LOCAL) BuildConfig.SYRIA_LOCAL_MAPS_URL else BuildConfig.SYRIA_WEBSITE_URL) +
            "apps/version.json"

    private fun mapsUrl(): String = if (current == LOCAL) BuildConfig.SYRIA_LOCAL_MAPS_URL else BuildConfig.SYRIA_MAPS_URL

    /** Host shown in Settings next to each choice. */
    @JvmStatic
    fun host(server: String): String =
        Uri.parse(if (server == LOCAL) BuildConfig.SYRIA_LOCAL_API_URL else BuildConfig.SYRIA_API_URL).host.orEmpty()

    /**
     * Switches to the other server: the account belongs to the server it was created on, so the user is signed
     * out (unsynced recordings stay on the phone and go to the new server once signed in there), then the app
     * restarts so the map downloader uses the new server too. Refused (false) while a recording runs.
     */
    @JvmStatic
    fun switchTo(activity: Activity, server: String): Boolean {
        if (server == current) return true
        if (ActivityTrackingService.isActive()) return false
        LiveLocationService.disable(activity)
        SyriaPrefs.switchServer(activity, server)
        SyriaLocale.restart(activity)
        return true
    }
}
