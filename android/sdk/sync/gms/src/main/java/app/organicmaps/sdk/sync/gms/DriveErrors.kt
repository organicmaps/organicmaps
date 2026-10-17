package app.organicmaps.sdk.sync.gms

import app.organicmaps.sdk.sync.SyncError
import org.json.JSONObject

/** Maps a Drive API error response to [SyncError]. */
object DriveErrors {
    private const val HTTP_UNAUTHORIZED = 401
    private const val HTTP_TOO_MANY_REQUESTS = 429
    private const val HTTP_SERVER_ERROR = 500

    fun map(code: Int, body: String): SyncError {
        val error = runCatching { JSONObject(body).getJSONObject("error") }.getOrNull()
        val reason = error?.optJSONArray("errors")?.optJSONObject(0)?.optString("reason")
        return when {
            reason == "storageQuotaExceeded" -> SyncError.QuotaExceeded
            reason == "rateLimitExceeded" || reason == "userRateLimitExceeded" -> SyncError.Transient
            code == HTTP_UNAUTHORIZED -> SyncError.AuthRequired
            code == HTTP_TOO_MANY_REQUESTS || code >= HTTP_SERVER_ERROR -> SyncError.Transient
            else -> SyncError.Other(error?.optString("message").orEmpty().ifEmpty { "HTTP $code" })
        }
    }
}
