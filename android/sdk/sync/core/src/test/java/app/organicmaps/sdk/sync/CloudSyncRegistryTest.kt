package app.organicmaps.sdk.sync

import android.content.Intent
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Assert.assertSame
import org.junit.Test

class CloudSyncRegistryTest {
    @Test
    fun provider_isCreatedOnFirstUseOnly() {
        var created = 0
        val provider = StubProvider()
        CloudSyncRegistry.register {
            ++created
            provider
        }
        assertEquals(0, created)

        assertSame(provider, CloudSyncRegistry.provider)
        assertSame(provider, CloudSyncRegistry.provider)
        assertEquals(1, created)
    }

    @Test
    fun provider_unavailable_isNull() {
        CloudSyncRegistry.register { null }

        assertNull(CloudSyncRegistry.provider)
    }

    private class StubProvider : CloudSyncProvider {
        override val id = "stub"

        override suspend fun authorize() = AuthResult.Authorized

        override suspend fun handleAuthorizationResult(data: Intent?, switchingTo: String?) = AuthResult.Authorized

        override fun chooseAccountIntent() = error("not used")

        override suspend fun switchAccount(accountName: String) = AuthResult.Authorized

        override fun cloudStore() = error("not used")

        override suspend fun accountEmail(): String? = null

        override suspend fun storageQuota(): StorageQuota? = null

        override fun manageStorageIntent() = error("not used")

        override fun signOut() = Unit
    }
}
