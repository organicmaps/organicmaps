package app.organicmaps.sdk.sync.testing

import app.organicmaps.sdk.sync.CoreBookmarks
import java.io.File

/** Runs file-thread blocks inline and keeps category names by file name. */
class FakeCoreBookmarks : CoreBookmarks {
    var canApply = true
    val emptyCategories = mutableSetOf<String>()
    val names = mutableMapOf<String, String>()
    val reloaded = mutableListOf<File>()
    val trashed = mutableListOf<File>()
    var afterFileThread: (() -> Unit)? = null

    override suspend fun <T> onFileThread(block: () -> T): T = block().also { afterFileThread?.invoke() }

    override fun canApplyLocalChanges() = canApply

    override fun isCategoryEmpty(file: File) = file.name in emptyCategories

    override suspend fun reloadAndAwait(file: File) {
        reloaded += file
        names.getOrPut(file.name) { "loaded ${file.name}" }
    }

    override fun moveToTrash(file: File): Boolean {
        trashed += file
        return file.delete()
    }

    override fun categoryName(file: File) = names[file.name]

    override fun renameCategory(file: File, newName: String) {
        names[file.name] = newName
    }
}
