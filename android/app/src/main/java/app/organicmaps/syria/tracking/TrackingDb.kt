package app.organicmaps.syria.tracking

import android.content.ContentValues
import android.content.Context
import android.database.Cursor
import android.database.sqlite.SQLiteDatabase
import android.database.sqlite.SQLiteOpenHelper

data class Session(
    val id: String,
    val status: String,
    val startedAt: Long,
    val endedAt: Long?,
    val name: String?,
    val distanceM: Double,
    val movingTimeS: Long,
    val pointCount: Int,
    val synced: Boolean,
    val updatedAt: Long,
)

data class Segment(val id: String, val sessionId: String, val seq: Int, val startedAt: Long, val endedAt: Long?)

data class TrackPoint(
    val id: String,
    val sessionId: String,
    val segmentId: String,
    val recordedAt: Long,
    val lat: Double,
    val lon: Double,
    val alt: Double?,
    val accuracyM: Float?,
    val speedMps: Float?,
    val bearingDeg: Float?,
    val provider: String?,
    val isMock: Boolean,
    val batteryPct: Int?,
)

data class TrackEvent(
    val id: String,
    val type: String,
    val occurredAt: Long,
    val sessionId: String?,
    val payload: String?,
)

/**
 * Local outbox of tracking data. Every row has a client-generated UUID and a `synced` flag; the sync
 * worker uploads unsynced rows and flips the flag when the server acknowledges them.
 */
class TrackingDb private constructor(context: Context) : SQLiteOpenHelper(context, DB_NAME, null, DB_VERSION) {

    override fun onCreate(db: SQLiteDatabase) {
        db.execSQL(
            """CREATE TABLE session (
                id TEXT PRIMARY KEY, status TEXT NOT NULL, started_at INTEGER NOT NULL, ended_at INTEGER, name TEXT,
                distance_m REAL NOT NULL DEFAULT 0, moving_time_s INTEGER NOT NULL DEFAULT 0,
                point_count INTEGER NOT NULL DEFAULT 0, synced INTEGER NOT NULL DEFAULT 0,
                updated_at INTEGER NOT NULL)""",
        )
        db.execSQL(
            """CREATE TABLE segment (
                id TEXT PRIMARY KEY, session_id TEXT NOT NULL, seq INTEGER NOT NULL, started_at INTEGER NOT NULL,
                ended_at INTEGER, synced INTEGER NOT NULL DEFAULT 0)""",
        )
        db.execSQL(
            """CREATE TABLE point (
                id TEXT PRIMARY KEY, session_id TEXT NOT NULL, segment_id TEXT NOT NULL, recorded_at INTEGER NOT NULL,
                lat REAL NOT NULL, lon REAL NOT NULL, alt REAL, accuracy REAL, speed REAL, bearing REAL, provider TEXT,
                is_mock INTEGER NOT NULL DEFAULT 0, battery INTEGER, synced INTEGER NOT NULL DEFAULT 0)""",
        )
        db.execSQL("CREATE INDEX point_session_idx ON point (session_id, recorded_at)")
        db.execSQL("CREATE INDEX point_synced_idx ON point (synced)")
        db.execSQL(
            """CREATE TABLE event (
                id TEXT PRIMARY KEY, type TEXT NOT NULL, occurred_at INTEGER NOT NULL, session_id TEXT, payload TEXT,
                synced INTEGER NOT NULL DEFAULT 0)""",
        )
    }

    override fun onUpgrade(db: SQLiteDatabase, oldVersion: Int, newVersion: Int) {
        // First schema version.
    }

    // ---- sessions

    fun insertSession(s: Session) {
        writableDatabase.insertOrThrow("session", null, s.toValues())
    }

    fun updateSessionProgress(id: String, distanceM: Double, movingTimeS: Long, pointCount: Int) {
        val values = ContentValues().apply {
            put("distance_m", distanceM)
            put("moving_time_s", movingTimeS)
            put("point_count", pointCount)
            put("synced", 0)
            put("updated_at", System.currentTimeMillis())
        }
        writableDatabase.update("session", values, "id = ?", arrayOf(id))
    }

    fun updateSessionStatus(id: String, status: String, endedAt: Long?) {
        val values = ContentValues().apply {
            put("status", status)
            if (endedAt != null) put("ended_at", endedAt)
            put("synced", 0)
            put("updated_at", System.currentTimeMillis())
        }
        writableDatabase.update("session", values, "id = ?", arrayOf(id))
    }

    fun getSession(id: String): Session? =
        readableDatabase.query("session", null, "id = ?", arrayOf(id), null, null, null).use { c ->
            if (c.moveToFirst()) c.toSession() else null
        }

    /** The session that is still active or paused, if any. */
    fun getOpenSession(): Session? =
        readableDatabase
            .query("session", null, "status <> ?", arrayOf(STATUS_STOPPED), null, null, "started_at DESC", "1")
            .use { c -> if (c.moveToFirst()) c.toSession() else null }

