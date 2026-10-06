package app.organicmaps.sdk.bookmarks.data

import android.os.Parcel
import android.os.Parcelable
import androidx.annotation.ColorInt
import androidx.annotation.Keep
import androidx.core.os.ParcelCompat

@Keep
data class TrackSelectionCandidate(
    val trackId: Long,
    val title: String,
    @get:ColorInt val color: Int,
    val isSelected: Boolean,
) : Parcelable {
    override fun describeContents(): Int = 0

    override fun writeToParcel(dest: Parcel, flags: Int) {
        dest.writeLong(trackId)
        dest.writeString(title)
        dest.writeInt(color)
        ParcelCompat.writeBoolean(dest, isSelected)
    }

    companion object {
        @JvmField
        val CREATOR: Parcelable.Creator<TrackSelectionCandidate> =
            object : Parcelable.Creator<TrackSelectionCandidate> {
                override fun createFromParcel(source: Parcel): TrackSelectionCandidate {
                    val trackId = source.readLong()
                    val title = source.readString()!!
                    val color = source.readInt()
                    val isSelected = ParcelCompat.readBoolean(source)
                    return TrackSelectionCandidate(trackId, title, color, isSelected)
                }

                override fun newArray(size: Int): Array<TrackSelectionCandidate?> = arrayOfNulls(size)
            }
    }
}
