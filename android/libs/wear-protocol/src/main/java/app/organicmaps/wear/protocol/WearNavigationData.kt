package app.organicmaps.wear.protocol

/**
 * Wire contract shared by the phone bridge and the watch app: Data Layer paths and payload keys,
 * plus a protocol version so the two independently-updatable APKs can detect a mismatch.
 */
object WearNavigationData {
    const val CAPABILITY_PHONE_APP = "organic_maps_phone_app"
    const val PATH_NAVIGATION_STATE = "/organicmaps/navigation/state"

    // Bump only for incompatible changes. Decoders ignore unknown keys, so additive keys need no bump;
    // a bump makes peers on the previous version treat the state as unavailable.
    const val VERSION = 1

    const val KEY_VERSION = "version"
    const val KEY_MODE = "mode"
}
