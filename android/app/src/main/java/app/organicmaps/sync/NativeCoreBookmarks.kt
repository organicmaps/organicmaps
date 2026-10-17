package app.organicmaps.sync

import app.organicmaps.sdk.bookmarks.data.BookmarkCategory
import app.organicmaps.sdk.bookmarks.data.BookmarkManager
import app.organicmaps.sdk.sync.CoreBookmarks
import app.organicmaps.sdk.util.concurrency.UiThread
import java.io.File
import kotlin.coroutines.resume
import kotlinx.coroutines.suspendCancellableCoroutine
import kotlinx.coroutines.withTimeoutOrNull

/** [CoreBookmarks] over the native BookmarkManager. */
class NativeCoreBookmarks : CoreBookmarks {
    private val manager = BookmarkManager.INSTANCE

    override suspend fun <T> onFileThread(block: () -> T): T = suspendCancellableCoroutine { continuation ->
        BookmarkManager.runOnFileThread { continuation.resumeWith(runCatching(block)) }
    }

    override fun canApplyLocalChanges() = manager.areNotificationsEnabled()

    override fun isCategoryEmpty(file: File) = category(file)?.size() == 0

    // On timeout the reload still completes later; only a conflict copy may keep the name core gave it.
    override suspend fun reloadAndAwait(file: File) {
        withTimeoutOrNull(RELOAD_TIMEOUT_MILLIS) { awaitReload(file) }
    }

    private suspend fun awaitReload(file: File) {
        suspendCancellableCoroutine { continuation ->
            val listener = object : BookmarkManager.BookmarksLoadingListener {
                override fun onBookmarksLoadingFinished() {
                    // Another load may finish first; wait until this file's category exists.
                    if (categoryId(file) == null || !continuation.isActive) return
                    // Called while BookmarkManager iterates its listeners, so unregister later.
                    UiThread.runLater { manager.removeLoadingListener(this) }
                    continuation.resume(Unit)
                }
            }
            manager.addLoadingListener(listener)
            continuation.invokeOnCancellation { UiThread.runLater { manager.removeLoadingListener(listener) } }
            manager.reloadCategoryFile(file.path)
        }
    }

    override fun moveToTrash(file: File) = categoryId(file)?.let { manager.moveCategoryToTrash(it) } ?: false

    override fun categoryName(file: File): String? = category(file)?.name

    override fun renameCategory(file: File, newName: String) {
        category(file)?.setName(newName)
    }

    private fun categoryId(file: File): Long? = manager.getCategoryIdByFile(file.path).takeIf { it != NO_CATEGORY }

    private fun category(file: File): BookmarkCategory? = categoryId(file)?.let { manager.getCategoryById(it) }

    private companion object {
        const val NO_CATEGORY = -1L
        const val RELOAD_TIMEOUT_MILLIS = 10_000L
    }
}
