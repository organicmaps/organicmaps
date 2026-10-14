package app.organicmaps.sdk.sync

import app.organicmaps.sdk.sync.SyncAction.ConflictCopy
import app.organicmaps.sdk.sync.SyncAction.Download
import app.organicmaps.sdk.sync.SyncAction.DropBase
import app.organicmaps.sdk.sync.SyncAction.RecordBase
import app.organicmaps.sdk.sync.SyncAction.ResolveDuplicate
import app.organicmaps.sdk.sync.SyncAction.TrashCloud
import app.organicmaps.sdk.sync.SyncAction.TrashLocal
import app.organicmaps.sdk.sync.SyncAction.Upload
import java.io.File
import kotlinx.coroutines.CancellationException
import kotlinx.coroutines.NonCancellable
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.sync.Mutex
import kotlinx.coroutines.sync.withLock
import kotlinx.coroutines.withContext

sealed interface SyncStatus {
    data class Idle(val lastSync: Long?) : SyncStatus

    data object Running : SyncStatus

    data object NeedsSignIn : SyncStatus

    data class Error(val error: SyncError) : SyncStatus
}

sealed interface PassResult {
    data object Synced : PassResult

    /** Some remote changes wait until core saves local edits again. */
    data object Deferred : PassResult

    data class Failed(val error: SyncError) : PassResult
}

