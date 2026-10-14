package app.organicmaps.sdk.sync.gms

import android.accounts.Account
import android.content.Context
import android.content.Intent
import androidx.core.content.edit
import app.organicmaps.sdk.sync.AuthResult
import app.organicmaps.sdk.sync.CloudException
import app.organicmaps.sdk.sync.SyncError
import com.google.android.gms.auth.api.identity.AuthorizationRequest
import com.google.android.gms.auth.api.identity.AuthorizationResult
import com.google.android.gms.auth.api.identity.ClearTokenRequest
import com.google.android.gms.auth.api.identity.Identity
import com.google.android.gms.common.api.ApiException
import com.google.android.gms.common.api.CommonStatusCodes
import com.google.android.gms.common.api.Scope
import com.google.android.gms.tasks.Task
import kotlin.coroutines.resume
import kotlin.coroutines.resumeWithException
import kotlinx.coroutines.suspendCancellableCoroutine

/** Access to the app's hidden Drive folder through Google Play services; no client id or backend is needed. */
class DriveAuthorizer(context: Context) : Authorizer {
    private val client = Identity.getAuthorizationClient(context)
    private val prefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
    private val scopes = listOf(Scope(DRIVE_APPDATA_SCOPE))

    override var accountEmail: String?
        get() = prefs.getString(KEY_ACCOUNT, null)
        set(value) = prefs.edit { putString(KEY_ACCOUNT, value) }

    @Volatile
    private var accessToken: String? = null

    override suspend fun authorize(email: String?): AuthResult = try {
        val request = AuthorizationRequest.builder().setRequestedScopes(scopes)
        email?.let { request.setAccount(Account(it, GOOGLE_ACCOUNT_TYPE)) }
        handle(client.authorize(request.build()).await())
    } catch (e: ApiException) {
        AuthResult.Failed(e.toSyncError())
    }

    override fun resultFromIntent(data: Intent?): AuthResult = try {
        handle(client.getAuthorizationResultFromIntent(data))
    } catch (e: ApiException) {
        AuthResult.Failed(e.toSyncError())
    }

    override suspend fun token(): String {
        accessToken?.let { return it }
        return when (val result = authorize()) {
            AuthResult.Authorized -> accessToken ?: throw CloudException(SyncError.AuthRequired)
            is AuthResult.NeedsResolution -> throw CloudException(SyncError.AuthRequired)
            is AuthResult.Failed -> throw CloudException(result.error)
        }
    }

    override suspend fun invalidate(token: String) {
        accessToken = null
        try {
            client.clearToken(ClearTokenRequest.builder().setToken(token).build()).await()
        } catch (e: ApiException) {
            throw CloudException(e.toSyncError(), e)
        }
    }

    override fun signOut() {
        accessToken = null
        accountEmail = null
    }

    private fun handle(result: AuthorizationResult): AuthResult {
        val pendingIntent = result.pendingIntent
        if (result.hasResolution() && pendingIntent != null) return AuthResult.NeedsResolution(pendingIntent)
        accessToken = result.accessToken
        return AuthResult.Authorized
    }

    private fun ApiException.toSyncError(): SyncError =
        if (statusCode == CommonStatusCodes.NETWORK_ERROR) SyncError.Transient else SyncError.Other(message.orEmpty())

    private companion object {
        const val DRIVE_APPDATA_SCOPE = "https://www.googleapis.com/auth/drive.appdata"
        const val PREFS_NAME = "google_drive_sync"
        const val KEY_ACCOUNT = "account"
    }
}

internal const val GOOGLE_ACCOUNT_TYPE = "com.google"

private suspend fun <T> Task<T>.await(): T = suspendCancellableCoroutine { continuation ->
    addOnSuccessListener { continuation.resume(it) }
    addOnFailureListener { continuation.resumeWithException(it) }
    addOnCanceledListener { continuation.cancel() }
}
