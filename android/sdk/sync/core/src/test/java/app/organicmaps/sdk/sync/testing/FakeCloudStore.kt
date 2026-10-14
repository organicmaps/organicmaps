package app.organicmaps.sdk.sync.testing

import app.organicmaps.sdk.sync.CloudException
import app.organicmaps.sdk.sync.CloudFile
import app.organicmaps.sdk.sync.CloudListing
import app.organicmaps.sdk.sync.CloudStore
import app.organicmaps.sdk.sync.SyncError
import java.io.File
import java.security.MessageDigest

/** In-memory cloud with counters and failure injection. */
class FakeCloudStore(var accountId: String = "account-1", private val clock: () -> Long) : CloudStore {
    class Entry(val name: String, var content: ByteArray, val createdTime: String) {
        var trashedAt: Long? = null
    }

    val files = linkedMapOf<String, Entry>()
    var writes = 0
    var listError: SyncError? = null
    var deleteAllError: SyncError? = null
    var resetStartedAt: Long? = null
    var failingUpload: Int? = null
    var beforeCurrentMd5: ((String) -> Unit)? = null
    private var uploads = 0
    private var nextId = 1

    fun put(name: String, text: String, createdTime: String = "2026-10-01T10:00:00.000Z"): String {
        val id = "id${nextId++}"
        files[id] = Entry(name, text.toByteArray(), createdTime)
        return id
    }

    fun content(name: String) = live().single { it.value.name == name }.value.content.decodeToString()

    fun names() = live().map { it.value.name }.sorted()

    fun trashed() = files.values.filter { it.trashedAt != null }

    private fun live() = files.entries.filter { it.value.trashedAt == null }

    override suspend fun account() = accountId

    override suspend fun list(): CloudListing {
        listError?.let { throw CloudException(it) }
        val files = live().map { (id, entry) -> CloudFile(id, entry.name, md5(entry.content), entry.createdTime) }
        return CloudListing(files, resetStartedAt)
    }

    override suspend fun currentMd5(id: String): String? {
        beforeCurrentMd5?.invoke(id)
        return files[id]?.takeIf { it.trashedAt == null }?.let { md5(it.content) }
    }

    override suspend fun download(id: String, dest: File) = dest.writeBytes(files.getValue(id).content)

    override suspend fun upload(name: String, src: File, existingId: String?): CloudFile {
        if (++uploads == failingUpload) throw CloudException(SyncError.Transient)
        ++writes
        val id = existingId ?: "id${nextId++}"
        val entry =
            files[id]?.also { it.content = src.readBytes() } ?: Entry(name, src.readBytes(), "2026-10-14T10:00:00.000Z")
        files[id] = entry
        return CloudFile(id, entry.name, md5(entry.content), entry.createdTime)
    }

    override suspend fun moveToTrash(id: String, trashFolderId: String?): String {
        ++writes
        files.getValue(id).trashedAt = clock()
        return trashFolderId ?: TRASH_FOLDER_ID
    }

    override suspend fun purgeTrash(trashFolderId: String, trashedBefore: Long) {
        val old = files.filterValues { (it.trashedAt ?: Long.MAX_VALUE) < trashedBefore }.keys
        writes += old.size
        files.keys.removeAll(old)
    }

    override suspend fun deleteAll() {
        deleteAllError?.let { throw CloudException(it) }
        ++writes
        files.clear()
    }

    companion object {
        const val TRASH_FOLDER_ID = "trash"

        fun md5(bytes: ByteArray) = MessageDigest.getInstance("MD5").digest(bytes).joinToString("") {
            "%02x".format(it)
        }
    }
}
