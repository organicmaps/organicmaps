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
    suspend fun snapshot(): List<LocalFile> = withContext(io) {
        listCategoryFiles().map { LocalFile(it.name, Md5.of(it)) }
    }

    /** False while replacing a category would drop its unsaved edits or break the screen that shows it. */
    suspend fun canApplyChanges(): Boolean = withContext(main) { core.canApplyLocalChanges() }

    fun newTempFile(): File {
        tmpDir.mkdirs()
        return File.createTempFile("download", ".part", tmpDir)
    }

    fun cleanTemp() {
        tmpDir.listFiles()?.forEach { it.delete() }
    }

    // Each replacement re-checks that core allows it right before touching the file, and runs the file change and the
    // reload as one step: a rename without the reload would leave core with a stale category that its next save
    // writes back over the new version.

    /** Moves [tmp] over the file if its MD5 is still [expectedMd5] (null: the file must not exist), then reloads. */
    suspend fun install(name: String, tmp: File, expectedMd5: String?): Boolean {
        if (!canApplyChanges()) return false
        val target = file(name)
        return withContext(NonCancellable) {
            val installed = core.onFileThread {
                if (matches(target, expectedMd5)) {
                    move(tmp, target)
                    true
                } else {
                    false
                }
            }
            if (installed) withContext(main) { core.reloadAndAwait(target) }
            installed
        }
    }

    /**
     * Keeps the local version as "Name (device).kml" with its category named "Name (device)" and moves [tmp] over the
     * file, both in one File thread task, so an interrupted pass never leaves a copy without the replacement. Nothing
     * changes when the file is no longer [expectedMd5]. [reserved] names are taken in the cloud. @return the copy name.
     */
    suspend fun resolveConflict(
        name: String,
        tmp: File,
        expectedMd5: String,
        device: String,
        reserved: Set<String>,
    ): String? {
        if (!canApplyChanges()) return null
        val target = file(name)
        return withContext(NonCancellable) {
            val copyName = core.onFileThread {
                if (!matches(target, expectedMd5)) return@onFileThread null
                val copyName = FileNames.conflictName(name, device, existingNames() + reserved)
                // Copied to a temporary file first, so a copy is either complete or absent.
                val copy = newTempFile()
                target.copyTo(copy, overwrite = true)
                move(copy, file(copyName))
                move(tmp, target)
                copyName
            } ?: return@withContext null
            withContext(main) {
                // Until the reload below, the category of the file still holds the local version.
                val sourceName = core.categoryName(target) ?: name.removeSuffix(FileNames.KML_EXTENSION)
                val copy = file(copyName)
                core.reloadAndAwait(copy)
                core.renameCategory(copy, "$sourceName ($device)")
                core.reloadAndAwait(target)
            }
            copyName
        }
    }

    /**
     * Moves [tmp] to a free name derived from [name], never [name] itself, and loads it as a new category. [reserved]
     * names are taken in the cloud.
     */
    suspend fun installUnique(name: String, tmp: File, reserved: Set<String>): String? {
        if (!canApplyChanges()) return null
        return withContext(NonCancellable) {
            val uniqueName = core.onFileThread {
                FileNames.uniqueName(name, existingNames() + reserved + name).also { move(tmp, file(it)) }
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

    // Core creates the directory only when it saves a category for the first time.
    private fun move(from: File, to: File) {
        dir.mkdirs()
        check(from.renameTo(to)) { "Can't move $from to $to" }
    }

    // Core writes files as "<name>.kml.tmp<thread>" and renames them, so those never match.
    private fun listCategoryFiles(): List<File> {
        if (!dir.exists()) return emptyList()
        // An unreadable directory must not look empty: that would trash every list in the cloud.
        val files = dir.listFiles { file -> file.isFile && file.name.endsWith(FileNames.KML_EXTENSION) }
            ?: throw IOException("Can't list $dir")
        return files.sortedBy { it.name }
    }

    private fun matches(file: File, expectedMd5: String?) =
        if (expectedMd5 == null) !file.exists() else file.isFile && Md5.of(file) == expectedMd5
}
