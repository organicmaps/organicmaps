package app.organicmaps.sdk.sync

import android.app.PendingIntent
import android.content.Intent

sealed interface AuthResult {
    data object Authorized : AuthResult

    /** The user has to pick an account or grant access: launch [intent] from an activity. */
    data class NeedsResolution(val intent: PendingIntent) : AuthResult

    data class Failed(val error: SyncError) : AuthResult
}

/** Bytes used in the account and its limit, null when it has none. */
data class StorageQuota(val usage: Long, val limit: Long?)

interface CloudSyncProvider {
    /** Names the provider's state file, like "google_drive". */
    val id: String

    /** Authorizes the synced account without UI if possible. */
    suspend fun authorize(): AuthResult

    /** [switchingTo] is the account passed to [switchAccount] when the resolution came from it. */
    suspend fun handleAuthorizationResult(data: Intent?, switchingTo: String?): AuthResult

    /** Lets the user pick another account; the picked name goes to [switchAccount]. */
    fun chooseAccountIntent(): Intent

    /** Syncs with [accountName] once it is authorized; until then, and on failure, the current account stays. */
    suspend fun switchAccount(accountName: String): AuthResult

    fun cloudStore(): CloudStore

    /** The synced account; fetched only while it is not known yet. */
    suspend fun accountEmail(): String?

    /** @return null when it can't be fetched now. */
    suspend fun storageQuota(): StorageQuota?

    /** Opens the account's storage management. */
    fun manageStorageIntent(): Intent

    /** Stops using the account on this device; other devices of the account keep syncing. */
    fun signOut()
}

/** Holds the provider that a flavor-specific module registers at startup; empty means no sync. */
object CloudSyncRegistry {
    private var factory: (() -> CloudSyncProvider?)? = null
    private var isCreated = false
    private var instance: CloudSyncProvider? = null

    /** Created on first use, so a provider that sync never needs costs nothing at startup. */
    val provider: CloudSyncProvider?
        @Synchronized get() {
            val factory = factory
            if (!isCreated && factory != null) {
                instance = factory()
                isCreated = true
            }
            return instance
        }

    /** [factory] returns null when the provider can't work on this device. */
    @Synchronized
    fun register(factory: () -> CloudSyncProvider?) {
        this.factory = factory
        isCreated = false
        instance = null
    }
}
