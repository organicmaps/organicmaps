package app.organicmaps.widget

/** What the track recording widget shows while recording. */
internal data class TrackRecordingWidgetContent(
    val texts: Texts,
    val chronometerBase: Long,
    /** False until the track has a duration, otherwise the time would jump back. */
    val isTicking: Boolean,
    val createdAtMs: Long,
) {
    data class Texts(
        val distance: String,
        val ascent: String,
        val descent: String,
        val maxElevation: String,
        val minElevation: String,
    )

    // The chronometer ticks by itself, a new base alone needs no update.
    fun needsPush(previous: TrackRecordingWidgetContent?): Boolean =
        previous == null || isTicking != previous.isTicking || texts != previous.texts

    companion object {
        const val MIN_PUSH_INTERVAL_MS = 10_000L

        // Texts change every few meters, so update at most once per interval unless ticking starts or stops.
        fun formatDelayMs(previous: TrackRecordingWidgetContent?, durationSec: Double, elapsedRealtimeMs: Long): Long {
            if (previous == null || previous.isTicking != (durationSec > 0)) {
                return 0
            }
            return (previous.createdAtMs + MIN_PUSH_INTERVAL_MS - elapsedRealtimeMs).coerceAtLeast(0)
        }

        fun create(
            texts: Texts,
            durationSec: Double,
            sampleTimeMs: Long,
            elapsedRealtimeMs: Long,
        ): TrackRecordingWidgetContent {
            val chronometerBase = sampleTimeMs - (durationSec * 1000).toLong()
            return TrackRecordingWidgetContent(texts, chronometerBase, durationSec > 0, elapsedRealtimeMs)
        }
    }
}
