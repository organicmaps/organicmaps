package app.organicmaps.sdk.wear

import app.organicmaps.wear.protocol.WearNavigationDetails
import app.organicmaps.wear.protocol.WearNavigationMode
import org.junit.Assert.assertEquals
import org.junit.Test

class WearBridgeTest {
    @Test
    fun publishesNavigationMode() {
        val publisher = RecordingPublisher()
        WearBridge.register(publisher)

        WearBridge.publishNavigating(true)

        assertEquals(WearNavigationMode.NAVIGATION, publisher.mode)
    }

    @Test
    fun publishesNormalMode() {
        val publisher = RecordingPublisher()
        WearBridge.register(publisher)

        WearBridge.publishNavigating(false)

        assertEquals(WearNavigationMode.NORMAL, publisher.mode)
    }

    @Test
    fun publishesNavigationDetails() {
        val publisher = RecordingDetailsPublisher()
        val details = WearNavigationDetails(nextStreet = "Place Victor Hugo")
        WearBridge.registerDetails(publisher)

        WearBridge.publishDetails(details)

        assertEquals(details, publisher.details)
    }

    @Test
    fun registrationReplacesPublisher() {
        val replaced = RecordingPublisher()
        val current = RecordingPublisher()
        WearBridge.register(replaced)
        WearBridge.register(current)

        WearBridge.publishNavigating(true)

        assertEquals(0, replaced.publishCount)
        assertEquals(1, current.publishCount)
    }

    private class RecordingDetailsPublisher : WearNavigationDetailsPublisher {
        var details: WearNavigationDetails? = null

        override fun publish(details: WearNavigationDetails) {
            this.details = details
        }
    }

    private class RecordingPublisher : WearNavigationPublisher {
        var mode: WearNavigationMode? = null
        var publishCount = 0

        override fun publish(mode: WearNavigationMode) {
            this.mode = mode
            ++publishCount
        }
    }
}
