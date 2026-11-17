package app.organicmaps.syria

import android.content.Context
import android.content.SharedPreferences
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import java.util.UUID

/**
 * Account session and settings of the Syria Organic Maps additions.
 * Stored in private preferences, like the OSM editor login of Organic Maps.
 */
object SyriaPrefs {
    private const val FILE = "syria_prefs"
    private const val KEY_DEVICE_ID = "device_id"
    private const val KEY_ACCESS = "access_token"
    private const val KEY_REFRESH = "refresh_token"
    private const val KEY_USER_ID = "user_id"
    private const val KEY_PHONE = "phone"
    private const val KEY_NAME = "display_name"
    private const val KEY_LOG_MAP_UPDATES = "log_map_updates"
    private const val KEY_LOCALE_APPLIED = "default_locale_applied"
    private const val KEY_LAST_SYNC = "last_sync_at"

    private val loggedIn = MutableLiveData<Boolean>()

    private fun prefs(context: Context): SharedPreferences =
        context.applicationContext.getSharedPreferences(FILE, Context.MODE_PRIVATE)

    /** Stable per-install id, generated on first use. */
    @JvmStatic
    fun deviceId(context: Context): String {
        val p = prefs(context)
        p.getString(KEY_DEVICE_ID, null)?.let { return it }
        val id = UUID.randomUUID().toString()
        p.edit().putString(KEY_DEVICE_ID, id).apply()
        return id
    }

    @JvmStatic
    fun isLoggedIn(context: Context): Boolean = prefs(context).getString(KEY_REFRESH, null) != null

    @JvmStatic
    fun loggedIn(context: Context): LiveData<Boolean> {
        if (loggedIn.value == null) loggedIn.postValue(isLoggedIn(context))
        return loggedIn
    }

    @JvmStatic fun accessToken(context: Context): String? = prefs(context).getString(KEY_ACCESS, null)

    @JvmStatic fun refreshToken(context: Context): String? = prefs(context).getString(KEY_REFRESH, null)

    @JvmStatic fun userId(context: Context): String? = prefs(context).getString(KEY_USER_ID, null)

    @JvmStatic fun phone(context: Context): String? = prefs(context).getString(KEY_PHONE, null)

    @JvmStatic fun displayName(context: Context): String? = prefs(context).getString(KEY_NAME, null)

    /** Everything kept locally about a signed-in user. */
    data class Session(
        val accessToken: String,
        val refreshToken: String,
        val userId: String,
        val phone: String?,
        val displayName: String?,
    )

    fun setSession(context: Context, session: Session) {
        prefs(context).edit()
            .putString(KEY_ACCESS, session.accessToken)
            .putString(KEY_REFRESH, session.refreshToken)
            .putString(KEY_USER_ID, session.userId)
            .putString(KEY_PHONE, session.phone)
            .putString(KEY_NAME, session.displayName)
            .apply()
        loggedIn.postValue(true)
    }

    fun setTokens(context: Context, accessToken: String, refreshToken: String) {
        prefs(context).edit().putString(KEY_ACCESS, accessToken).putString(KEY_REFRESH, refreshToken).apply()
    }

    @JvmStatic
    fun clearSession(context: Context) {
        prefs(context).edit()
            .remove(KEY_ACCESS)
            .remove(KEY_REFRESH)
            .remove(KEY_USER_ID)
            .remove(KEY_PHONE)
            .remove(KEY_NAME)
            .apply()
        loggedIn.postValue(false)
    }

    @JvmStatic
    fun isLogMapUpdates(context: Context): Boolean = prefs(context).getBoolean(KEY_LOG_MAP_UPDATES, true)

    @JvmStatic
    fun setLogMapUpdates(context: Context, enabled: Boolean) {
        prefs(context).edit().putBoolean(KEY_LOG_MAP_UPDATES, enabled).apply()
    }

    @JvmStatic
    fun isDefaultLocaleApplied(context: Context): Boolean = prefs(context).getBoolean(KEY_LOCALE_APPLIED, false)

    @JvmStatic
    fun markDefaultLocaleApplied(context: Context) {
        prefs(context).edit().putBoolean(KEY_LOCALE_APPLIED, true).apply()
    }

    @JvmStatic
    fun lastSyncAt(context: Context): Long = prefs(context).getLong(KEY_LAST_SYNC, 0L)

    @JvmStatic
    fun setLastSyncAt(context: Context, timeMs: Long) {
        prefs(context).edit().putLong(KEY_LAST_SYNC, timeMs).apply()
    }
}
