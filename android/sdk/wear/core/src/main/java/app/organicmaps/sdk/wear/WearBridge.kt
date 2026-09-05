package app.organicmaps.sdk.wear

import app.organicmaps.wear.protocol.WearNavigationState

/**
 * Routes navigation state to a paired Wear OS device.
 *
 * Google debug and beta bundle `:sdk:wear:gms`, whose manifest-merged `ContentProvider`
 * registers the real publisher at startup. Everywhere else -- Google release and profileable,
 * F-Droid, Huawei, Web -- nothing registers and the bridge falls back to a no-op, so callers in
 * common code never reference Play services types.
 */
object WearBridge {
    private val NO_OP = WearNavigationPublisher {}

    @Volatile
    private var publisher: WearNavigationPublisher = NO_OP

    /** Called once at process startup from the Google Wear module. */
    @JvmStatic
    fun register(publisher: WearNavigationPublisher) {
        this.publisher = publisher
    }

    /** Maps a navigation-active flag to a published state; suitable as a routing state listener. */
    @JvmStatic
    fun publishNavigating(navigating: Boolean) {
        publisher.publish(
            if (navigating) WearNavigationState.navigation() else WearNavigationState.normal(),
        )
    }
}
