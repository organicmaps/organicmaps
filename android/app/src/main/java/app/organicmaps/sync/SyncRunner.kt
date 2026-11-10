package app.organicmaps.sync

import app.organicmaps.sdk.sync.BookmarksSyncEngine
import app.organicmaps.sdk.sync.CloudStore
import app.organicmaps.sdk.sync.LocalChangesGate
import app.organicmaps.sdk.sync.PassResult
import app.organicmaps.sdk.sync.SyncError
import kotlinx.coroutines.CancellationException
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.async
import kotlinx.coroutines.coroutineScope
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.withContext

sealed interface SyncStatus {
    data class Idle(val lastSync: Long?) : SyncStatus

    /** [lastSync] stays on screen while the pass runs. */
    data class Running(val lastSync: Long?) : SyncStatus

    data object NeedsSignIn : SyncStatus

    data class Error(val error: SyncError) : SyncStatus
}

/** Runs the core engine off the main thread, which it waits for, and shows what it did. */
class SyncRunner(
    private val engine: BookmarksSyncEngine,
    private val store: CloudStore,
    private val gate: LocalChangesGate,
) {
    private val mutableStatus = MutableStateFlow<SyncStatus>(SyncStatus.Idle(engine.lastSync))
    val status: StateFlow<SyncStatus> = mutableStatus.asStateFlow()

    /** True during a pass, and while the cloud data is being deleted. */
    val isRunning: Boolean
        get() = engine.isRunning

    /** Shows a failure that happened outside of a pass, like a background authorization. */
    fun showError(error: SyncError) {
        mutableStatus.value = statusFor(error)
    }

    /** Clears a sign-in request or an error shown before; the next pass reports its own result. */
    fun onAuthorized() {
        val status = mutableStatus.value
        if (status == SyncStatus.NeedsSignIn || status is SyncStatus.Error) {
            mutableStatus.value = SyncStatus.Idle(engine.lastSync)
        }
    }

    suspend fun runPass(): PassResult = coroutineScope {
        mutableStatus.value = SyncStatus.Running(engine.lastSync)
        val pass = async(Dispatchers.IO) { engine.runPass(store, gate) }
        val result = try {
            pass.await()
        } catch (e: CancellationException) {
            // The native call doesn't see the cancellation: stop it, the scope waits for it to return.
            engine.cancel()
            store.cancelPass()
            mutableStatus.value = SyncStatus.Idle(engine.lastSync)
            throw e
        }
        mutableStatus.value =
            if (result is PassResult.Failed) statusFor(result.error) else SyncStatus.Idle(engine.lastSync)
        result
    }

    suspend fun forgetMissingLocalFiles() = withContext(Dispatchers.IO) { engine.forgetMissingLocalFiles() }

    /** @throws app.organicmaps.sdk.sync.CloudException */
    suspend fun deleteCloudData() {
        withContext(Dispatchers.IO) { engine.deleteCloudData(store) }
        mutableStatus.value = SyncStatus.Idle(null)
    }

    private fun statusFor(error: SyncError): SyncStatus = when (error) {
        SyncError.AuthRequired -> SyncStatus.NeedsSignIn

        // WorkManager retries, so the last sync stays on screen.
        SyncError.Transient, SyncError.Cancelled -> SyncStatus.Idle(engine.lastSync)

        else -> SyncStatus.Error(error)
    }
}