/** Runs one reconciliation pass at a time and persists the base state after every applied action. */
class SyncEngine(
    private val cloud: CloudStore,
    private val local: LocalStore,
    private val baseStore: BaseStateStore,
    private val deviceName: String,
    private val now: () -> Long,
) {
    private val mutex = Mutex()
    private val mutableStatus = MutableStateFlow<SyncStatus>(SyncStatus.Idle(baseStore.load().lastSync))
    val status: StateFlow<SyncStatus> = mutableStatus.asStateFlow()

    @Volatile
    var isRunning = false
        private set

    fun markNeedsSignIn() {
        mutableStatus.value = SyncStatus.NeedsSignIn
    }

    @Suppress("TooGenericExceptionCaught") // Any failure must end the pass with a visible status, not "Syncing…".
    suspend fun runPass(): PassResult = mutex.withLock {
        isRunning = true
        mutableStatus.value = SyncStatus.Running
        try {
            val result = pass()
            mutableStatus.value = SyncStatus.Idle(baseStore.load().lastSync)
            result
        } catch (e: CloudException) {
            mutableStatus.value =
                if (e.error == SyncError.AuthRequired) SyncStatus.NeedsSignIn else SyncStatus.Error(e.error)
            PassResult.Failed(e.error)
        } catch (e: CancellationException) {
            throw e
        } catch (e: Exception) {
            val error = SyncError.Other(e.message ?: e.javaClass.simpleName)
            mutableStatus.value = SyncStatus.Error(error)
            PassResult.Failed(error)
        } finally {
            isRunning = false
            if (mutableStatus.value ==
                SyncStatus.Running
            ) {
                mutableStatus.value = SyncStatus.Idle(baseStore.load().lastSync)
            }
        }
    }

    /** Forgets the base first, so a partial deletion can never look like files deleted on this device. */
    suspend fun deleteCloudData() = mutex.withLock {
        forgetLocalState()
        withContext(NonCancellable) { cloud.deleteAll() }
    }

    fun forgetLocalState() {
        baseStore.delete()
        mutableStatus.value = SyncStatus.Idle(null)
    }

    private suspend fun pass(): PassResult {
        local.cleanTemp()
        val account = cloud.account()
        val listing = cloud.list()
        val localFiles = local.snapshot()
        var state = baseStore.load()
        // A marker left by a device that died while deleting all data expires.
        val isCloudReset = listing.resetStartedAt?.let { now() - it < RESET_TIMEOUT_MILLIS } == true
        val plan = Reconciler.plan(localFiles, listing.files, state, account, isCloudReset)
        if (plan.resetBase) state = BaseState.EMPTY
        state = state.copy(account = account)
        baseStore.save(state)

        var deferred = false
        for (action in plan.actions) {
            if (changesLocalFiles(action) && !local.canApplyChanges()) {
                deferred = true
                continue
            }
            state = apply(action, state)
            baseStore.save(state)
        }

        state.trashFolderId?.let { cloud.purgeTrash(it, now() - TRASH_RETENTION_MILLIS) }
        baseStore.save(state.copy(lastSync = now()))
        return if (deferred) PassResult.Deferred else PassResult.Synced
    }

    private fun changesLocalFiles(action: SyncAction) = when (action) {
        is Download, is ConflictCopy, is TrashLocal -> true
        is ResolveDuplicate -> action.keepAsLocalCopy
        else -> false
    }

    private suspend fun apply(action: SyncAction, state: BaseState): BaseState = when (action) {
        is Upload -> upload(action, state)

        is Download -> {
            val installed = downloadVerified(action.cloud)?.let {
                local.install(action.name, it, action.expectedLocalMd5)
            }
            if (installed == true) state.withFile(action.name, action.cloud) else state
        }

        is ConflictCopy -> conflictCopy(action, state)

        is TrashCloud -> trashCloud(action, state)

        is TrashLocal -> if (local.trash(action.name, action.expectedMd5)) state.withoutFile(action.name) else state

        is RecordBase -> state.copy(files = state.files + (action.name to action.entry))

        is DropBase -> state.withoutFile(action.name)

        is ResolveDuplicate -> resolveDuplicate(action, state)
    }

    private suspend fun upload(action: Upload, state: BaseState): BaseState {
        // Overwrite only the version the plan saw unchanged since the last sync.
        if (action.cloudId != null && cloud.currentMd5(action.cloudId) != state.files[action.name]?.md5) return state
        return state.withFile(action.name, cloud.upload(action.name, local.file(action.name), action.cloudId))
    }

    private suspend fun conflictCopy(action: ConflictCopy, state: BaseState): BaseState {
        val tmp = downloadVerified(action.cloud) ?: return state
        if (!KmlFiles.hasPlacemarks(tmp)) {
            // An untouched default list from another device must not push this device's list into a copy.
            tmp.delete()
            if (cloud.currentMd5(action.cloud.id) != action.cloud.md5) return state
            return state.withFile(action.name, cloud.upload(action.name, local.file(action.name), action.cloud.id))
        }
        local.copyForConflict(action.name, deviceName) ?: return state
        val installed = local.install(action.name, tmp, action.localMd5)
        return if (installed) state.withFile(action.name, action.cloud) else state
    }

    private suspend fun trashCloud(action: TrashCloud, state: BaseState): BaseState =
        when (cloud.currentMd5(action.cloudId)) {
            null -> state.withoutFile(action.name)

            action.expectedMd5 -> {
                val trashFolderId = cloud.moveToTrash(action.cloudId, state.trashFolderId)
                state.copy(trashFolderId = trashFolderId).withoutFile(action.name)
            }

            else -> state
        }

    private suspend fun resolveDuplicate(action: ResolveDuplicate, state: BaseState): BaseState {
        if (action.keepAsLocalCopy) {
            val tmp = downloadVerified(action.duplicate) ?: return state
            if (KmlFiles.hasPlacemarks(tmp)) {
                local.installUnique(action.name, tmp) ?: return state
            } else {
                tmp.delete()
            }
        }
        return state.copy(trashFolderId = cloud.moveToTrash(action.duplicate.id, state.trashFolderId))
    }

    /** @return null if the file changed in the cloud after it was listed; the next pass sees the new version. */
    private suspend fun downloadVerified(file: CloudFile): File? {
        val tmp = local.newTempFile()
        cloud.download(file.id, tmp)
        if (Md5.of(tmp) == file.md5) return tmp
        tmp.delete()
        return null
    }

    private fun BaseState.withFile(name: String, file: CloudFile) = copy(
        files =
            files + (name to BaseEntry(file.id, file.md5)),
    )

    private fun BaseState.withoutFile(name: String) = copy(files = files - name)

    private companion object {
        const val TRASH_RETENTION_MILLIS = 30L * 24 * 60 * 60 * 1000
        const val RESET_TIMEOUT_MILLIS = 60L * 60 * 1000
    }
}
