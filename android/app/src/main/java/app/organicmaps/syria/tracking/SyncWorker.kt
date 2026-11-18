package app.organicmaps.syria.tracking

import android.content.Context
import androidx.work.BackoffPolicy
import androidx.work.Constraints
import androidx.work.CoroutineWorker
import androidx.work.ExistingPeriodicWorkPolicy
import androidx.work.ExistingWorkPolicy
import androidx.work.NetworkType
import androidx.work.OneTimeWorkRequestBuilder
import androidx.work.PeriodicWorkRequestBuilder
import androidx.work.WorkManager
import androidx.work.WorkerParameters
import app.organicmaps.sdk.util.log.Logger
import app.organicmaps.syria.ApiClient
import app.organicmaps.syria.ApiException
import app.organicmaps.syria.SyriaPrefs
import app.organicmaps.syria.SyriaStatus
import java.io.IOException
import java.time.Instant
import java.util.concurrent.TimeUnit
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

/** Uploads the local outbox in batches; the server acks ids, which are then flagged as synced. */
class SyncWorker(context: Context, params: WorkerParameters) : CoroutineWorker(context, params) {

    override suspend fun doWork(): Result = withContext(Dispatchers.IO) {
        val context = applicationContext
        if (!SyriaPrefs.isLoggedIn(context)) return@withContext Result.success()
        val db = TrackingDb.get(context)
        SyriaStatus.setSyncing(context, true)
        try {
            var error: String? = null
            val result = try {
                val total = db.unsyncedCount()
                SyriaStatus.setProgress(context, 0, total)
                var rounds = 0
                var more = true
                while (more && rounds++ < MAX_ROUNDS) {
                    more = OutboxUploader.uploadRound(context, db)
                    SyriaStatus.setProgress(context, total - db.unsyncedCount(), total)
                }
                SyriaPrefs.setLastSyncAt(context, System.currentTimeMillis())
                Result.success()
            } catch (e: ApiException) {
                Logger.w(TAG, "Sync rejected: HTTP ${e.status} ${e.message}")
                // A proxy in front of a stopped API answers 502-504: for the user that is the same as no server.
                error = if (e.status in 502..504) SyriaPrefs.SYNC_ERROR_UNREACHABLE else "HTTP ${e.status}: ${e.message}"
                // 4xx answers will not change on retry (expired login, malformed batch); 5xx may.
                if (e.status in 400..499) Result.failure() else retryOrFail()
            } catch (e: IOException) {
                Logger.w(TAG, "Sync failed: ${e.message}")
                // No route, refused connection, timeout, unknown host: the server is not reachable.
                error = SyriaPrefs.SYNC_ERROR_UNREACHABLE
                retryOrFail()
            }
            SyriaPrefs.setLastSyncResult(context, error)
            result
        } finally {
            SyriaStatus.setSyncing(context, false)
        }
    }

    private fun retryOrFail(): Result = if (runAttemptCount >= MAX_RETRIES) Result.failure() else Result.retry()

    companion object {
        private const val TAG = "SyriaSync"
        const val UNIQUE_NOW = "SyriaTrackingSync"
        private const val UNIQUE_PERIODIC = "SyriaTrackingSyncPeriodic"
        private const val MAX_RETRIES = 10
        private const val MAX_ROUNDS = 50

        private val constraints = Constraints.Builder().setRequiredNetworkType(NetworkType.CONNECTED).build()

        fun iso(timeMs: Long): String = Instant.ofEpochMilli(timeMs).toString()

        /** Uploads as soon as the network allows; queued behind a run that is already in progress. */
        @JvmStatic
        fun enqueueNow(context: Context) {
            val request = OneTimeWorkRequestBuilder<SyncWorker>()
                .setConstraints(constraints)
                .setBackoffCriteria(BackoffPolicy.EXPONENTIAL, 30, TimeUnit.SECONDS)
                .build()
            WorkManager.getInstance(context)
                .enqueueUniqueWork(UNIQUE_NOW, ExistingWorkPolicy.APPEND_OR_REPLACE, request)
        }

        /**
         * The "Sync now" button and the one-minute sync (SyriaStatus): starts right away, replacing a queued run that may be waiting out a
         * retry backoff after an earlier failure. Uploads are idempotent, so cutting a running one short is safe.
         */
        @JvmStatic
        fun syncNowManually(context: Context) {
            val request = OneTimeWorkRequestBuilder<SyncWorker>()
                .setConstraints(constraints)
                .setBackoffCriteria(BackoffPolicy.EXPONENTIAL, 30, TimeUnit.SECONDS)
                .build()
            WorkManager.getInstance(context).enqueueUniqueWork(UNIQUE_NOW, ExistingWorkPolicy.REPLACE, request)
        }

        /** Catches up every 15 minutes while points keep coming in (e.g. a long offline recording). */
        @JvmStatic
        fun schedulePeriodic(context: Context) {
            val request = PeriodicWorkRequestBuilder<SyncWorker>(15, TimeUnit.MINUTES)
                .setConstraints(constraints)
                .setBackoffCriteria(BackoffPolicy.EXPONENTIAL, 30, TimeUnit.SECONDS)
                .build()
            WorkManager.getInstance(context)
                .enqueueUniquePeriodicWork(UNIQUE_PERIODIC, ExistingPeriodicWorkPolicy.KEEP, request)
        }
    }
}
