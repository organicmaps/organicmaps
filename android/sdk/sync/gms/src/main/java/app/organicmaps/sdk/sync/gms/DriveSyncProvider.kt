package app.organicmaps.sdk.sync.gms

import android.accounts.Account
import android.content.Context
import android.content.Intent
import androidx.core.net.toUri
import app.organicmaps.sdk.sync.AuthResult
import app.organicmaps.sdk.sync.CloudException
import app.organicmaps.sdk.sync.CloudStore
import app.organicmaps.sdk.sync.CloudSyncProvider
import app.organicmaps.sdk.sync.StorageQuota
import com.google.android.gms.common.AccountPicker
import okhttp3.OkHttpClient

class DriveSyncProvider internal constructor(
    private val authorizer: Authorizer,
    private val store: DriveCloudStore = DriveCloudStore(OkHttpClient(), authorizer),
) : CloudSyncProvider {
    constructor(context: Context) : this(DriveAuthorizer(context.applicationContext))

    override val id = "google_drive"

    override suspend fun authorize(): AuthResult = authorizer.authorize()

    override suspend fun handleAuthorizationResult(data: Intent?, switchingTo: String?): AuthResult {
        val result = authorizer.resultFromIntent(data)
        if (result == AuthResult.Authorized && switchingTo != null) authorizer.accountEmail = switchingTo
        return result
    }

    override fun chooseAccountIntent(): Intent = AccountPicker.newChooseAccountIntent(
        AccountPicker.AccountChooserOptions.Builder()
            .setAllowableAccountsTypes(listOf(GOOGLE_ACCOUNT_TYPE))
            .setSelectedAccount(authorizer.accountEmail?.let { Account(it, GOOGLE_ACCOUNT_TYPE) })
            .setAlwaysShowAccountPicker(true)
            .build(),
    )

    override suspend fun switchAccount(accountName: String): AuthResult {
        if (accountName == authorizer.accountEmail) return AuthResult.Authorized
        val result = authorizer.authorize(accountName)
        if (result == AuthResult.Authorized) authorizer.accountEmail = accountName
        return result
    }

    override fun cloudStore(): CloudStore = store

    override suspend fun accountEmail(): String? = authorizer.accountEmail ?: try {
        store.email().also { authorizer.accountEmail = it }
    } catch (ignored: CloudException) {
        null
    }

    override suspend fun storageQuota(): StorageQuota? = try {
        store.storageQuota()
    } catch (ignored: CloudException) {
        null
    }

    override fun manageStorageIntent() = Intent(Intent.ACTION_VIEW, MANAGE_STORAGE_URL.toUri())

    // Revoking the access would end it for the app on every device of the account, not only on this one.
    override fun signOut() = authorizer.signOut()

    private companion object {
        // Opens Google One when it is installed.
        const val MANAGE_STORAGE_URL = "https://one.google.com/storage"
    }
}
