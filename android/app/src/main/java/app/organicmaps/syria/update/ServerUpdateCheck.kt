package app.organicmaps.syria.update

import android.app.Activity
import android.content.ActivityNotFoundException
import android.content.Context
import android.content.Intent
import android.net.Uri
import android.os.Handler
import android.os.Looper
import app.organicmaps.BuildConfig
import app.organicmaps.R
import app.organicmaps.sdk.util.log.Logger
import app.organicmaps.syria.SyriaLocale
import app.organicmaps.syria.SyriaServer
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import java.net.HttpURLConnection
import java.net.URL
import org.json.JSONObject

/**
 * Reads the latest release from our server (`apps/version.json`, see [SyriaServer.appVersionUrl]):
 *
 * ```
 * {"notes": {"ar": "…", "en": "…"},
 *  "google": {"versionCode": 26112001, "versionName": "2026.11.20", "minVersionCode": 0, "url": "https://…apk"},
 *  "huawei": {"versionCode": 126112001, …}}
 * ```
 *
 * keyed by build flavor because each flavor has its own version codes. A newer version is offered with a link
 * to its APK; one below `minVersionCode` must be updated (no "Later"). "Later" hides that version for 3 days.
 */
object ServerUpdateCheck {
    private const val TAG = "ServerUpdateCheck"
    private const val PREFS = "syria_app_update"
    private const val KEY_CHECKED_AT = "checked_at"
    private const val KEY_SNOOZED_CODE = "snoozed_code"
    private const val KEY_SNOOZED_AT = "snoozed_at"
    private const val CHECK_EVERY_MS = 6 * 3600_000L
    private const val SNOOZE_MS = 3 * 24 * 3600_000L
    private const val TIMEOUT_MS = 10_000

    private class Release(val code: Long, val name: String, val minCode: Long, val url: String, val notes: String)

    fun check(activity: Activity) {
        val prefs = activity.getSharedPreferences(PREFS, Context.MODE_PRIVATE)
        val now = System.currentTimeMillis()
        if (now - prefs.getLong(KEY_CHECKED_AT, 0) < CHECK_EVERY_MS) return
        val language = SyriaLocale.currentLanguage(activity)
        Thread {
            val release = fetch(language) ?: return@Thread
            // A required update is asked again at every start until it is installed.
            if (BuildConfig.VERSION_CODE >= release.minCode) prefs.edit().putLong(KEY_CHECKED_AT, now).apply()
            Handler(Looper.getMainLooper()).post { offer(activity, release) }
        }.start()
    }

    private fun fetch(language: String): Release? = try {
        val connection = URL(SyriaServer.appVersionUrl).openConnection() as HttpURLConnection
        connection.connectTimeout = TIMEOUT_MS
        connection.readTimeout = TIMEOUT_MS
        connection.useCaches = false
        try {
            if (connection.responseCode != HttpURLConnection.HTTP_OK) {
                null
            } else {
                val json = JSONObject(connection.inputStream.bufferedReader().use { it.readText() })
                val flavor = json.optJSONObject(BuildConfig.FLAVOR)
                val notes = json.optJSONObject("notes")
                flavor?.let {
                    Release(
                        code = it.getLong("versionCode"),
                        name = it.getString("versionName"),
                        minCode = it.optLong("minVersionCode", 0),
                        url = it.getString("url"),
                        notes = notes?.optString(language).orEmpty(),
                    )
                }
            }
        } finally {
            connection.disconnect()
        }
    } catch (e: Exception) {
        // Offline or no release published: try again at the next check.
        Logger.w(TAG, "No release info: ${e.message}")
        null
    }

    private fun offer(activity: Activity, release: Release) {
        if (activity.isFinishing || activity.isDestroyed) return
        if (release.code <= BuildConfig.VERSION_CODE) return
        val required = BuildConfig.VERSION_CODE < release.minCode
        val prefs = activity.getSharedPreferences(PREFS, Context.MODE_PRIVATE)
        val snoozed = prefs.getLong(KEY_SNOOZED_CODE, 0) == release.code &&
            System.currentTimeMillis() - prefs.getLong(KEY_SNOOZED_AT, 0) < SNOOZE_MS
        if (snoozed && !required) return

        var message = activity.getString(R.string.syria_update_message, release.name)
        if (release.notes.isNotBlank()) message += "\n\n" + release.notes
        val dialog = MaterialAlertDialogBuilder(activity)
            .setTitle(if (required) R.string.syria_update_required_title else R.string.syria_update_title)
            .setMessage(message)
            .setCancelable(!required)
            .setPositiveButton(R.string.syria_update_download) { _, _ -> download(activity, release.url) }
        if (!required) {
            dialog.setNegativeButton(R.string.syria_update_later) { _, _ ->
                prefs.edit()
                    .putLong(KEY_SNOOZED_CODE, release.code)
                    .putLong(KEY_SNOOZED_AT, System.currentTimeMillis())
                    .apply()
            }
        }
        dialog.show()
    }

    /** The browser downloads the APK; Android then installs it over this one, keeping maps and recordings. */
    private fun download(activity: Activity, url: String) {
        try {
            activity.startActivity(Intent(Intent.ACTION_VIEW, Uri.parse(url)))
        } catch (e: ActivityNotFoundException) {
            Logger.w(TAG, "No browser to open $url")
        }
    }
}
