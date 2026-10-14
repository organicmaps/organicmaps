package app.organicmaps.sdk.sync

import java.io.File

/** The native bookmarks core. Methods without suspend must be called on the main thread. */
interface CoreBookmarks {
    /** Runs [block] on the core thread that saves category files, after all saves queued so far. */
    suspend fun <T> onFileThread(block: () -> T): T

    /**
     * False while replacing a category would lose edits that core has not saved yet, or break a screen that holds ids
     * of its bookmarks and tracks, like a bookmarks screen or the Place Page.
     */
    fun canApplyLocalChanges(): Boolean

    /**
     * Reloads the category file. Returns once core has a category for the file: a new file is awaited, an already
     * loaded one may still show its previous version for a moment.
     */
    suspend fun reloadAndAwait(file: File)

    fun moveToTrash(file: File): Boolean

    fun categoryName(file: File): String?

    fun renameCategory(file: File, newName: String)
}
