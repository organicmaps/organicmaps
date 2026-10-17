package app.organicmaps.widget

/** What the track recording widget shows while recording. */
internal data class TrackRecordingWidgetContent(
    val texts: Texts,
    val chronometerBase: Long,
    /**
     * The duration is zero until the track has two points, e.g. while waiting for a GPS fix. The time stands still
     * meanwhile, otherwise it would jump back when the first segment arrives.
     */
    val isTicking: Boolean,
    val createdAtMs: Long,
) {
    /** Formatted statistics of the recording. */
    data class Texts(
        val distance: String,
        val ascent: String,
        val descent: String,
        val maxElevation: String,
        val minElevation: String,
    )

    /** The chronometer ticks by itself, so only a change of the texts or of its running state is worth an update. */
    fun needsPush(previous: TrackRecordingWidgetContent?): Boolean =
        previous == null || isTicking != previous.isTicking || texts != previous.texts

    companion object {
        const val MIN_PUSH_INTERVAL_MS = 10_000L

        /**
         * How long to wait before formatting new statistics. The texts change every few meters, so the widget is
         * updated at most once per interval, except when the chronometer starts or stops.
         */
        fun formatDelayMs(previous: TrackRecordingWidgetContent?, durationSec: Double, elapsedRealtimeMs: Long): Long {
            if (previous == null || previous.isTicking != (durationSec > 0)) {
                return 0
            }
            return (previous.createdAtMs + MIN_PUSH_INTERVAL_MS - elapsedRealtimeMs).coerceAtLeast(0)
        }

        /** @param sampleTimeMs when the statistics arrived, which can be earlier than the content is created. */
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
