package app.organicmaps.sdk.sync

import java.io.File

sealed interface SyncError {
    data object AuthRequired : SyncError

    data object QuotaExceeded : SyncError

    /** Network problems, rate limits and server errors that a later retry may fix. */
    data object Transient : SyncError

    data class Other(val message: String) : SyncError
}

/**
 * [generation] identifies the history of the cloud data: another account has another one, and deleting all data,
 * from the app or in the Drive settings, deletes it, so it is null until the next pass starts a new one.
 */
data class CloudListing(val files: List<CloudFile>, val generation: String?)

class CloudException(val error: SyncError, cause: Throwable? = null) : Exception(error.toString(), cause)

/** Cloud storage of category files. Every method throws [CloudException]. */
interface CloudStore {
    /**
     * The top-level `*.kml` files of the current generation: none without a generation, and none left from the
     * deleted data of an older one.
     */
    suspend fun list(): CloudListing

    /** Starts a new generation of cloud data. @return its id. */
    suspend fun createGeneration(): String

    /** @return null if the file is gone. */
    suspend fun currentMd5(id: String): String?

    /** @return false if the file is gone. */
    suspend fun download(id: String, dest: File): Boolean

    /** Creates a file when [existingId] is null, otherwise replaces its content. */
    suspend fun upload(name: String, src: File, existingId: String?): CloudFile

    /** Moves the file to the trash folder, creating it when [trashFolderId] is null. @return the trash folder id. */
    suspend fun moveToTrash(id: String, trashFolderId: String?): String

    suspend fun purgeTrash(trashFolderId: String, trashedBefore: Long)

    /** Deletes the generation first, so other devices never take a partly deleted cloud for deletions. */
    suspend fun deleteAll()
}
