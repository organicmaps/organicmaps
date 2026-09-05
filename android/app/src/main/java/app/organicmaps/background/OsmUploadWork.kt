package app.organicmaps.background

import android.content.Context
import androidx.work.Constraints
import androidx.work.CoroutineWorker
import androidx.work.ExistingWorkPolicy
import androidx.work.NetworkType
import androidx.work.OneTimeWorkRequest
import androidx.work.WorkManager
import androidx.work.WorkerParameters
import app.organicmaps.MwmApplication
import app.organicmaps.sdk.editor.Editor
import app.organicmaps.sdk.editor.OsmOAuth
import app.organicmaps.sdk.util.log.Logger
import kotlin.coroutines.resume
import kotlinx.coroutines.suspendCancellableCoroutine

class OsmUploadWork(context: Context, private val workerParameters: WorkerParameters) :
    CoroutineWorker(context, workerParameters) {

    override suspend fun doWork(): Result {
        if (!MwmApplication.from(applicationContext).organicMaps.arePlatformAndCoreInitialized()) {
            Logger.w(TAG, "Application is not initialized, ignoring $workerParameters")
            return Result.failure()
        }

        val result = suspendCancellableCoroutine { continuation ->
            Editor.uploadChanges { continuation.resume(it) }
        }
        if (result != Editor.UPLOAD_RESULT_ERROR) {
            return Result.success()
        }
        return if (runAttemptCount >= MAX_RUN_ATTEMPTS) Result.failure() else Result.retry()
    }

    companion object {
        private val TAG = OsmUploadWork::class.java.simpleName
        private const val MAX_RUN_ATTEMPTS = 5

        /**
         * Starts this worker to upload map edits to osm servers.
         */
        @JvmStatic
        fun startActionUploadOsmChanges(context: Context) {
            if (!Editor.nativeHasSomethingToUpload() || !OsmOAuth.isAuthorized()) {
                return
            }

            val c = Constraints.Builder().setRequiredNetworkType(NetworkType.CONNECTED).build()
            val wr = OneTimeWorkRequest.Builder(OsmUploadWork::class.java).setConstraints(c).build()
            WorkManager.getInstance(context)
                .beginUniqueWork("UploadOsmChanges", ExistingWorkPolicy.KEEP, wr)
                .enqueue()
        }
    }
}
