package app.organicmaps.sdk.wear

import app.organicmaps.wear.protocol.WearNavigationDetails

/** Phone-side port for pushing transient navigation details to a paired Wear OS device. */
fun interface WearNavigationDetailsPublisher {
    fun publish(details: WearNavigationDetails)
}
