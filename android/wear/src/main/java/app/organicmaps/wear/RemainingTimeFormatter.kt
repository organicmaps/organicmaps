package app.organicmaps.wear

/** Formats remaining navigation time using the same rounding as the phone. */
internal object RemainingTimeFormatter {
    fun format(seconds: Int?, hourLabel: String, minuteLabel: String): String? {
        if (seconds == null || seconds < 0) return null

        val totalMinutes = maxOf(1L, (seconds.toLong() + 30L) / 60L)
        val hours = totalMinutes / 60L
        val minutes = totalMinutes % 60L
        val hourPart = "$hours\u00A0$hourLabel"
        val minutePart = "$minutes\u00A0$minuteLabel"

        return when {
            hours == 0L -> minutePart
            minutes == 0L -> hourPart
            else -> "$hourPart\u00A0$minutePart"
        }
    }
}
