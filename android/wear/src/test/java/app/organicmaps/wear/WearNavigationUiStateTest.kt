package app.organicmaps.wear

import app.organicmaps.wear.protocol.WearNavigationDetails
import app.organicmaps.wear.protocol.WearNavigationMode
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

class WearNavigationUiStateTest {
    private fun details(street: String) = WearNavigationDetails(nextStreet = street)

    @Test
    fun pauseClearsGuidanceAndRejectsMessagesUntilResumed() {
        val state = WearNavigationUiState()
        val oldGeneration = state.resume()
        state.setMode(WearNavigationMode.NAVIGATION)
        assertTrue(state.acceptDetails(details("Old street")))
        state.pause()
        assertNull(state.details)
        assertFalse(state.acceptDetails(details("Paused street")))
        val newGeneration = state.resume()
        assertFalse(state.isCurrent(oldGeneration))
        assertTrue(state.isCurrent(newGeneration))
        assertTrue(state.acceptDetails(details("New street")))
        assertEquals("New street", state.details?.nextStreet)
    }

    @Test
    fun newerMessageReplacesGuidanceUntilTimerExpires() {
        val state = WearNavigationUiState()
        state.resume()
        state.setMode(WearNavigationMode.NAVIGATION)
        assertTrue(state.acceptDetails(details("First")))
        assertTrue(state.acceptDetails(details("Second")))
        assertEquals("Second", state.details?.nextStreet)
        assertTrue(state.expireDetails())
        assertNull(state.details)
    }

    @Test
    fun normalModeClearsAndRejectsDetails() {
        val state = WearNavigationUiState()
        state.resume()
        state.setMode(WearNavigationMode.NAVIGATION)
        assertTrue(state.acceptDetails(details("Old route")))
        state.setMode(WearNavigationMode.NORMAL)
        assertNull(state.details)
        assertFalse(state.acceptDetails(details("Stale route")))
        assertFalse(state.expireDetails())
    }

    @Test
    fun expiryAfterPauseDoesNotRestoreOrClearAnything() {
        val state = WearNavigationUiState()
        state.resume()
        state.setMode(WearNavigationMode.NAVIGATION)
        assertTrue(state.acceptDetails(details("Old")))
        state.pause()
        assertFalse(state.expireDetails())
        assertNull(state.details)
    }
}
