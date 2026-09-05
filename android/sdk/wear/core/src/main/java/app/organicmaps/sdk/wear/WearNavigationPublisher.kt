package app.organicmaps.sdk.wear

import app.organicmaps.wear.protocol.WearNavigationState

/**
 * Phone-side port for pushing navigation state to a paired Wear OS device.
 *
 * The real implementation (`:sdk:wear:gms`) talks to the Google Wear Data Layer and ships
 * in Google debug and beta only. It registers itself at startup through [WearBridge]; every
 * other variant keeps the no-op default, so callers in common, all-variants code never reference
 * Play services types.
 */
fun interface WearNavigationPublisher {
    fun publish(state: WearNavigationState)
}
