package app.organicmaps.sync

import androidx.work.NetworkType
import app.organicmaps.sdk.util.NetworkPolicy
import org.junit.Assert.assertEquals
import org.junit.Test

class BookmarksSyncWorkConstraintsTest {
    @Test
    fun alwaysUseMobileData_requiresConnected() {
        assertEquals(
            NetworkType.CONNECTED,
            BookmarksSync.networkTypeFor(NetworkPolicy.Type.ALWAYS, roamingAccepted = true),
        )
    }

    @Test
    fun alwaysUseMobileDataWithoutRoaming_requiresNotRoaming() {
        assertEquals(
            NetworkType.NOT_ROAMING,
            BookmarksSync.networkTypeFor(NetworkPolicy.Type.ALWAYS, roamingAccepted = false),
        )
    }

    @Test
    fun otherPolicies_requireUnmetered() {
        val policies =
            listOf(
                NetworkPolicy.Type.ASK,
                NetworkPolicy.Type.NEVER,
                NetworkPolicy.Type.TODAY,
                NetworkPolicy.Type.NOT_TODAY,
            )
        for (policy in policies) {
            assertEquals(
                policy.name,
                NetworkType.UNMETERED,
                BookmarksSync.networkTypeFor(policy, roamingAccepted = true),
            )
        }
    }
}
