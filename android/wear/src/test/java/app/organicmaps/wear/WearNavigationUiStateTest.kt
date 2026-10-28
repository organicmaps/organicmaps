package app.organicmaps.wear

import app.organicmaps.wear.protocol.WearNavigationDetails
import app.organicmaps.wear.protocol.WearNavigationMode
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

class WearNavigationUiStateTest {
    private fun details(street: String) = WearNavigationDetails(nextStreet = street)

    @Test
    fun pauseClearsRenderedSourceAndRejectsOldDeliveryAfterResume() {
        val state = WearNavigationUiState()
        val oldGeneration = state.resume()
        state.setMode(WearNavigationMode.NAVIGATION)
        assertNotNull(state.acceptDetails(details("Old street"), oldGeneration))
        state.pause()
        assertNull(state.details)
        assertFalse(state.resumed)

        val newGeneration = state.resume()
        assertNull(state.acceptDetails(details("Delayed old delivery"), oldGeneration))
        assertNull(state.details)
        assertNotNull(state.acceptDetails(details("Current street"), newGeneration))
        assertEquals("Current street", state.details?.nextStreet)
    }

    @Test
    fun secondMessageExtendsExpiryAndFirstTimerCannotClearIt() {
        val state = WearNavigationUiState()
        val generation = state.resume()
        state.setMode(WearNavigationMode.NAVIGATION)
        val firstExpiry = state.acceptDetails(details("First"), generation)!!
        val secondExpiry = state.acceptDetails(details("Second"), generation)!!
        assertFalse(state.expireDetails(firstExpiry))
        assertEquals("Second", state.details?.nextStreet)
        assertTrue(state.expireDetails(secondExpiry))
        assertNull(state.details)
    }

    @Test
    fun normalModeClearsOldRouteAndRejectsTransientGuidance() {
        val state = WearNavigationUiState()
        val generation = state.resume()
        state.setMode(WearNavigationMode.NAVIGATION)
        val oldTimer = state.acceptDetails(details("Cancelled route"), generation)!!
        state.setMode(WearNavigationMode.NORMAL)
        assertNull(state.details)
        assertNull(state.acceptDetails(details("Old packet"), generation))
        state.setMode(WearNavigationMode.NAVIGATION)
        assertFalse(state.expireDetails(oldTimer))
        assertNull(state.details)
        assertNotNull(state.acceptDetails(details("New route"), generation))
        assertEquals("New route", state.details?.nextStreet)
    }

    @Test
    fun delayedExpiryAfterPauseIsHarmless() {
        val state = WearNavigationUiState()
        val generation = state.resume()
        state.setMode(WearNavigationMode.NAVIGATION)
        val staleToken = state.acceptDetails(details("Old"), generation)!!
        state.pause()
        assertFalse(state.expireDetails(staleToken))
        state.resume()
        assertFalse(state.expireDetails(staleToken))
        assertNull(state.details)
    }
}
