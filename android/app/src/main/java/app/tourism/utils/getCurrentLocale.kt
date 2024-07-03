package app.tourism.utils

import androidx.compose.ui.text.intl.Locale

fun getCurrentLocale(): String {
    var language = Locale.current.language
    if (language == "tg") language = "tj"
    return language
}