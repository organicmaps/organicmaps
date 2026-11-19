package app.organicmaps.syria

import android.app.Activity
import android.app.LocaleManager
import android.content.Context
import android.content.Intent
import android.os.Build
import android.os.LocaleList
import androidx.appcompat.app.AppCompatDelegate
import androidx.core.os.ConfigurationCompat
import androidx.core.os.LocaleListCompat
import app.organicmaps.R
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import java.util.Locale

/**
 * Arabic is the default UI language. The user can switch to English in the app settings or in the system
 * per-app language settings; both store the choice in the same place (the system on Android 13+).
 */
object SyriaLocale {
    private const val DEFAULT_LANGUAGE = "ar"

    /**
     * Runs from Application.onCreate on a fresh install, before any activity exists. AppCompat silently
     * ignores setApplicationLocales there on Android 13+, so the framework service is used directly.
     * The new configuration reaches the process only later, while the map core reads Locale.getDefault()
     * as it starts: set that too, so the map, search and voice start in Arabic as well.
     */
    @JvmStatic
    fun applyDefaultOnce(context: Context) {
        if (SyriaPrefs.isDefaultLocaleApplied(context)) return
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            val manager = context.getSystemService(LocaleManager::class.java)
            if (!manager.applicationLocales.isEmpty) return
            manager.applicationLocales = LocaleList.forLanguageTags(DEFAULT_LANGUAGE)
        } else {
            if (!AppCompatDelegate.getApplicationLocales().isEmpty) return
            AppCompatDelegate.setApplicationLocales(LocaleListCompat.forLanguageTags(DEFAULT_LANGUAGE))
        }
        Locale.setDefault(Locale(DEFAULT_LANGUAGE))
    }

    /**
     * Confirms the Arabic default from the first screens. Returns false while it is still being applied
     * (the screen is then recreated in Arabic). On Android 13+ AppCompat's one-time locale sync can reset
     * the language set in Application.onCreate, so it is re-applied until a language is seen in effect.
     * After that a language chosen by the user, "system default" included, is left alone.
     */
    @JvmStatic
    fun ensureDefault(activity: Activity): Boolean {
        if (SyriaPrefs.isDefaultLocaleApplied(activity)) return true
        if (AppCompatDelegate.getApplicationLocales().isEmpty) {
            AppCompatDelegate.setApplicationLocales(LocaleListCompat.forLanguageTags(DEFAULT_LANGUAGE))
            return false
        }
        SyriaPrefs.markDefaultLocaleApplied(activity)
        return true
    }

    /** UI language currently shown: "ar" or "en" (the two languages the app ships). */
    @JvmStatic
    fun currentLanguage(context: Context): String {
        val locale = ConfigurationCompat.getLocales(context.resources.configuration)[0]
        return if (locale?.language == DEFAULT_LANGUAGE) DEFAULT_LANGUAGE else "en"
    }

    /** Switches the UI language from the app settings; Android recreates the open screens. */
    @JvmStatic
    fun setAppLanguage(languageTag: String) {
        AppCompatDelegate.setApplicationLocales(LocaleListCompat.forLanguageTags(languageTag))
    }

    /**
     * Android applies a per-app language change to the UI right away, but the map, search and voice
     * guidance read the language when the core starts. Offers a restart the first time the main
     * screen shows a language different from the one seen before.
     */
    @JvmStatic
    fun checkChanged(activity: Activity) {
        // No prompt while the first-run default is still being applied.
        if (!ensureDefault(activity)) return
        val locale = ConfigurationCompat.getLocales(activity.resources.configuration)[0] ?: return
        val current = locale.language
        val previous = SyriaPrefs.appLanguage(activity)
        if (previous == current) return
        SyriaPrefs.setAppLanguage(activity, current)
        if (previous == null) return
        MaterialAlertDialogBuilder(activity)
            .setTitle(R.string.syria_language_changed_title)
            .setMessage(R.string.syria_language_changed_message)
            .setNegativeButton(R.string.syria_later, null)
            .setPositiveButton(R.string.syria_restart_now) { _, _ -> restart(activity) }
            .show()
    }

    /** Restarts the app from its launcher screen (a new process, so the core starts afresh). */
    @JvmStatic
    fun restart(activity: Activity) {
        val launch = activity.packageManager.getLaunchIntentForPackage(activity.packageName) ?: return
        activity.startActivity(Intent.makeRestartActivityTask(launch.component))
        Runtime.getRuntime().exit(0)
    }
}
