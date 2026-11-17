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
    private const val KEY_LAST_SYNC_FAILED = "last_sync_failed"
    private const val KEY_LAST_SYNC_ERROR = "last_sync_error"
    private const val KEY_LAST_SYNC_RESULT_AT = "last_sync_result_at"

    /** Stored as the last sync error when the server could not be reached at all. */
    const val SYNC_ERROR_UNREACHABLE = "unreachable"
    private const val KEY_APP_LANGUAGE = "app_language"
    private const val KEY_ROUTE_OPTIMIZATION_DEFAULT = "route_optimization_default_applied"
    private const val KEY_NOTIFICATION_PERMISSION_ASKED = "notification_permission_asked"
    private const val KEY_TUTORIAL_DONE = "tutorial_done"

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
        SyriaStatus.refresh(context)
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
        SyriaStatus.refresh(context)
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

    @JvmStatic
    fun isLastSyncFailed(context: Context): Boolean = prefs(context).getBoolean(KEY_LAST_SYNC_FAILED, false)

    /** Records the result of the last sync: null error means it succeeded. */
    @JvmStatic
    fun setLastSyncResult(context: Context, error: String?) {
        prefs(context).edit()
            .putBoolean(KEY_LAST_SYNC_FAILED, error != null)
            .putString(KEY_LAST_SYNC_ERROR, error)
            .putLong(KEY_LAST_SYNC_RESULT_AT, System.currentTimeMillis())
            .apply()
    }

    /** When the last sync attempt finished, successfully or not. */
    @JvmStatic
    fun lastSyncResultAt(context: Context): Long = prefs(context).getLong(KEY_LAST_SYNC_RESULT_AT, 0L)

    @JvmStatic
    fun lastSyncError(context: Context): String? = prefs(context).getString(KEY_LAST_SYNC_ERROR, null)

    /** Language tag of the UI the user last saw, to notice a change made in the system settings. */
    @JvmStatic
    fun appLanguage(context: Context): String? = prefs(context).getString(KEY_APP_LANGUAGE, null)

    @JvmStatic
    fun setAppLanguage(context: Context, tag: String) {
        prefs(context).edit().putString(KEY_APP_LANGUAGE, tag).apply()
    }

    @JvmStatic
    fun isTutorialDone(context: Context): Boolean = prefs(context).getBoolean(KEY_TUTORIAL_DONE, false)

    @JvmStatic
    fun markTutorialDone(context: Context) {
        prefs(context).edit().putBoolean(KEY_TUTORIAL_DONE, true).apply()
    }

    @JvmStatic
    fun resetTutorial(context: Context) {
        prefs(context).edit().putBoolean(KEY_TUTORIAL_DONE, false).apply()
    }

    @JvmStatic
    fun isNotificationPermissionAsked(context: Context): Boolean =
        prefs(context).getBoolean(KEY_NOTIFICATION_PERMISSION_ASKED, false)

    @JvmStatic
    fun markNotificationPermissionAsked(context: Context) {
        prefs(context).edit().putBoolean(KEY_NOTIFICATION_PERMISSION_ASKED, true).apply()
    }

    @JvmStatic
    fun isRouteOptimizationDefaultApplied(context: Context): Boolean =
        prefs(context).getBoolean(KEY_ROUTE_OPTIMIZATION_DEFAULT, false)

    @JvmStatic
    fun markRouteOptimizationDefaultApplied(context: Context) {
        prefs(context).edit().putBoolean(KEY_ROUTE_OPTIMIZATION_DEFAULT, true).apply()
    }
}
