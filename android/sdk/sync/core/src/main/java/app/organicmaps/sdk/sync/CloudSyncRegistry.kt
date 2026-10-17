package app.organicmaps.sdk.sync

import android.app.PendingIntent
import android.content.Intent

sealed interface AuthResult {
    data object Authorized : AuthResult

    /** The user has to pick an account or grant access: launch [intent] from an activity. */
    data class NeedsResolution(val intent: PendingIntent) : AuthResult

    data class Failed(val error: SyncError) : AuthResult
}

interface CloudSyncProvider {
    /** Names the provider's state file, like "google_drive". */
    val id: String

    /** Authorizes without UI if possible. */
    suspend fun authorize(): AuthResult

    fun handleAuthorizationResult(data: Intent?): AuthResult

    fun cloudStore(): CloudStore

    suspend fun accountEmail(): String?

    suspend fun signOut()
}

/** Holds the provider that a flavor-specific module registers at startup; empty means no sync. */
object CloudSyncRegistry {
    @Volatile
    var provider: CloudSyncProvider? = null
        private set

    fun register(provider: CloudSyncProvider) {
        this.provider = provider
    }
}
