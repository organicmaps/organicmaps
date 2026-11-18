package app.organicmaps.syria

import app.organicmaps.R
import org.junit.Assert.assertEquals
import org.junit.Test

class SyriaFormatTest {
    private val units = mapOf(
        R.string.syria_duration_days to "d",
        R.string.syria_duration_hours to "h",
        R.string.syria_duration_minutes to "m",
        R.string.syria_duration_seconds to "s",
    )

    private fun duration(seconds: Long) = SyriaFormat.duration(seconds) { res, value -> "$value${units.getValue(res)}" }

    private fun clock(seconds: Long) = SyriaFormat.clock(seconds) { days -> "${days}d" }

    @Test
    fun durationLeavesOutLeadingZeroUnits() {
        assertEquals("0s", duration(0))
        assertEquals("59s", duration(59))
        assertEquals("1m 0s", duration(60))
        assertEquals("1h 0m 0s", duration(3600))
        assertEquals("2h 5m 7s", duration(2 * 3600 + 5 * 60 + 7))
        assertEquals("1d 0h 0m 1s", duration(86_401))
        assertEquals("3d 4h 5m 6s", duration(3 * 86_400 + 4 * 3600 + 5 * 60 + 6))
    }

    @Test
    fun durationClampsNegative() {
        assertEquals("0s", duration(-5))
    }

    @Test
    fun clockAddsDaysPastOneDay() {
        assertEquals("00:00", clock(0))
        assertEquals("05:07", clock(5 * 60 + 7))
        assertEquals("2:05:07", clock(2 * 3600 + 5 * 60 + 7))
        assertEquals("1d 02:05:07", clock(86_400 + 2 * 3600 + 5 * 60 + 7))
    }

    @Test
    fun phoneUsesDoubleZeroAndGroups() {
        assertEquals("⁦00963 944 123 456⁩", SyriaFormat.phone("+963944123456"))
        assertEquals("⁦004915112345678⁩", SyriaFormat.phone("+4915112345678"))
        assertEquals("", SyriaFormat.phone(null))
        assertEquals("", SyriaFormat.phone(""))
    }

    @Test
    fun normalizePhoneInput() {
        assertEquals("+963944123456", SyriaFormat.normalizePhoneInput("944123456"))
        assertEquals("+963944123456", SyriaFormat.normalizePhoneInput("0944 123 456"))
        assertEquals("+963944123456", SyriaFormat.normalizePhoneInput("00963944123456"))
        assertEquals("+963944123456", SyriaFormat.normalizePhoneInput("+963 944 123 456"))
        assertEquals("+963944123456", SyriaFormat.normalizePhoneInput("963944123456"))
        assertEquals("+4915112345678", SyriaFormat.normalizePhoneInput("004915112345678"))
    }

    @Test
    fun normalizePhoneInputWithCountryCode() {
        assertEquals("+447911123456", SyriaFormat.normalizePhoneInput("07911 123456", "44"))
        assertEquals("+447911123456", SyriaFormat.normalizePhoneInput("7911123456", "+44"))
        // A number typed with + or 00 keeps its own code.
        assertEquals("+963944123456", SyriaFormat.normalizePhoneInput("+963944123456", "44"))
        assertEquals("+963944123456", SyriaFormat.normalizePhoneInput("00963944123456", "44"))
        // An empty code field falls back to Syria.
        assertEquals("+963944123456", SyriaFormat.normalizePhoneInput("0944123456", ""))
    }
}
