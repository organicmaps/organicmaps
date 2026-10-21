package app.organicmaps.widget

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class TrackRecordingWidgetContentTest {
    private val texts = TrackRecordingWidgetContent.Texts("1 km", "10 m", "5 m", "300 m", "100 m")

    @Test
    fun chronometerBaseIsSampleTimeMinusDuration() {
        assertEquals(34_500L, TrackRecordingWidgetContent.create(texts, 65.5, 100_000L, 100_000L).chronometerBase)
    }

    @Test
    fun deferredContentKeepsTheClock() {
        assertEquals(40_000L, TrackRecordingWidgetContent.create(texts, 64.0, 104_000L, 110_000L).chronometerBase)
    }

    @Test
    fun firstContentNeedsPush() {
        assertTrue(TrackRecordingWidgetContent.create(texts, 0.0, 0L, 0L).needsPush(null))
    }

    @Test
    fun baseOnlyChangeDoesNotNeedPush() {
        val previous = TrackRecordingWidgetContent.create(texts, 60.0, 100_000L, 100_000L)
        assertFalse(TrackRecordingWidgetContent.create(texts, 61.0, 101_500L, 101_500L).needsPush(previous))
    }

    @Test
    fun zeroDurationDoesNotTick() {
        assertFalse(TrackRecordingWidgetContent.create(texts, 0.0, 100_000L, 100_000L).isTicking)
        assertTrue(TrackRecordingWidgetContent.create(texts, 1.0, 100_000L, 100_000L).isTicking)
    }

    @Test
    fun startOfTickingNeedsPush() {
        val previous = TrackRecordingWidgetContent.create(texts, 0.0, 100_000L, 100_000L)
        assertTrue(TrackRecordingWidgetContent.create(texts, 1.0, 101_000L, 101_000L).needsPush(previous))
    }

    @Test
    fun anyTextChangeNeedsPush() {
        val previous = TrackRecordingWidgetContent.create(texts, 60.0, 100_000L, 100_000L)
        val changed =
            listOf(
                texts.copy(distance = "1.1 km"),
                texts.copy(ascent = "11 m"),
                texts.copy(descent = "6 m"),
                texts.copy(maxElevation = "301 m"),
                texts.copy(minElevation = "99 m"),
            )
        for (current in changed) {
            assertTrue(
                current.toString(),
                TrackRecordingWidgetContent.create(current, 60.0, 100_000L, 100_000L).needsPush(previous),
            )
        }
    }

    @Test
    fun firstStatisticsAreFormattedNow() {
        assertEquals(0L, TrackRecordingWidgetContent.formatDelayMs(null, 0.0, 100_000L))
    }

    @Test
    fun statisticsWaitForTheInterval() {
        val previous = TrackRecordingWidgetContent.create(texts, 60.0, 100_000L, 100_000L)
        val interval = TrackRecordingWidgetContent.MIN_PUSH_INTERVAL_MS

        assertEquals(interval - 4_000L, TrackRecordingWidgetContent.formatDelayMs(previous, 64.0, 104_000L))
        assertEquals(0L, TrackRecordingWidgetContent.formatDelayMs(previous, 70.0, 100_000L + interval))
    }

    @Test
    fun startOfTickingIsFormattedNow() {
        val previous = TrackRecordingWidgetContent.create(texts, 0.0, 100_000L, 100_000L)
        assertEquals(0L, TrackRecordingWidgetContent.formatDelayMs(previous, 1.0, 101_000L))
    }
}
