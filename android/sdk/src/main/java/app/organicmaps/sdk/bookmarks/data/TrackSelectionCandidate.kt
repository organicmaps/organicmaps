package app.organicmaps.sdk.bookmarks.data

import androidx.annotation.ColorInt
import androidx.annotation.Keep

@Keep
data class TrackSelectionCandidate(
    val trackId: Long,
    val title: String,
    @get:ColorInt val color: Int,
    val isSelected: Boolean,
)
