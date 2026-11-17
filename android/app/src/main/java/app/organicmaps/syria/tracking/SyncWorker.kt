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
import org.json.JSONArray
import org.json.JSONObject

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
                    more = uploadRound(context, db)
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

    /** Uploads one batch from the outbox. Returns true when the batch was full, so more may be waiting. */
    private fun uploadRound(context: Context, db: TrackingDb): Boolean {
        val sessions = db.unsyncedSessions(SESSIONS_PER_BATCH)
        val segments = db.unsyncedSegments(SEGMENTS_PER_BATCH)
        val points = db.unsyncedPoints(POINTS_PER_BATCH)
        val events = db.unsyncedEvents(EVENTS_PER_BATCH)
        val fill = listOf(
            sessions.size to SESSIONS_PER_BATCH,
            segments.size to SEGMENTS_PER_BATCH,
            points.size to POINTS_PER_BATCH,
            events.size to EVENTS_PER_BATCH,
        )
        if (fill.all { (count, _) -> count == 0 }) return false

        val batch = JSONObject()
            .put("device", ApiClient.deviceJson(context))
            .put("clientSentAt", iso(System.currentTimeMillis()))
            .put("sessions", JSONArray(sessions.map { it.toJson() }))
            .put("segments", JSONArray(segments.map { it.toJson() }))
            .put("points", JSONArray(points.map { it.toJson() }))
            .put("events", JSONArray(events.map { it.toJson() }))
        val acked = ApiClient.syncBatch(context, batch).getJSONObject("acked")

        val ackedSessions = acked.getJSONArray("sessions").toStringSet()
        db.markSessionsSynced(sessions.filter { it.id in ackedSessions })
        db.markSynced("segment", acked.getJSONArray("segments").toStringSet())
        db.markSynced("point", acked.getJSONArray("points").toStringSet())
        db.markSynced("event", acked.getJSONArray("events").toStringSet())
        Logger.i(
            TAG,
            "Synced ${sessions.size} sessions, ${segments.size} segments, ${points.size} points, ${events.size} events",
        )
        return fill.any { (count, max) -> count == max }
    }

    private fun JSONArray.toStringSet(): Set<String> = (0 until length()).mapTo(HashSet()) { getString(it) }

    private fun Session.toJson(): JSONObject = JSONObject()
        .put("id", id)
        .put("status", status)
        .put("startedAt", iso(startedAt))
        .put("endedAt", endedAt?.let { iso(it) } ?: JSONObject.NULL)
        .put("name", name ?: JSONObject.NULL)
        .put("clientDistanceM", distanceM)
        .put("clientMovingTimeS", movingTimeS)

    private fun Segment.toJson(): JSONObject = JSONObject()
        .put("id", id)
        .put("sessionId", sessionId)
        .put("seq", seq)
        .put("startedAt", iso(startedAt))
        .put("endedAt", endedAt?.let { iso(it) } ?: JSONObject.NULL)

    private fun TrackPoint.toJson(): JSONObject = JSONObject()
        .put("id", id)
        .put("sessionId", sessionId)
        .put("segmentId", segmentId)
        .put("recordedAt", iso(recordedAt))
        .put("lat", lat)
        .put("lon", lon)
        .put("alt", alt ?: JSONObject.NULL)
        .put("accuracyM", accuracyM?.toDouble() ?: JSONObject.NULL)
        .put("speedMps", speedMps?.toDouble() ?: JSONObject.NULL)
        .put("bearingDeg", bearingDeg?.toDouble() ?: JSONObject.NULL)
        .put("provider", provider ?: JSONObject.NULL)
        .put("isMock", isMock)
        .put("batteryPct", batteryPct ?: JSONObject.NULL)

    private fun TrackEvent.toJson(): JSONObject = JSONObject()
        .put("id", id)
        .put("type", type)
        .put("occurredAt", iso(occurredAt))
        .put("sessionId", sessionId ?: JSONObject.NULL)
        .put("payload", payload?.let { JSONObject(it) } ?: JSONObject.NULL)

    companion object {
        private const val TAG = "SyriaSync"
        const val UNIQUE_NOW = "SyriaTrackingSync"
        private const val UNIQUE_PERIODIC = "SyriaTrackingSyncPeriodic"
        private const val MAX_RETRIES = 10
        private const val MAX_ROUNDS = 50
        private const val SESSIONS_PER_BATCH = 100
        private const val SEGMENTS_PER_BATCH = 500
        private const val POINTS_PER_BATCH = 500
        private const val EVENTS_PER_BATCH = 200

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
