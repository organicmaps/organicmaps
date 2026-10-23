package app.organicmaps.sdk.sync.gms

import app.organicmaps.sdk.sync.SyncError
import java.net.HttpURLConnection.HTTP_INTERNAL_ERROR
import java.net.HttpURLConnection.HTTP_UNAUTHORIZED
import org.json.JSONObject

/** Maps a Drive API error response to [SyncError]. */
object DriveErrors {
    // java.net.HttpURLConnection has no constant for it.
    private const val HTTP_TOO_MANY_REQUESTS = 429

    fun map(code: Int, body: String): SyncError {
        val error = runCatching { JSONObject(body).getJSONObject("error") }.getOrNull()
        val reason = error?.optJSONArray("errors")?.optJSONObject(0)?.optString("reason")
        return when {
            reason == "storageQuotaExceeded" -> SyncError.QuotaExceeded
            reason == "rateLimitExceeded" || reason == "userRateLimitExceeded" -> SyncError.Transient
            code == HTTP_UNAUTHORIZED -> SyncError.AuthRequired
            code == HTTP_TOO_MANY_REQUESTS || code >= HTTP_INTERNAL_ERROR -> SyncError.Transient
            else -> SyncError.Other(error?.optString("message").orEmpty().ifEmpty { "HTTP $code" })
        }
    }
}
