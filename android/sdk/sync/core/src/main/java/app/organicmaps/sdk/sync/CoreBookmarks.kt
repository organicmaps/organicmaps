package app.organicmaps.sdk.sync

import java.io.File

/** The native bookmarks core. Methods without suspend must be called on the main thread. */
interface CoreBookmarks {
    /** Runs [block] on the core thread that saves category files, after all saves queued so far. */
    suspend fun <T> onFileThread(block: () -> T): T

    /** False while core defers saving, for example when a bookmarks screen is open. */
    fun canApplyLocalChanges(): Boolean

    /** False when no category is loaded from the file. */
    fun isCategoryEmpty(file: File): Boolean

    /** Reloads the category file on the main thread and returns once core has loaded it. */
    suspend fun reloadAndAwait(file: File)

    fun moveToTrash(file: File): Boolean

    fun categoryName(file: File): String?

    fun renameCategory(file: File, newName: String)
}
