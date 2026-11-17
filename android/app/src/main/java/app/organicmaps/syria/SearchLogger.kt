package app.organicmaps.syria

import android.content.Context
import androidx.annotation.MainThread
import app.organicmaps.syria.tracking.SyncWorker
import app.organicmaps.syria.tracking.TrackEvent
import app.organicmaps.syria.tracking.TrackingDb
import java.util.UUID
import org.json.JSONObject

/**
 * Saves searches as `search` events in the offline outbox. They are queued even when signed out and
 * uploaded under the account once the user signs in. One search is the last query typed, logged when
 * a result is picked, all results are shown on the map, or the search is closed without a pick.
 */
@MainThread
object SearchLogger {
    private class Pending(
        val query: String,
        val locale: String?,
        val isCategory: Boolean,
        val lat: Double?,
        val lon: Double?,
        val startedAt: Long,
    ) {
        var resultCount = -1
        var logged = false
    }

    private var pending: Pending? = null

    @JvmStatic
    fun onQuery(query: String, locale: String?, isCategory: Boolean, hasLocation: Boolean, lat: Double, lon: Double) {
        val text = query.trim()
        if (text.isEmpty()) {
            pending = null
            return
        }
        if (pending?.query == text) return
        pending = Pending(
            text,
            locale,
            isCategory,
            if (hasLocation) lat else null,
            if (hasLocation) lon else null,
            System.currentTimeMillis(),
        )
    }

    @JvmStatic
    fun onResults(count: Int) {
        pending?.resultCount = count
    }

    @JvmStatic
    fun onResultSelected(context: Context, name: String?, featureType: String?, lat: Double, lon: Double, index: Int) {
        val selected = JSONObject()
            .put("name", name ?: JSONObject.NULL)
            .put("type", featureType ?: JSONObject.NULL)
            .put("lat", lat)
            .put("lon", lon)
            .put("index", index)
        write(context, "select", selected)
    }

    @JvmStatic
    fun onShowAll(context: Context) = write(context, "show_all", null)

    /** The search screen closed: logs the last query if nothing was picked from it. */
    @JvmStatic
    fun onClosed(context: Context) {
        if (pending?.logged == false) write(context, "abandon", null)
        pending = null
    }

    private fun write(context: Context, action: String, selected: JSONObject?) {
        val p = pending ?: return
        // A user may go back to the same results and pick another one: each pick is its own event.
        p.logged = true
        val payload = JSONObject()
            .put("query", p.query)
            .put("locale", p.locale ?: JSONObject.NULL)
            .put("isCategory", p.isCategory)
            .put("resultCount", if (p.resultCount >= 0) p.resultCount else JSONObject.NULL)
            .put("action", action)
            .put("selected", selected ?: JSONObject.NULL)
            .put("lat", p.lat ?: JSONObject.NULL)
            .put("lon", p.lon ?: JSONObject.NULL)
            .put("searchedAt", SyncWorker.iso(p.startedAt))
        val event = TrackEvent(UUID.randomUUID().toString(), TYPE, System.currentTimeMillis(), null, payload.toString())
        val appContext = context.applicationContext
        Thread {
            TrackingDb.get(appContext).insertEvent(event)
            if (SyriaPrefs.isLoggedIn(appContext)) SyncWorker.enqueueNow(appContext)
        }.start()
    }

    private const val TYPE = "search"
}
