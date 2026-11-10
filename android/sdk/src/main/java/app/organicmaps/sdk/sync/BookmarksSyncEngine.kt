package app.organicmaps.sdk.sync

import androidx.annotation.Keep
import androidx.annotation.WorkerThread

/** Called on the main thread: false while replacing a category would break a screen that keeps its ids. */
@Keep
fun interface LocalChangesGate {
    fun canApply(): Boolean
}

@Keep
sealed interface PassResult {
    data object Synced : PassResult

    /** Some remote changes wait until no screen keeps their categories. */
    data object Deferred : PassResult

    /** Something changed while the pass ran, so another pass is needed soon. */
    data object Retry : PassResult

    data class Failed(val error: SyncError) : PassResult
}

/**
 * The core engine that synchronizes the bookmarks directory with one cloud store, persisting its state at [statePath].
 * Create it after the core is initialized. Calls wait for the main thread, so make them on another one, except
 * [cancel], [isRunning] and [lastSync].
 */
@Keep
class BookmarksSyncEngine(statePath: String, stagingDir: String, deviceName: String, hashAlgorithm: HashAlgorithm?) {
    private val handle = nativeCreate(statePath, stagingDir, deviceName, hashAlgorithm?.ordinal ?: -1)

    /** True during a pass and while the cloud data is being deleted. */
    val isRunning: Boolean
        get() = nativeIsRunning(handle)

    /** Milliseconds since the epoch, null if never synchronized. */
    val lastSync: Long?
        get() = nativeGetLastSync(handle).takeIf { it != 0L }

    @WorkerThread
    fun runPass(store: CloudStore, gate: LocalChangesGate): PassResult = nativeRunPass(handle, store, gate)

    /** Makes the running pass stop before its next cloud call, call [CloudStore.cancelPass] too to abort the current. */
    fun cancel() = nativeCancel(handle)

    /** Keeps the synchronized state only for files that still exist, see the core SyncEngine. */
    @WorkerThread
    fun forgetMissingLocalFiles() = nativeForgetMissingLocalFiles(handle)

    @WorkerThread
    @Throws(CloudException::class)
    fun deleteCloudData(store: CloudStore) = nativeDeleteCloudData(handle, store)

    private companion object {
        // Called by the core, in the order of its enum. An interface companion would get no static method.
        @JvmStatic
        private fun passResult(status: Int, errorCode: Int, message: String): PassResult = when (status) {
            0 -> PassResult.Synced
            1 -> PassResult.Deferred
            2 -> PassResult.Retry
            else -> PassResult.Failed(SyncError.fromCode(errorCode, message))
        }

        @JvmStatic
        private external fun nativeCreate(
            statePath: String,
            stagingDir: String,
            deviceName: String,
            hashAlgorithm: Int,
        ): Long

        @JvmStatic
        private external fun nativeRunPass(handle: Long, store: CloudStore, gate: LocalChangesGate): PassResult

        @JvmStatic
        private external fun nativeCancel(handle: Long)

        @JvmStatic
        private external fun nativeIsRunning(handle: Long): Boolean

        @JvmStatic
        private external fun nativeGetLastSync(handle: Long): Long

        @JvmStatic
        private external fun nativeForgetMissingLocalFiles(handle: Long)

        @JvmStatic
        private external fun nativeDeleteCloudData(handle: Long, store: CloudStore)
    }
}
