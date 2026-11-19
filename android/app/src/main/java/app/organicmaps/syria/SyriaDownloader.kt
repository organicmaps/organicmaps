package app.organicmaps.syria

import android.content.Context
import android.text.format.DateUtils
import app.organicmaps.R
import app.organicmaps.sdk.downloader.MapManager

/** Downloader list additions: when each downloaded map was last updated on this device. */
object SyriaDownloader {
    private const val DATE_TIME_FLAGS = DateUtils.FORMAT_SHOW_DATE or DateUtils.FORMAT_SHOW_YEAR or
        DateUtils.FORMAT_ABBREV_MONTH or DateUtils.FORMAT_SHOW_TIME

    /** "Last updated: <date, time>" for a downloaded map, null when it isn't on this device. */
    @JvmStatic
    fun updatedText(context: Context, countryId: String): String? {
        val seconds = MapManager.nativeGetUpdatedTime(countryId)
        if (seconds <= 0) return null
        val dateTime = DateUtils.formatDateTime(context, seconds * DateUtils.SECOND_IN_MILLIS, DATE_TIME_FLAGS)
        return context.getString(R.string.syria_downloader_updated, dateTime)
    }
}
