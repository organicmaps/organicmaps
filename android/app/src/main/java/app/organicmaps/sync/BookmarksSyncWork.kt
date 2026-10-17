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
            AuthResult.Authorized -> Unit

            is AuthResult.NeedsResolution -> {
                engine.markNeedsSignIn()
                return Result.success()
            }

            is AuthResult.Failed -> return retryIfTransient(auth.error)
        }

        val result = engine.runPass()
        Logger.i(TAG, "Pass finished: $result")
        return if (result is PassResult.Failed) retryIfTransient(result.error) else Result.success()
    }

    private suspend fun isCoreReady() = withContext(Dispatchers.Main) {
        MwmApplication.from(applicationContext).organicMaps.arePlatformAndCoreInitialized() &&
            !BookmarkManager.INSTANCE.isAsyncBookmarksLoadingInProgress
    }

    private fun retryIfTransient(error: SyncError) =
        if (error == SyncError.Transient && runAttemptCount < MAX_RETRIES) Result.retry() else Result.success()

    private companion object {
        const val TAG = "BookmarksSyncWork"
        const val MAX_RETRIES = 5
    }
}
