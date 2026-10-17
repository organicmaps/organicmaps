package app.organicmaps.sdk.sync

import java.io.File

sealed interface SyncError {
    data object AuthRequired : SyncError

    data object QuotaExceeded : SyncError

    /** Network problems, rate limits and server errors that a later retry may fix. */
    data object Transient : SyncError

    data class Other(val message: String) : SyncError
}

/** [resetStartedAt] is set while another device deletes all cloud data. */
data class CloudListing(val files: List<CloudFile>, val resetStartedAt: Long?)

class CloudException(val error: SyncError, cause: Throwable? = null) : Exception(error.toString(), cause)

/** Cloud storage of category files. Every method throws [CloudException]. */
interface CloudStore {
    /** A stable id of the signed-in account. */
    suspend fun account(): String

    /** The top-level `*.kml` files. */
    suspend fun list(): CloudListing

    /** @return null if the file is gone. */
    suspend fun currentMd5(id: String): String?

    suspend fun download(id: String, dest: File)

    /** Creates a file when [existingId] is null, otherwise replaces its content. */
    suspend fun upload(name: String, src: File, existingId: String?): CloudFile

    /** Moves the file to the trash folder, creating it when [trashFolderId] is null. @return the trash folder id. */
    suspend fun moveToTrash(id: String, trashFolderId: String?): String

    suspend fun purgeTrash(trashFolderId: String, trashedBefore: Long)

    suspend fun deleteAll()
}
