package app.organicmaps.sdk.sync

/** A category file in the bookmarks directory, identified by its file name. */
data class LocalFile(val name: String, val md5: String, val isEmptyCategory: Boolean)

/** A category file in the cloud. [createdTime] is RFC 3339 in UTC, so it orders as a string. */
data class CloudFile(val id: String, val name: String, val md5: String, val createdTime: String)

/** The state of a file after its last successful synchronization. */
data class BaseEntry(val cloudId: String, val md5: String)

data class BaseState(
    val account: String?,
    val trashFolderId: String?,
    val lastSync: Long?,
    val files: Map<String, BaseEntry>,
) {
    companion object {
        val EMPTY = BaseState(account = null, trashFolderId = null, lastSync = null, files = emptyMap())
    }
}

sealed interface SyncAction {
    val name: String

    /** Creates the cloud file when [cloudId] is null. */
    data class Upload(override val name: String, val localMd5: String, val cloudId: String?) : SyncAction

    /** Replaces the local file only if its MD5 is still [expectedLocalMd5]; null means it must not exist. */
    data class Download(override val name: String, val cloud: CloudFile, val expectedLocalMd5: String?) : SyncAction

    /** Keeps the local version as a copy named after the device, then downloads [cloud] under [name]. */
    data class ConflictCopy(override val name: String, val cloud: CloudFile, val localMd5: String) : SyncAction

    data class TrashCloud(override val name: String, val cloudId: String, val expectedMd5: String) : SyncAction

    data class TrashLocal(override val name: String, val expectedMd5: String) : SyncAction

    data class RecordBase(override val name: String, val entry: BaseEntry) : SyncAction

    data class DropBase(override val name: String) : SyncAction

    /** Trashes a cloud file that lost the name to an older one, first downloading it when [keepAsLocalCopy]. */
    data class ResolveDuplicate(override val name: String, val duplicate: CloudFile, val keepAsLocalCopy: Boolean) :
        SyncAction
}

/** When [resetBase] is set, the actions were planned against an empty base, as for the first synchronization. */
data class SyncPlan(val resetBase: Boolean, val actions: List<SyncAction>)
