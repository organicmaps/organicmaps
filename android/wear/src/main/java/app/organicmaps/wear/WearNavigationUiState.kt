package app.organicmaps.wear

import app.organicmaps.wear.protocol.WearNavigationDetails
import app.organicmaps.wear.protocol.WearNavigationMode

/**
 * Android-independent navigation UI state. MessageClient and the resettable expiry Runnable
 * run on the main thread. Generations guard asynchronous registration and state-read Tasks.
 */
internal class WearNavigationUiState {
    var resumed = false
        private set
    var generation = 0
        private set
    var mode = WearNavigationMode.NORMAL
        private set
    var details: WearNavigationDetails? = null
        private set

    fun resume(): Int {
        resumed = true
        generation++
        return generation
    }

    fun pause() {
        resumed = false
        generation++
        clearDetails()
    }

    fun isCurrent(expectedGeneration: Int): Boolean = resumed && generation == expectedGeneration

    fun setMode(newMode: WearNavigationMode) {
        mode = newMode
        if (newMode == WearNavigationMode.NORMAL) clearDetails()
    }

    fun acceptDetails(value: WearNavigationDetails): Boolean {
        if (!resumed || mode != WearNavigationMode.NAVIGATION) return false
        details = value
        return true
    }

    fun expireDetails(): Boolean {
        if (!resumed || mode != WearNavigationMode.NAVIGATION || details == null) return false
        clearDetails()
        return true
    }

    private fun clearDetails() {
        details = null
    }
}
