package app.organicmaps.sdk.sync.testing

import app.organicmaps.sdk.sync.CloudException
import app.organicmaps.sdk.sync.CloudFile
import app.organicmaps.sdk.sync.CloudListing
import app.organicmaps.sdk.sync.CloudStore
import app.organicmaps.sdk.sync.SyncError
import java.io.File
import java.security.MessageDigest
import kotlinx.coroutines.CompletableDeferred

/** In-memory cloud with counters and failure injection. Files belong to the generation they were created in. */
class FakeCloudStore(private val clock: () -> Long) : CloudStore {
    class Entry(val name: String, var content: ByteArray, val createdTime: String, val generation: String?) {
        var trashedAt: Long? = null
    }

    val files = linkedMapOf<String, Entry>()
    var writes = 0
    var purges = 0
    var listError: SyncError? = null
    var deleteAllError: SyncError? = null
    var generation: String? = null
    var failingUpload: Int? = null
    var beforeCurrentMd5: ((String) -> Unit)? = null
    var beforeDownload: ((String) -> Unit)? = null

    /** Names and ids that [list] misses, like a Drive listing that lags behind. */
    val hiddenFromListing = mutableSetOf<String>()
    val hiddenIdsFromListing = mutableSetOf<String>()

    /** Suspends uploads until it completes. */
    var uploadGate: CompletableDeferred<Unit>? = null
    private var uploads = 0
    private var nextId = 1

    /** Adds a file from another device, which starts a generation if there is none. */
    fun put(name: String, text: String, createdTime: String = "2026-10-01T10:00:00.000Z"): String {
        val generation = generation ?: "gen${nextId++}".also { generation = it }
        val id = "id${nextId++}"
        files[id] = Entry(name, text.toByteArray(), createdTime, generation)
        return id
    }

    fun content(name: String) = live().single { it.value.name == name }.value.content.decodeToString()

    fun names() = live().map { it.value.name }.sorted()

    fun trashed() = files.values.filter { it.trashedAt != null }

    private fun live() = files.entries.filter { it.value.trashedAt == null }

    override suspend fun list(): CloudListing {
        listError?.let { throw CloudException(it) }
        val generation = generation ?: return CloudListing(emptyList(), null)
        val files = live()
            .filter { it.value.generation == generation }
            .filter { it.value.name !in hiddenFromListing && it.key !in hiddenIdsFromListing }
            .map { (id, entry) -> CloudFile(id, entry.name, md5(entry.content), entry.createdTime) }
        return CloudListing(files, generation)
    }

    override suspend fun createGeneration() = "gen${nextId++}".also { generation = it }

    override suspend fun currentMd5(id: String): String? {
        beforeCurrentMd5?.invoke(id)
        return files[id]?.takeIf { it.trashedAt == null }?.let { md5(it.content) }
    }

    override suspend fun download(id: String, dest: File): Boolean {
        beforeDownload?.invoke(id)
        val entry = files[id] ?: return false
        dest.writeBytes(entry.content)
        return true
    }

    override suspend fun upload(name: String, src: File, existingId: String?): CloudFile {
        uploadGate?.await()
        if (++uploads == failingUpload) throw CloudException(SyncError.Transient)
        ++writes
        val id = existingId ?: "id${nextId++}"
        val entry = files[id]?.also { it.content = src.readBytes() }
            ?: Entry(name, src.readBytes(), "2026-10-14T10:00:00.000Z", generation)
        files[id] = entry
        return CloudFile(id, entry.name, md5(entry.content), entry.createdTime)
    }

    override suspend fun moveToTrash(id: String, trashFolderId: String?): String {
        ++writes
        files.getValue(id).trashedAt = clock()
        return trashFolderId ?: TRASH_FOLDER_ID
    }

    override suspend fun purgeTrash(trashFolderId: String, trashedBefore: Long) {
        ++purges
        val old = files.filterValues { (it.trashedAt ?: Long.MAX_VALUE) < trashedBefore }.keys
        writes += old.size
        files.keys.removeAll(old)
    }

    override suspend fun deleteAll() {
        deleteAllError?.let { throw CloudException(it) }
        ++writes
        generation = null
        files.clear()
    }

    companion object {
        const val TRASH_FOLDER_ID = "trash"

        fun md5(bytes: ByteArray) = MessageDigest.getInstance("MD5").digest(bytes).joinToString("") {
            "%02x".format(it)
        }
    }
}
