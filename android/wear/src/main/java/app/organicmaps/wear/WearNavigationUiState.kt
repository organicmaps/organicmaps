package app.organicmaps.wear

import app.organicmaps.wear.protocol.WearNavigationDetails
import app.organicmaps.wear.protocol.WearNavigationMode

/**
 * Android-independent UI lifecycle policy. MainActivity supplies the actual timer and views.
 * A message posted before pause cannot update a newly resumed Activity; a superseded expiry
 * callback cannot clear later guidance.
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

    private var detailsToken = 0L

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

    /** Returns the expiry token if details were accepted, or null if an old callback arrived. */
    fun acceptDetails(value: WearNavigationDetails, expectedGeneration: Int): Long? {
        if (!isCurrent(expectedGeneration) || mode != WearNavigationMode.NAVIGATION) return null
        details = value
        return ++detailsToken
    }

    /** True only when the currently displayed details were cleared by this expiry. */
    fun expireDetails(expectedToken: Long): Boolean {
        val navigationVisible = resumed && mode == WearNavigationMode.NAVIGATION
        val detailsCurrent = details != null && detailsToken == expectedToken

        if (!navigationVisible || !detailsCurrent) {
            return false
        }
        clearDetails()
        return true
    }

    private fun clearDetails() {
        details = null
        detailsToken++
    }
}
