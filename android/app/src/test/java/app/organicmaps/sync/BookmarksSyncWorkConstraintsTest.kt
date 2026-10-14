package app.organicmaps.sync

import androidx.work.NetworkType
import org.junit.Assert.assertEquals
import org.junit.Test

class BookmarksSyncWorkConstraintsTest {
    @Test
    fun mobileDataAllowed_requiresConnected() {
        assertEquals(
            NetworkType.CONNECTED,
            BookmarksSync.networkTypeFor(isMobileDataAllowed = true, roamingAccepted = true),
        )
    }

    @Test
    fun mobileDataAllowedWithoutRoaming_requiresNotRoaming() {
        assertEquals(
            NetworkType.NOT_ROAMING,
            BookmarksSync.networkTypeFor(isMobileDataAllowed = true, roamingAccepted = false),
        )
    }

    @Test
    fun mobileDataNotAllowed_requiresUnmetered() {
        assertEquals(
            NetworkType.UNMETERED,
            BookmarksSync.networkTypeFor(isMobileDataAllowed = false, roamingAccepted = true),
        )
    }
}
