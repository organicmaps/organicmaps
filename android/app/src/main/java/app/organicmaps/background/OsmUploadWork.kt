package app.organicmaps.background

import android.content.Context
import androidx.work.Constraints
import androidx.work.CoroutineWorker
import androidx.work.ExistingWorkPolicy
import androidx.work.NetworkType
import androidx.work.OneTimeWorkRequestBuilder
import androidx.work.WorkManager
import androidx.work.WorkerParameters
import app.organicmaps.MwmApplication
import app.organicmaps.sdk.editor.Editor
import app.organicmaps.sdk.editor.OsmOAuth
import app.organicmaps.sdk.util.log.Logger
import kotlin.coroutines.resume
import kotlinx.coroutines.suspendCancellableCoroutine

class OsmUploadWork(context: Context, params: WorkerParameters) : CoroutineWorker(context, params) {

    override suspend fun doWork(): Result {
        if (!MwmApplication.from(applicationContext).organicMaps.arePlatformAndCoreInitialized()) {
            Logger.w(TAG, "Application is not initialized, ignoring work $id")
            return Result.failure()
        }

        val result = suspendCancellableCoroutine { continuation ->
            Editor.uploadChanges { continuation.resume(it) }
        }
        return when (result) {
            Editor.UPLOAD_RESULT_SUCCESS, Editor.UPLOAD_RESULT_NOTHING_TO_UPLOAD -> Result.success()

            Editor.UPLOAD_RESULT_ERROR ->
                if (runAttemptCount >= MAX_RETRIES) Result.failure() else Result.retry()

            Editor.UPLOAD_FAILED_NOT_AUTHORIZED -> Result.failure()

            else -> error("Unexpected editor upload result: $result")
        }
    }

    companion object {
        private const val TAG = "OsmUploadWork"
        private const val MAX_RETRIES = 5
        private const val UNIQUE_WORK_NAME = "UploadOsmChanges"

        @JvmStatic
        fun startActionUploadOsmChanges(context: Context) {
            if (!Editor.nativeHasSomethingToUpload() || !OsmOAuth.isAuthorized()) {
                return
            }

            val constraints = Constraints.Builder().setRequiredNetworkType(NetworkType.CONNECTED).build()
            val request = OneTimeWorkRequestBuilder<OsmUploadWork>().setConstraints(constraints).build()
            WorkManager.getInstance(context)
                .beginUniqueWork(UNIQUE_WORK_NAME, ExistingWorkPolicy.KEEP, request)
                .enqueue()
        }
    }
}
