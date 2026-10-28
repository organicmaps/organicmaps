package app.organicmaps.wear

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

class RemainingTimeFormatterTest {
    private fun format(seconds: Int?) = RemainingTimeFormatter.format(seconds, "h", "min")

    @Test
    fun minimumOneMinuteAndHalfUpRounding() {
        assertEquals("1\u00A0min", format(0))
        assertEquals("1\u00A0min", format(29))
        assertEquals("1\u00A0min", format(59))
        assertEquals("1\u00A0min", format(89))
        assertEquals("2\u00A0min", format(90))
    }

    @Test
    fun roundIntoHoursAndOmitZeroMinuteSuffix() {
        assertEquals("1\u00A0h", format(3599))
        assertEquals("1\u00A0h", format(3600))
        assertEquals("1\u00A0h\u00A01\u00A0min", format(3650))
        assertEquals("2\u00A0h", format(7200))
    }

    @Test
    fun missingOrInvalidTimeIsHidden() {
        assertNull(format(null))
        assertNull(format(-1))
    }
}