    fun listSessions(limit: Int): List<Session> =
        readableDatabase.query("session", null, null, null, null, null, "started_at DESC", limit.toString())
            .use { c -> c.mapAll { it.toSession() } }

    fun deleteSession(id: String) {
        val db = writableDatabase
        db.beginTransaction()
        try {
            db.delete("point", "session_id = ?", arrayOf(id))
            db.delete("segment", "session_id = ?", arrayOf(id))
            db.delete("session", "id = ?", arrayOf(id))
            db.setTransactionSuccessful()
        } finally {
            db.endTransaction()
        }
    }

    // ---- segments

    fun insertSegment(seg: Segment) {
        val values = ContentValues().apply {
            put("id", seg.id)
            put("session_id", seg.sessionId)
            put("seq", seg.seq)
            put("started_at", seg.startedAt)
            if (seg.endedAt != null) put("ended_at", seg.endedAt)
        }
        writableDatabase.insertOrThrow("segment", null, values)
    }

    fun closeSegment(id: String, endedAt: Long) {
        val values = ContentValues().apply {
            put("ended_at", endedAt)
            put("synced", 0)
        }
        writableDatabase.update("segment", values, "id = ?", arrayOf(id))
    }

    fun segmentsOf(sessionId: String): List<Segment> =
        readableDatabase.query("segment", null, "session_id = ?", arrayOf(sessionId), null, null, "seq")
            .use { c -> c.mapAll { it.toSegment() } }

    // ---- points

    fun insertPoint(p: TrackPoint) {
        val values = ContentValues().apply {
            put("id", p.id)
            put("session_id", p.sessionId)
            put("segment_id", p.segmentId)
            put("recorded_at", p.recordedAt)
            put("lat", p.lat)
            put("lon", p.lon)
            if (p.alt != null) put("alt", p.alt)
            if (p.accuracyM != null) put("accuracy", p.accuracyM)
            if (p.speedMps != null) put("speed", p.speedMps)
            if (p.bearingDeg != null) put("bearing", p.bearingDeg)
            put("provider", p.provider)
            put("is_mock", if (p.isMock) 1 else 0)
            if (p.batteryPct != null) put("battery", p.batteryPct)
        }
        writableDatabase.insertOrThrow("point", null, values)
    }

    fun lastPoint(segmentId: String): TrackPoint? =
        readableDatabase.query("point", null, "segment_id = ?", arrayOf(segmentId), null, null, "recorded_at DESC", "1")
            .use { c -> if (c.moveToFirst()) c.toPoint() else null }

    // ---- events

    fun insertEvent(e: TrackEvent) {
        val values = ContentValues().apply {
            put("id", e.id)
            put("type", e.type)
            put("occurred_at", e.occurredAt)
            put("session_id", e.sessionId)
            put("payload", e.payload)
        }
        writableDatabase.insertOrThrow("event", null, values)
    }

    // ---- sync outbox

    fun unsyncedSessions(limit: Int): List<Session> =
        readableDatabase.query("session", null, "synced = 0", null, null, null, "started_at", limit.toString())
            .use { c -> c.mapAll { it.toSession() } }

    fun unsyncedSegments(limit: Int): List<Segment> =
        readableDatabase.query("segment", null, "synced = 0", null, null, null, "started_at", limit.toString())
            .use { c -> c.mapAll { it.toSegment() } }

    fun unsyncedPoints(limit: Int): List<TrackPoint> =
        readableDatabase.query("point", null, "synced = 0", null, null, null, "recorded_at", limit.toString())
            .use { c -> c.mapAll { it.toPoint() } }

    fun unsyncedEvents(limit: Int): List<TrackEvent> =
        readableDatabase.query("event", null, "synced = 0", null, null, null, "occurred_at", limit.toString())
            .use { c -> c.mapAll { it.toEvent() } }

    fun unsyncedCount(): Int {
        val db = readableDatabase
        val sql = "SELECT (SELECT count(*) FROM session WHERE synced = 0)" +
            " + (SELECT count(*) FROM segment WHERE synced = 0)" +
            " + (SELECT count(*) FROM point WHERE synced = 0)" +
            " + (SELECT count(*) FROM event WHERE synced = 0)"
        return db.rawQuery(sql, null).use { c -> if (c.moveToFirst()) c.getInt(0) else 0 }
    }

    /** Sessions are only marked when unchanged since they were read, so a later edit is uploaded again. */
    fun markSessionsSynced(sessions: List<Session>) {
        val db = writableDatabase
        db.beginTransaction()
        try {
            for (s in sessions) {
                db.execSQL(
                    "UPDATE session SET synced = 1 WHERE id = ? AND updated_at <= ?",
                    arrayOf<Any>(s.id, s.updatedAt),
                )
            }
            db.setTransactionSuccessful()
        } finally {
            db.endTransaction()
        }
    }

