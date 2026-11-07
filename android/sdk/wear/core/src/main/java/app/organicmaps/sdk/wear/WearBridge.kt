package app.organicmaps.sdk.wear

import app.organicmaps.wear.protocol.WearNavigationDetails
import app.organicmaps.wear.protocol.WearNavigationMode

/**
 * Routes navigation to Google Wear when its publisher is registered by the GMS variant.
 * All other app variants use the same no-op implementation.
 */
object WearBridge {
    private val NO_OP =
        object : WearNavigationPublisher {
            override fun publish(mode: WearNavigationMode) = Unit

            override fun publish(details: WearNavigationDetails) = Unit

            override fun invalidateDetails() = Unit
        }

    @Volatile private var publisher: WearNavigationPublisher = NO_OP

    /** Called once at process startup from the Google Wear module. */
    fun register(implementation: WearNavigationPublisher) {
        publisher = implementation
    }

    @JvmStatic
    fun publishNavigating(navigating: Boolean) {
        publisher.publish(if (navigating) WearNavigationMode.NAVIGATION else WearNavigationMode.NORMAL)
    }

    @JvmStatic
    fun publishDetails(details: WearNavigationDetails) {
        publisher.publish(details)
    }

    @JvmStatic
    fun invalidateDetails() {
        publisher.invalidateDetails()
    }
}
