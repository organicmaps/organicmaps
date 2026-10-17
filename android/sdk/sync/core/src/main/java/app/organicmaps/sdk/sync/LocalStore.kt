package app.organicmaps.sdk.sync

import java.io.File
import java.io.IOException
import kotlinx.coroutines.CoroutineDispatcher
import kotlinx.coroutines.NonCancellable
import kotlinx.coroutines.withContext

/**
 * The flat directory of category files that core loads. Files are replaced on the core File thread, so a queued
 * core save can't overwrite them afterwards, and every replacement re-checks the MD5 the plan was based on.
 */
class LocalStore(
    private val dir: File,
    private val tmpDir: File,
    private val core: CoreBookmarks,
    private val main: CoroutineDispatcher,
    private val io: CoroutineDispatcher,
) {
    suspend fun snapshot(): List<LocalFile> {
        val hashes = withContext(io) { listCategoryFiles().map { it to Md5.of(it) } }
        return withContext(main) { hashes.map { (file, md5) -> LocalFile(file.name, md5, core.isCategoryEmpty(file)) } }
    }

    /** False while core defers saving, so replacing a category would drop its unsaved edits. */
    suspend fun canApplyChanges(): Boolean = withContext(main) { core.canApplyLocalChanges() }

    fun newTempFile(): File {
        tmpDir.mkdirs()
        return File.createTempFile("download", ".part", tmpDir)
    }

    fun cleanTemp() {
        tmpDir.listFiles()?.forEach { it.delete() }
    }

    // Each replacement re-checks that core is not deferring saves right before touching the file, and runs the
    // file change and the reload as one step: a rename without the reload would leave core with a stale category
    // that its next save writes back over the new version.

    /** Moves [tmp] over the file if its MD5 is still [expectedMd5] (null: the file must not exist), then reloads. */
    suspend fun install(name: String, tmp: File, expectedMd5: String?): Boolean {
        if (!canApplyChanges()) return false
        val target = file(name)
        return withContext(NonCancellable) {
            val installed = core.onFileThread { matches(target, expectedMd5) && tmp.renameTo(target) }
            if (installed) withContext(main) { core.reloadAndAwait(target) }
            installed
        }
    }

    /** Copies the file to "Name (device).kml" and names the copied category "Name (device)". @return the copy name. */
    suspend fun copyForConflict(name: String, device: String): String? {
        if (!canApplyChanges()) return null
        val source = file(name)
        return withContext(NonCancellable) {
            val copyName = core.onFileThread {
                FileNames.conflictName(name, device, existingNames()).also { source.copyTo(file(it)) }
            }
            withContext(main) {
                val sourceName = core.categoryName(source) ?: name.removeSuffix(KML_EXTENSION)
                val copy = file(copyName)
                core.reloadAndAwait(copy)
                core.renameCategory(copy, "$sourceName ($device)")
            }
            copyName
        }
    }

    /** Moves [tmp] to a free name derived from [name], never [name] itself, and loads it as a new category. */
    suspend fun installUnique(name: String, tmp: File): String? {
        if (!canApplyChanges()) return null
        return withContext(NonCancellable) {
            val uniqueName = core.onFileThread {
                FileNames.uniqueName(name, existingNames() + name).also {
                    check(tmp.renameTo(file(it))) { "Can't move $tmp" }
                }
            }
            withContext(main) { core.reloadAndAwait(file(uniqueName)) }
            uniqueName
        }
    }

    suspend fun trash(name: String, expectedMd5: String): Boolean {
        if (!canApplyChanges()) return false
        val target = file(name)
        if (!core.onFileThread { matches(target, expectedMd5) }) return false
        return withContext(main) { core.moveToTrash(target) }
    }

    fun file(name: String) = File(dir, name)

    private fun existingNames(): Set<String> = dir.list()?.toSet().orEmpty()

    // Core writes files as "<name>.kml.tmp<thread>" and renames them, so those never match.
    private fun listCategoryFiles(): List<File> {
        if (!dir.exists()) return emptyList()
        // An unreadable directory must not look empty: that would trash every list in the cloud.
        val files =
            dir.listFiles { file -> file.isFile && !file.name.startsWith(".") && file.name.endsWith(KML_EXTENSION) }
                ?: throw IOException("Can't list $dir")
        return files.sortedBy { it.name }
    }

    private fun matches(file: File, expectedMd5: String?) =
        if (expectedMd5 == null) !file.exists() else file.isFile && Md5.of(file) == expectedMd5

    private companion object {
        const val KML_EXTENSION = ".kml"
    }
}
