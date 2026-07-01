package app.organicmaps.widget.placepage

import android.content.Context
import android.graphics.drawable.Drawable
import androidx.annotation.StringRes
import app.organicmaps.R
import app.organicmaps.sdk.bookmarks.data.BookmarkCategory
import app.organicmaps.sdk.bookmarks.data.BookmarkInfo
import app.organicmaps.sdk.bookmarks.data.BookmarkManager
import app.organicmaps.sdk.bookmarks.data.Icon
import app.organicmaps.sdk.bookmarks.data.Track
import app.organicmaps.utils.Graphics

/**
 * Uniform view over the two things the bookmark editor edits: a bookmark (icon + color)
 * or a track (color only). Encapsulates model access, save/delete flows and swatch drawing
 * so the fragment can drop its `when (type)` branches.
 */
internal sealed interface EditTarget {
    val id: Long
    val name: String
    val description: String

    @get:StringRes
    val toolbarTitleRes: Int
    var color: Int

    fun buildSwatch(context: Context): Drawable

    fun isNewlyCreated(unknownPlaceName: String): Boolean

    fun save(newName: String, newDescription: String, category: BookmarkCategory): Boolean

    fun delete()
}

internal class BookmarkEditTarget(private val info: BookmarkInfo) : EditTarget {
    override val id: Long get() = info.bookmarkId
    override val name: String get() = info.name
    override val description: String get() = info.description
    override val toolbarTitleRes: Int = R.string.placepage_edit_bookmark_button
    override var color: Int = info.icon.argb()

    override fun buildSwatch(context: Context): Drawable = Graphics.drawCircleAndImage(
        color,
        R.dimen.track_circle_size,
        info.icon.resId,
        R.dimen.bookmark_icon_size,
        context,
    )

    override fun isNewlyCreated(unknownPlaceName: String): Boolean = info.name == unknownPlaceName

    override fun save(newName: String, newDescription: String, category: BookmarkCategory): Boolean {
        val movedFromCategory = info.categoryId != category.id
        if (movedFromCategory) info.changeCategory(category.id)
        info.update(newName, Icon(color, info.icon.type), newDescription)
        return movedFromCategory
    }

    override fun delete() {
        BookmarkManager.INSTANCE.deleteBookmark(info.bookmarkId)
    }
}

internal class TrackEditTarget(private val data: Track) : EditTarget {
    override val id: Long get() = data.trackId
    override val name: String get() = data.name
    override val description: String get() = data.description
    override val toolbarTitleRes: Int = R.string.edit_track
    override var color: Int = data.color

    override fun buildSwatch(context: Context): Drawable = Graphics.drawCircle(
        color,
        R.dimen.track_circle_size,
        context.resources,
    )

    override fun isNewlyCreated(unknownPlaceName: String): Boolean = false

    override fun save(newName: String, newDescription: String, category: BookmarkCategory): Boolean {
        val movedFromCategory = data.categoryId != category.id
        if (movedFromCategory) data.categoryId = category.id
        data.update(newName, color, newDescription)
        return movedFromCategory
    }

    override fun delete() {
        BookmarkManager.INSTANCE.deleteTrack(data.trackId)
    }
}
