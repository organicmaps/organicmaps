package app.organicmaps.sdk.wear

import app.organicmaps.wear.protocol.WearNavigationMode
import app.organicmaps.wear.protocol.WearNavigationState
import org.junit.Assert.assertEquals
import org.junit.Test

class WearBridgeTest {
    @Test
    fun publishesNavigationMode() {
        val publisher = RecordingPublisher()
        WearBridge.register(publisher)

        WearBridge.publishNavigating(true)

        assertEquals(WearNavigationMode.NAVIGATION, publisher.state?.mode)
    }

    @Test
    fun publishesNormalMode() {
        val publisher = RecordingPublisher()
        WearBridge.register(publisher)

        WearBridge.publishNavigating(false)

        assertEquals(WearNavigationMode.NORMAL, publisher.state?.mode)
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

    private class RecordingPublisher : WearNavigationPublisher {
        var state: WearNavigationState? = null
        var publishCount = 0

        override fun publish(state: WearNavigationState) {
            this.state = state
            ++publishCount
        }
    }
}
