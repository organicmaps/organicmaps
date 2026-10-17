package app.organicmaps.sdk.wear

import app.organicmaps.wear.protocol.WearNavigationDetails
import app.organicmaps.wear.protocol.WearNavigationMode

/**
 * Routes navigation data to a paired Wear OS device.
 *
 * Google debug and beta bundle `:sdk:wear:gms`, whose manifest-merged `ContentProvider`
 * registers the real publishers at startup. Everywhere else -- Google release and profileable,
 * F-Droid, Huawei, Web -- nothing registers and the bridge falls back to no-ops, so callers in
 * common code never reference Play services types.
 */
object WearBridge {
    private val NO_OP_MODE = WearNavigationPublisher {}
    private val NO_OP_DETAILS = WearNavigationDetailsPublisher {}

    @Volatile
    private var modePublisher: WearNavigationPublisher = NO_OP_MODE

    @Volatile
    private var detailsPublisher: WearNavigationDetailsPublisher = NO_OP_DETAILS

    /** Called once at process startup from the Google Wear module. */
    fun register(publisher: WearNavigationPublisher) {
        modePublisher = publisher
    }

    /** Called once at process startup from the Google Wear module. */
    fun registerDetails(publisher: WearNavigationDetailsPublisher) {
        detailsPublisher = publisher
    }

    /** Maps a navigation-active flag to a published persistent mode. */
    @JvmStatic
    fun publishNavigating(navigating: Boolean) {
        val mode =
            if (navigating) WearNavigationMode.NAVIGATION else WearNavigationMode.NORMAL

        modePublisher.publish(mode)
    }

    /** Publishes transient details while navigation is active. */
    @JvmStatic
    fun publishDetails(details: WearNavigationDetails) {
        detailsPublisher.publish(details)
    }
}
