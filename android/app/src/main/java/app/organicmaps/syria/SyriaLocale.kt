package app.organicmaps.syria

import android.app.LocaleManager
import android.content.Context
import android.os.Build
import android.os.LocaleList
import androidx.appcompat.app.AppCompatDelegate
import androidx.core.os.LocaleListCompat

/** Arabic is the default UI language; the user can switch to English in the system per-app language settings. */
object SyriaLocale {
    private const val DEFAULT_LANGUAGE = "ar"

    /**
     * Runs from Application.onCreate, before any activity exists. AppCompat silently ignores
     * setApplicationLocales there on Android 13+, so the framework service is used directly.
     */
    @JvmStatic
    fun applyDefaultOnce(context: Context) {
        if (SyriaPrefs.isDefaultLocaleApplied(context)) return
        SyriaPrefs.markDefaultLocaleApplied(context)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            val manager = context.getSystemService(LocaleManager::class.java)
            if (manager.applicationLocales.isEmpty) {
                manager.applicationLocales = LocaleList.forLanguageTags(DEFAULT_LANGUAGE)
            }
        } else if (AppCompatDelegate.getApplicationLocales().isEmpty) {
            AppCompatDelegate.setApplicationLocales(LocaleListCompat.forLanguageTags(DEFAULT_LANGUAGE))
        }
    }
}
