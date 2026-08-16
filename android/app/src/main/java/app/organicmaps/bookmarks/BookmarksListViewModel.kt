package app.organicmaps.bookmarks

import android.os.Bundle
import androidx.core.os.bundleOf
import androidx.lifecycle.SavedStateHandle
import androidx.lifecycle.ViewModel

/**
 * Holds what [BookmarksListFragment] points its actions at: the multi-selection, and the row a single-item
 * fragment was opened for.
 *
 * A ViewModel alone would not survive an activity the system destroys while the process lives ("Don't keep
 * activities", memory pressure): that one gets its saved state back and its ViewModel cleared. Hence
 * [SavedStateHandle], which covers both cases and serializes only when the state is saved, not on every toggle.
 *
 * Process death is not covered on purpose: the activity restarts the core through SplashActivity and never
 * receives its saved state.
 */
class BookmarksListViewModel(handle: SavedStateHandle) : ViewModel() {

    var isSelectionMode: Boolean = false
        private set

    // LinkedHashSet: the first selected bookmark - or track, if no bookmark is selected - defines the color the
    // picker opens with.
    val bookmarkIds: MutableSet<Long> = LinkedHashSet()
    val trackIds: MutableSet<Long> = LinkedHashSet()

    val selectedCount: Int
        get() = bookmarkIds.size + trackIds.size

    /**
     * The row a context menu, a confirmation or the color picker was opened for: those come back on their own
     * after a recreation, none of them carrying what it acts on. The editor is not among them - it is handed the
     * ids as arguments. A bookmark and a track can share an id, hence the type.
     */
    var focusedItemId: Long = NO_ITEM
        private set
    var focusedItemType: Int = NO_ITEM_TYPE
        private set

    private var loadGeneration = NO_GENERATION

    init {
        handle.get<Bundle>(STATE_KEY)?.let { saved ->
            isSelectionMode = saved.getBoolean(EXTRA_SELECTION_MODE)
            loadGeneration = saved.getInt(EXTRA_LOAD_GENERATION, NO_GENERATION)
            focusedItemId = saved.getLong(EXTRA_FOCUSED_ITEM_ID, NO_ITEM)
            focusedItemType = saved.getInt(EXTRA_FOCUSED_ITEM_TYPE, NO_ITEM_TYPE)
            saved.getLongArray(EXTRA_BOOKMARK_IDS)?.forEach { bookmarkIds.add(it) }
            saved.getLongArray(EXTRA_TRACK_IDS)?.forEach { trackIds.add(it) }
        }
        handle.setSavedStateProvider(STATE_KEY) {
            bundleOf(
                EXTRA_SELECTION_MODE to isSelectionMode,
                EXTRA_LOAD_GENERATION to loadGeneration,
                EXTRA_FOCUSED_ITEM_ID to focusedItemId,
                EXTRA_FOCUSED_ITEM_TYPE to focusedItemType,
                EXTRA_BOOKMARK_IDS to bookmarkIds.toLongArray(),
                EXTRA_TRACK_IDS to trackIds.toLongArray(),
            )
        }
    }

    fun hasFocusedItem() = focusedItemId != NO_ITEM

    fun focusItem(id: Long, type: Int) {
        focusedItemId = id
        focusedItemType = type
    }

    fun clearFocusedItem() {
        focusedItemId = NO_ITEM
        focusedItemType = NO_ITEM_TYPE
    }

    /**
     * Whether the state was built on an older load, which makes every id in it meaningless: a load renumbers
     * the category, so a surviving id addresses a different row rather than none.
     *
     * [dropState] redraws nothing, so it is only for a caller with no view yet; one with a live view has to go
     * through the fragment's own teardown instead.
     */
    fun isFromEarlierLoad(generation: Int) = loadGeneration != generation

    fun dropState() {
        setMode(false)
        clearSelection()
        clearFocusedItem()
    }

    fun bindToLoad(generation: Int) {
        loadGeneration = generation
    }

    /**
     * @return true when the mode actually changed, in which case the selection was cleared.
     */
    fun setMode(enabled: Boolean): Boolean {
        if (isSelectionMode == enabled) return false

        isSelectionMode = enabled
        clearSelection()
        return true
    }

    fun clearSelection() {
        bookmarkIds.clear()
        trackIds.clear()
    }

    companion object {
        private const val STATE_KEY = "bookmarks_list_state"
        private const val EXTRA_SELECTION_MODE = "selection_mode"
        private const val EXTRA_LOAD_GENERATION = "load_generation"
        private const val EXTRA_FOCUSED_ITEM_ID = "focused_item_id"
        private const val EXTRA_FOCUSED_ITEM_TYPE = "focused_item_type"
        private const val EXTRA_BOOKMARK_IDS = "selected_bookmark_ids"
        private const val EXTRA_TRACK_IDS = "selected_track_ids"

        private const val NO_GENERATION = -1
        private const val NO_ITEM = -1L
        private const val NO_ITEM_TYPE = -1
    }
}
