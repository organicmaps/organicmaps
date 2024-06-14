package app.tourism.utils

import android.app.LocaleManager
import android.content.Context
import android.os.Build
import android.os.LocaleList
import androidx.appcompat.app.AppCompatDelegate
import androidx.core.os.LocaleListCompat

fun changeSystemAppLanguage(context: Context, language: String) {
    var locale = language
    if (language == "tj") locale = "tg"

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU)
        context.getSystemService(LocaleManager::class.java).applicationLocales =
            LocaleList.forLanguageTags(locale)
    else
        AppCompatDelegate.setApplicationLocales(LocaleListCompat.forLanguageTags(locale))
}