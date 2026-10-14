package app.organicmaps.settings

import android.content.Context
import android.util.AttributeSet
import androidx.core.view.isInvisible
import androidx.preference.Preference
import androidx.preference.PreferenceViewHolder
import app.organicmaps.R

/** The sync status, with a spinner while a pass runs, a note under it and the "Sync now" button. */
class BookmarksSyncStatusPreference(context: Context, attrs: AttributeSet?) : Preference(context, attrs) {
    var isSyncing = false
        set(value) {
            if (field == value) return
            field = value
            notifyChanged()
        }

    var onSyncNow: (() -> Unit)? = null

    init {
        layoutResource = R.layout.preference_bookmarks_sync_status
        isSelectable = false
    }

    override fun onBindViewHolder(holder: PreferenceViewHolder) {
        super.onBindViewHolder(holder)
        // Invisible rather than gone, so the row keeps its height when the spinner appears.
        holder.findViewById(R.id.progress).isInvisible = !isSyncing
        holder.findViewById(R.id.sync_now).apply {
            setOnClickListener { onSyncNow?.invoke() }
            // Another tap would restart the running pass. Faded rather than disabled: the disabled background of this
            // button style is plain grey.
            isClickable = !isSyncing
            alpha = if (isSyncing) SYNCING_ALPHA else 1f
        }
    }

    private companion object {
        const val SYNCING_ALPHA = 0.5f
    }
}
