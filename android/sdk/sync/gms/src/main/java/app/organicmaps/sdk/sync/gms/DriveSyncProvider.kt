package app.organicmaps.sdk.sync.gms

import android.content.Context
import android.content.Intent
import app.organicmaps.sdk.sync.AuthResult
import app.organicmaps.sdk.sync.CloudException
import app.organicmaps.sdk.sync.CloudStore
import app.organicmaps.sdk.sync.CloudSyncProvider
import okhttp3.OkHttpClient

class DriveSyncProvider(context: Context) : CloudSyncProvider {
    private val authorizer = DriveAuthorizer(context.applicationContext)
    private val store = DriveCloudStore(OkHttpClient(), authorizer)

    override val id = "google_drive"

    override suspend fun authorize(): AuthResult = authorizer.authorize()

    override fun handleAuthorizationResult(data: Intent?): AuthResult = authorizer.resultFromIntent(data)

    override fun cloudStore(): CloudStore = store

    override suspend fun accountEmail(): String? = try {
        store.email()
    } catch (ignored: CloudException) {
        null
    }

    override suspend fun signOut() {
        val email = accountEmail() ?: return
        try {
            authorizer.revoke(email)
        } catch (ignored: CloudException) {
            // Sync is disabled anyway; the grant stays listed in the Google account until the user removes it.
        }
    }
}
