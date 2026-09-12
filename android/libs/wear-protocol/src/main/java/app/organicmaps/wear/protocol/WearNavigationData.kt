package app.organicmaps.wear.protocol

/**
 * Wire contract shared by the phone bridge and the watch app: Data Layer paths and payload keys,
 * plus a protocol version so the two independently-updatable APKs can detect a mismatch.
 */
object WearNavigationData {
    const val CAPABILITY_PHONE_APP = "organic_maps_phone_app"
    const val PATH_NAVIGATION_STATE = "/organicmaps/navigation/state"
    const val PATH_NAVIGATION_DETAILS = "/organicmaps/navigation/details"

    // Bump only for incompatible changes. Decoders ignore unknown keys, so additive keys need no bump;
    // a bump makes peers on the previous version treat the payload as unavailable.
    const val VERSION = 1

    const val KEY_VERSION = "version"
    const val KEY_MODE = "mode"

    const val KEY_DISTANCE_TO_TURN_VALUE = "distance_to_turn_value"
    const val KEY_DISTANCE_TO_TURN_UNIT = "distance_to_turn_unit"
    const val KEY_NEXT_STREET = "next_street"
    const val KEY_REMAINING_DISTANCE_VALUE = "remaining_distance_value"
    const val KEY_REMAINING_DISTANCE_UNIT = "remaining_distance_unit"
    const val KEY_REMAINING_TIME_SECONDS = "remaining_time_seconds"
}
