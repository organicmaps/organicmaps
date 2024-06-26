package app.tourism.utils

import java.text.ParseException
import java.text.SimpleDateFormat
import java.util.Calendar
import java.util.Date
import java.util.Locale

fun String.toUserFriendlyDate(dateFormat: String = "yyyy-MM-dd"): String {
    var userFriendlyDate = ""

    val currentLocale = Locale.getDefault()
    val formatter = SimpleDateFormat(dateFormat, currentLocale)

    var date: Date? = null

    try {
        date = formatter.parse(this)
    } catch (e: ParseException) {
        userFriendlyDate = this
    }
    val givenDate = Calendar.getInstance()

    if (date != null) {
        givenDate.time = date

        givenDate.isLenient = false
        val givenDay = givenDate.get(Calendar.DAY_OF_MONTH)
        val givenMonth = givenDate.getDisplayName(Calendar.MONTH, Calendar.LONG, currentLocale)
        val givenYear = givenDate.get(Calendar.YEAR)

        userFriendlyDate = "$givenDay $givenMonth $givenYear"
    }
    return userFriendlyDate
}
