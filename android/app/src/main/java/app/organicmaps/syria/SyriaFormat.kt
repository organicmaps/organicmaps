package app.organicmaps.syria

import android.content.Context
import app.organicmaps.R

/** Text formatting of the Syria additions: durations and phone numbers. */
object SyriaFormat {
    private const val SYRIA_CODE = "963"
    private const val LTR_ISOLATE = '⁦'
    private const val POP_ISOLATE = '⁩'
    private const val SECONDS_PER_DAY = 86_400L

    /** Days, hours, minutes and seconds of a duration, from the first non-zero unit down to seconds. */
    data class Parts(val days: Long, val hours: Long, val minutes: Long, val seconds: Long)

    @JvmStatic
    fun parts(totalSeconds: Long): Parts {
        val s = totalSeconds.coerceAtLeast(0)
        return Parts(s / SECONDS_PER_DAY, s % SECONDS_PER_DAY / 3600, s % 3600 / 60, s % 60)
    }

    /** "1d 2h 5m 7s" / "1 يوم 2 س 5 د 7 ث"; leading zero units are left out, "0s" for zero. */
    @JvmStatic
    fun duration(context: Context, totalSeconds: Long): String =
        duration(totalSeconds) { unit, value -> context.getString(unit, value) }

    /** Formatting core, with the unit lookup injected so that it can be unit tested. */
    fun duration(totalSeconds: Long, unit: (Int, Long) -> String): String {
        val p = parts(totalSeconds)
        val units = listOf(
            R.string.syria_duration_days to p.days,
            R.string.syria_duration_hours to p.hours,
            R.string.syria_duration_minutes to p.minutes,
            R.string.syria_duration_seconds to p.seconds,
        )
        val first = units.indexOfFirst { it.second > 0 }.takeIf { it >= 0 } ?: units.lastIndex
        return units.drop(first).joinToString(" ") { (res, value) -> unit(res, value) }
    }

    /** Stopwatch text for the live counters: "05:07", "2:05:07", or "1d 02:05:07" past a day. */
    @JvmStatic
    fun clock(context: Context, totalSeconds: Long): String =
        clock(totalSeconds) { days -> context.getString(R.string.syria_duration_days, days) }

    fun clock(totalSeconds: Long, days: (Long) -> String): String {
        val p = parts(totalSeconds)
        val time = when {
            p.days > 0 -> "%02d:%02d:%02d".format(p.hours, p.minutes, p.seconds)
            p.hours > 0 -> "%d:%02d:%02d".format(p.hours, p.minutes, p.seconds)
            else -> "%02d:%02d".format(p.minutes, p.seconds)
        }
        return if (p.days > 0) days(p.days) + " " + time else time
    }

    /**
     * "+963944123456" -> "00963 944 123 456", wrapped in an LTR isolate so that the digits keep their
     * order inside Arabic text. "00" instead of "+" avoids the sign jumping to the wrong side in RTL.
     */
    @JvmStatic
    fun phone(e164: String?): String {
        if (e164.isNullOrBlank()) return ""
        val digits = e164.filter { it.isDigit() }
        val formatted = if (digits.startsWith(SYRIA_CODE)) {
            "00$SYRIA_CODE " + digits.substring(SYRIA_CODE.length).chunked(3).joinToString(" ")
        } else {
            "00$digits"
        }
        return "$LTR_ISOLATE$formatted$POP_ISOLATE"
    }

    /**
     * Turns what the user typed after the fixed "00963" prefix into an international number:
     * "0944 123 456" and "944123456" -> "+963944123456"; "00…" and "+…" are kept as typed.
     */
    @JvmStatic
    fun normalizePhoneInput(input: String): String {
        val trimmed = input.trim()
        val digits = trimmed.filter { it.isDigit() }
        return when {
            trimmed.startsWith("+") -> "+$digits"
            digits.startsWith("00") -> "+" + digits.substring(2)
            digits.startsWith(SYRIA_CODE) && digits.length > SYRIA_LOCAL_LENGTH -> "+$digits"
            else -> "+$SYRIA_CODE" + digits.trimStart('0')
        }
    }

    // Syrian national numbers without the trunk 0 have 9 digits (mobile 9XX XXX XXX).
    private const val SYRIA_LOCAL_LENGTH = 9
}
