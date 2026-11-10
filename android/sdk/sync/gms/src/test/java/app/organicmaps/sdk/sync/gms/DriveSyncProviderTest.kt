package app.organicmaps.sdk.sync.gms

import android.app.PendingIntent
import android.content.Intent
import app.organicmaps.sdk.sync.AuthResult
import app.organicmaps.sdk.sync.SyncError
import kotlinx.coroutines.test.runTest
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test
import org.mockito.Mockito.mock

class DriveSyncProviderTest {
    private val authorizer = FakeAuthorizer(accountEmail = ACCOUNT_A)
    private val provider = DriveSyncProvider(authorizer)
    private val needsConsent = AuthResult.NeedsResolution(mock(PendingIntent::class.java))

    @Test
    fun authorize_usesSyncedAccount() = runTest {
        authorizer.results += AuthResult.Authorized

        provider.authorize()

        assertEquals(listOf(ACCOUNT_A), authorizer.authorizedWith)
    }

    @Test
    fun switchAccount_authorized_switches() = runTest {
        authorizer.results += AuthResult.Authorized

        assertEquals(AuthResult.Authorized, provider.switchAccount(ACCOUNT_B))

        assertEquals(listOf(ACCOUNT_B), authorizer.authorizedWith)
        assertEquals(ACCOUNT_B, provider.accountEmail())
    }

    @Test
    fun switchAccount_needsConsent_switchesAfterIt() = runTest {
        authorizer.results += needsConsent

        assertEquals(needsConsent, provider.switchAccount(ACCOUNT_B))
        assertEquals(ACCOUNT_A, provider.accountEmail())

        assertEquals(AuthResult.Authorized, provider.handleAuthorizationResult(null, switchingTo = ACCOUNT_B))
        assertEquals(ACCOUNT_B, provider.accountEmail())
    }

    @Test
    fun switchAccount_backgroundPassDuringConsent_stillSwitches() = runTest {
        authorizer.results += needsConsent
        provider.switchAccount(ACCOUNT_B)
        authorizer.results += AuthResult.Authorized
        provider.authorize()

        provider.handleAuthorizationResult(null, switchingTo = ACCOUNT_B)

        assertEquals(ACCOUNT_B, provider.accountEmail())
    }

    @Test
    fun switchAccount_consentDenied_keepsAccount() = runTest {
        authorizer.results += needsConsent
        provider.switchAccount(ACCOUNT_B)
        authorizer.intentResult = AuthResult.Failed(SyncError.Other("denied"))

        provider.handleAuthorizationResult(null, switchingTo = ACCOUNT_B)

        assertEquals(ACCOUNT_A, provider.accountEmail())
    }

    @Test
    fun signIn_afterAbandonedSwitch_keepsAccount() = runTest {
        authorizer.results += needsConsent
        provider.switchAccount(ACCOUNT_B)
        authorizer.results += needsConsent
        provider.authorize()

        provider.handleAuthorizationResult(null, switchingTo = null)

        assertEquals(ACCOUNT_A, provider.accountEmail())
    }

    @Test
    fun switchAccount_failed_keepsAccount() = runTest {
        authorizer.results += AuthResult.Failed(SyncError.Transient)

        assertEquals(AuthResult.Failed(SyncError.Transient), provider.switchAccount(ACCOUNT_B))

        assertEquals(ACCOUNT_A, provider.accountEmail())
    }

    @Test
    fun switchAccount_sameAccount_doesNotAuthorizeAgain() = runTest {
        assertEquals(AuthResult.Authorized, provider.switchAccount(ACCOUNT_A))

        assertTrue(authorizer.authorizedWith.isEmpty())
    }

    @Test
    fun signOut_forgetsAccountOnThisDeviceOnly() {
        provider.signOut()

        assertTrue(authorizer.isSignedOut)
        assertNull(authorizer.accountEmail)
    }

    private class FakeAuthorizer(override var accountEmail: String?) : Authorizer {
        val results = ArrayDeque<AuthResult>()
        var intentResult: AuthResult = AuthResult.Authorized
        val authorizedWith = mutableListOf<String?>()
        var isSignedOut = false

        override suspend fun authorize(email: String?): AuthResult {
            authorizedWith += email
            return results.removeFirst()
        }

        override fun resultFromIntent(data: Intent?) = intentResult

        override fun signOut() {
            isSignedOut = true
            accountEmail = null
        }

        override suspend fun token(): String = error("no network in these tests")

        override suspend fun invalidate(token: String) = Unit
    }

    private companion object {
        const val ACCOUNT_A = "a@example.com"
        const val ACCOUNT_B = "b@example.com"
    }
}