    fun markSynced(table: String, ids: Collection<String>) {
        require(table == "segment" || table == "point" || table == "event") { "unexpected table $table" }
        if (ids.isEmpty()) return
        val db = writableDatabase
        db.beginTransaction()
        try {
            ids.chunked(SQLITE_MAX_VARIABLES).forEach { chunk ->
                val placeholders = chunk.joinToString(",") { "?" }
                db.execSQL("UPDATE $table SET synced = 1 WHERE id IN ($placeholders)", chunk.toTypedArray())
            }
            db.setTransactionSuccessful()
        } finally {
            db.endTransaction()
        }
    }

    // ---- row mapping

    private fun Session.toValues() = ContentValues().apply {
        put("id", id)
        put("status", status)
        put("started_at", startedAt)
        if (endedAt != null) put("ended_at", endedAt)
        put("name", name)
        put("distance_m", distanceM)
        put("moving_time_s", movingTimeS)
        put("point_count", pointCount)
        put("synced", if (synced) 1 else 0)
        put("updated_at", updatedAt)
    }

    private fun Cursor.toSession() = Session(
        id = getString(getColumnIndexOrThrow("id")),
        status = getString(getColumnIndexOrThrow("status")),
        startedAt = getLong(getColumnIndexOrThrow("started_at")),
        endedAt = getLongOrNull("ended_at"),
        name = getStringOrNull("name"),
        distanceM = getDouble(getColumnIndexOrThrow("distance_m")),
        movingTimeS = getLong(getColumnIndexOrThrow("moving_time_s")),
        pointCount = getInt(getColumnIndexOrThrow("point_count")),
        synced = getInt(getColumnIndexOrThrow("synced")) != 0,
        updatedAt = getLong(getColumnIndexOrThrow("updated_at")),
    )

    private fun Cursor.toSegment() = Segment(
        id = getString(getColumnIndexOrThrow("id")),
        sessionId = getString(getColumnIndexOrThrow("session_id")),
        seq = getInt(getColumnIndexOrThrow("seq")),
        startedAt = getLong(getColumnIndexOrThrow("started_at")),
        endedAt = getLongOrNull("ended_at"),
    )

    private fun Cursor.toPoint() = TrackPoint(
        id = getString(getColumnIndexOrThrow("id")),
        sessionId = getString(getColumnIndexOrThrow("session_id")),
        segmentId = getString(getColumnIndexOrThrow("segment_id")),
        recordedAt = getLong(getColumnIndexOrThrow("recorded_at")),
        lat = getDouble(getColumnIndexOrThrow("lat")),
        lon = getDouble(getColumnIndexOrThrow("lon")),
        alt = getDoubleOrNull("alt"),
        accuracyM = getDoubleOrNull("accuracy")?.toFloat(),
        speedMps = getDoubleOrNull("speed")?.toFloat(),
        bearingDeg = getDoubleOrNull("bearing")?.toFloat(),
        provider = getStringOrNull("provider"),
        isMock = getInt(getColumnIndexOrThrow("is_mock")) != 0,
        batteryPct = getLongOrNull("battery")?.toInt(),
    )

    private fun Cursor.toEvent() = TrackEvent(
        id = getString(getColumnIndexOrThrow("id")),
        type = getString(getColumnIndexOrThrow("type")),
        occurredAt = getLong(getColumnIndexOrThrow("occurred_at")),
        sessionId = getStringOrNull("session_id"),
        payload = getStringOrNull("payload"),
    )

    private fun Cursor.getLongOrNull(column: String): Long? {
        val i = getColumnIndexOrThrow(column)
        return if (isNull(i)) null else getLong(i)
    }

    private fun Cursor.getDoubleOrNull(column: String): Double? {
        val i = getColumnIndexOrThrow(column)
        return if (isNull(i)) null else getDouble(i)
    }

    private fun Cursor.getStringOrNull(column: String): String? {
        val i = getColumnIndexOrThrow(column)
        return if (isNull(i)) null else getString(i)
    }

    private fun <T> Cursor.mapAll(map: (Cursor) -> T): List<T> {
        val result = ArrayList<T>(count)
        while (moveToNext()) result.add(map(this))
        return result
    }

    companion object {
        const val STATUS_ACTIVE = "active"
        const val STATUS_PAUSED = "paused"
        const val STATUS_STOPPED = "stopped"

        private const val DB_NAME = "syria_tracking.db"
        private const val DB_VERSION = 1
        private const val SQLITE_MAX_VARIABLES = 500

        @Volatile
        private var instance: TrackingDb? = null

        @JvmStatic
        fun get(context: Context): TrackingDb =
            instance ?: synchronized(this) {
                instance ?: TrackingDb(context.applicationContext).also { instance = it }
            }
    }
}
