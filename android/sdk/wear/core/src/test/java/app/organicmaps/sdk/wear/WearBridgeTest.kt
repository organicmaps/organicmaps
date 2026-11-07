package app.organicmaps.sdk.wear

import app.organicmaps.wear.protocol.WearNavigationDetails
import app.organicmaps.wear.protocol.WearNavigationMode
import org.junit.Assert.assertEquals
import org.junit.Test

class WearBridgeTest {
    @Test
    fun forwardsOrderedNavigationLifecycleToOnePublisher() {
        val events = mutableListOf<String>()
        val publisher = RecordingPublisher(events)
        WearBridge.register(publisher)
        WearBridge.publishNavigating(true)
        WearBridge.publishDetails(WearNavigationDetails(nextStreet = "Rue Victor Hugo"))
        WearBridge.invalidateDetails()
        WearBridge.publishNavigating(false)

        assertEquals(
            listOf("NAVIGATION", "Rue Victor Hugo", "invalidated", "NORMAL"),
            events,
        )
    }

    @Test
    fun registrationReplacesPreviousPublisher() {
        val previous = mutableListOf<String>()
        val current = mutableListOf<String>()
        WearBridge.register(RecordingPublisher(previous))
        WearBridge.register(RecordingPublisher(current))
        WearBridge.publishNavigating(true)
        assertEquals(emptyList<String>(), previous)
        assertEquals(listOf("NAVIGATION"), current)
    }

    private class RecordingPublisher(private val events: MutableList<String>) : WearNavigationPublisher {
        override fun publish(mode: WearNavigationMode) {
            events += mode.name
        }

        override fun publish(details: WearNavigationDetails) {
            events += details.nextStreet.orEmpty()
        }

        override fun invalidateDetails() {
            events += "invalidated"
        }
    }
}
