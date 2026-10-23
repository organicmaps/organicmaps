package app.organicmaps.sdk.sync

/** A category file in the bookmarks directory, identified by its file name. */
data class LocalFile(val name: String, val md5: String)

/** A category file in the cloud. [createdTime] is RFC 3339 in UTC, so it orders as a string. */
data class CloudFile(val id: String, val name: String, val md5: String, val createdTime: String)

/** The state of a file after its last successful synchronization. */
data class BaseEntry(val cloudId: String, val md5: String)

/** The base is valid only for the cloud [generation] it was recorded in. */
data class BaseState(
    val generation: String?,
    val trashFolderId: String?,
    val lastSync: Long?,
    val files: Map<String, BaseEntry>,
) {
    companion object {
        val EMPTY = BaseState(generation = null, trashFolderId = null, lastSync = null, files = emptyMap())
    }
}

sealed interface SyncAction {
    val name: String

    /**
     * Replaces the content of [cloudId] if it is unchanged since the last sync, or creates the cloud file when
     * [cloudId] is null or the file is gone.
     */
    data class Upload(override val name: String, val cloudId: String?) : SyncAction

    /**
     * Replaces the local file only if its MD5 is still [expectedLocalMd5]; null means it must not exist.
     * [replacedCloudId] is the synced cloud file that [cloud] took the name from: the download waits until it is
     * confirmed gone, as a listing can lag behind.
     */
    data class Download(
        override val name: String,
        val cloud: CloudFile,
        val expectedLocalMd5: String?,
        val replacedCloudId: String? = null,
    ) : SyncAction

    /**
     * Keeps the local version as a copy named after the device, then downloads [cloud] under [name]. Without a
     * synchronized state ([hasBase] is false), a version without bookmarks and tracks, like an untouched default list,
     * yields to the other one instead.
     */
    data class ConflictCopy(
        override val name: String,
        val cloud: CloudFile,
        val localMd5: String,
        val hasBase: Boolean,
    ) : SyncAction

    data class TrashCloud(override val name: String, val cloudId: String, val expectedMd5: String) : SyncAction

    /** Trashes the local file deleted in the cloud, once [cloudId] is confirmed to be gone. */
    data class TrashLocal(override val name: String, val expectedMd5: String, val cloudId: String) : SyncAction

    data class RecordBase(override val name: String, val entry: BaseEntry) : SyncAction

    data class DropBase(override val name: String) : SyncAction

    /** Trashes a cloud file that lost the name to an older one, first downloading it when [keepAsLocalCopy]. */
    data class ResolveDuplicate(override val name: String, val duplicate: CloudFile, val keepAsLocalCopy: Boolean) :
        SyncAction
}
