package app.organicmaps.sdk.sync

import android.content.Intent
import org.junit.Assert.assertNull
import org.junit.Assert.assertSame
import org.junit.Test

class CloudSyncRegistryTest {
    @Test
    fun register_setsProvider() {
        assertNull(CloudSyncRegistry.provider)
        val provider = StubProvider()

        CloudSyncRegistry.register(provider)

        assertSame(provider, CloudSyncRegistry.provider)
    }

    private class StubProvider : CloudSyncProvider {
        override val id = "stub"

        override suspend fun authorize() = AuthResult.Authorized

        override fun handleAuthorizationResult(data: Intent?) = AuthResult.Authorized

        override fun cloudStore() = error("not used")

        override suspend fun accountEmail(): String? = null

        override suspend fun signOut() = Unit
    }
}
