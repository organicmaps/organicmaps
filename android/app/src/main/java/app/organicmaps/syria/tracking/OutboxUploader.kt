package app.organicmaps.syria.tracking

import android.content.Context
import androidx.annotation.WorkerThread
import app.organicmaps.sdk.util.log.Logger
import app.organicmaps.syria.ApiClient
import org.json.JSONArray
import org.json.JSONObject

/**
 * One upload of the local outbox (recordings, searches, events), done by the background sync in rounds.
 * The server acks ids, which are then flagged as synced; re-sending a row is harmless.
 */
internal object OutboxUploader {
    private const val TAG = "SyriaSync"
    private const val SESSIONS_PER_BATCH = 100
    private const val SEGMENTS_PER_BATCH = 500
    private const val POINTS_PER_BATCH = 500
    private const val EVENTS_PER_BATCH = 200

    /** Uploads one batch from the outbox. Returns true when the batch was full, so more may be waiting. */
    @WorkerThread
    fun uploadRound(context: Context, db: TrackingDb): Boolean {
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
            .put("clientSentAt", SyncWorker.iso(System.currentTimeMillis()))
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
        .put("startedAt", SyncWorker.iso(startedAt))
        .put("endedAt", endedAt?.let { SyncWorker.iso(it) } ?: JSONObject.NULL)
        .put("name", name ?: JSONObject.NULL)
        .put("clientDistanceM", distanceM)
        .put("clientMovingTimeS", movingTimeS)

    private fun Segment.toJson(): JSONObject = JSONObject()
        .put("id", id)
        .put("sessionId", sessionId)
        .put("seq", seq)
        .put("startedAt", SyncWorker.iso(startedAt))
        .put("endedAt", endedAt?.let { SyncWorker.iso(it) } ?: JSONObject.NULL)

    private fun TrackPoint.toJson(): JSONObject = JSONObject()
        .put("id", id)
        .put("sessionId", sessionId)
        .put("segmentId", segmentId)
        .put("recordedAt", SyncWorker.iso(recordedAt))
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
        .put("occurredAt", SyncWorker.iso(occurredAt))
        .put("sessionId", sessionId ?: JSONObject.NULL)
        .put("payload", payload?.let { JSONObject(it) } ?: JSONObject.NULL)
}
