package app.organicmaps.sdk.sync

import androidx.annotation.Keep

/** Content hashes that the core can compute for local files. Ordinals match the core enum. */
@Keep
enum class HashAlgorithm {
    MD5,
    BLAKE3,
}

/**
 * A file in the cloud. [version] changes with every content change of the file [id]. [hash] is the content hash in
 * [CloudStore.hashAlgorithm], if the store reports hashes. [createdTimeMs] is milliseconds since the epoch.
 */
@Keep
data class CloudFile(
    @JvmField val id: String,
    @JvmField val name: String,
    @JvmField val version: String,
    @JvmField val hash: String?,
    @JvmField val createdTimeMs: Long,
)

sealed interface SyncError {
    data object AuthRequired : SyncError

    data object QuotaExceeded : SyncError

    /** Network problems, rate limits and server errors that a later retry may fix. */
    data object Transient : SyncError

    data object Cancelled : SyncError

    data class Other(val message: String) : SyncError

    companion object {
        // The order of the core enum.
        fun fromCode(code: Int, message: String): SyncError = when (code) {
            0 -> AuthRequired
            1 -> QuotaExceeded
            2 -> Transient
            3 -> Cancelled
            else -> Other(message)
        }
    }
}

@Keep
class CloudException(val error: SyncError, cause: Throwable? = null) : Exception(error.toString(), cause) {
    /** The core enum value, read by the core. */
    fun errorCode(): Int = when (error) {
        SyncError.AuthRequired -> 0
        SyncError.QuotaExceeded -> 1
        SyncError.Transient -> 2
        SyncError.Cancelled -> 3
        is SyncError.Other -> 4
    }

    /** Shown to the user for [SyncError.Other]. */
    fun errorMessage(): String = (error as? SyncError.Other)?.message ?: error.toString()

    companion object {
        @JvmStatic
        fun fromNative(code: Int, message: String) = CloudException(SyncError.fromCode(code, message))
    }
}

/**
 * Storage of category files for the core sync engine, which calls it on the thread of [BookmarksSyncEngine.runPass].
 * Calls block and throw [CloudException]. The engine chooses what to do; a store only moves files.
 */
@Keep
interface CloudStore {
    /** The algorithm of [CloudFile.hash], null if the store reports no hashes. */
    val hashAlgorithm: HashAlgorithm?

    /** The top-level files, without folders and trashed files. The listing can lag behind. */
    fun list(): Array<CloudFile>

    /** null once the file is deleted or trashed, even while a listing still shows it. */
    fun stat(id: String): CloudFile?

    /** Creates an empty file named ".generation". @return its id. */
    fun createGeneration(): String

    /** @return false if the file is gone. */
    fun download(id: String, destPath: String): Boolean

    /** Creates a file when [existingId] is null, otherwise replaces its content. */
    fun upload(name: String, srcPath: String, existingId: String?): CloudFile

    /** Moves the file to a trash that [purgeTrash] cleans up. */
    fun moveToTrash(id: String)

    fun purgeTrash(trashedBeforeMs: Long)

    /**
     * Deletes [generationId] first, as a listing can lag behind and miss it, then the other generations and files, so
     * other devices never take a partly deleted cloud for deletions.
     */
    fun deleteAll(generationId: String?)

    /** Aborts the calls of a running pass, which then fails; other calls continue. */
    fun cancelPass()
}
