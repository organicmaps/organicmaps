package app.organicmaps.sync

import android.content.Context
import androidx.work.CoroutineWorker
import androidx.work.WorkerParameters
import app.organicmaps.MwmApplication
import app.organicmaps.sdk.bookmarks.data.BookmarkManager
import app.organicmaps.sdk.sync.AuthResult
import app.organicmaps.sdk.sync.CloudSyncRegistry
import app.organicmaps.sdk.sync.PassResult
import app.organicmaps.sdk.sync.SyncError
import app.organicmaps.sdk.util.log.Logger
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

class BookmarksSyncWork(context: Context, params: WorkerParameters) : CoroutineWorker(context, params) {
    override suspend fun doWork(): Result {
        val provider = CloudSyncRegistry.provider ?: return Result.success()
        // Started without the core, for example after the process was killed: the next app start syncs anyway.
        if (!BookmarksSync.isEnabled(applicationContext) || !isCoreReady()) return Result.success()
        val engine =
            withContext(Dispatchers.Main) { BookmarksSync.engine(applicationContext) } ?: return Result.success()

        when (val auth = provider.authorize()) {
            // Learns the account once, so that later passes ask for it explicitly.
            AuthResult.Authorized -> provider.accountEmail()

            is AuthResult.NeedsResolution -> {
                engine.showError(SyncError.AuthRequired)
                return Result.success()
            }

            is AuthResult.Failed -> {
                engine.showError(auth.error)
                return retryIfTransient(auth.error)
            }
        }

        val result = withContext(Dispatchers.IO) { engine.runPass() }
        val cause = (result as? PassResult.Failed)?.cause
        if (cause != null) Logger.e(TAG, "Pass failed", cause) else Logger.i(TAG, "Pass finished: $result")
        withContext(Dispatchers.Main) { BookmarksSync.onPassFinished(applicationContext, result) }
        return when (result) {
            PassResult.Synced -> Result.success()

            // Bookmark screens or the Place Page keep categories from being replaced: try again, so remote changes
            // don't wait for the next trigger when they stay open.
            PassResult.Deferred -> retryIfAttemptsLeft()

            is PassResult.Failed -> retryIfTransient(result.error)
        }
    }

    private suspend fun isCoreReady() = withContext(Dispatchers.Main) {
        MwmApplication.from(applicationContext).organicMaps.arePlatformAndCoreInitialized() &&
            !BookmarkManager.INSTANCE.isAsyncBookmarksLoadingInProgress
    }

    private fun retryIfTransient(error: SyncError) =
        if (error == SyncError.Transient) retryIfAttemptsLeft() else Result.success()

    private fun retryIfAttemptsLeft() = if (runAttemptCount < MAX_RETRIES) Result.retry() else Result.success()

    private companion object {
        const val TAG = "BookmarksSyncWork"
        const val MAX_RETRIES = 5
    }
}
