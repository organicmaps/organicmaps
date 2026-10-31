package app.organicmaps.wear.protocol

/** Ephemeral navigation details sent from the phone while navigation is active. */
data class WearNavigationDetails(
    val distanceToTurn: WearDistance? = null,
    val nextStreet: String? = null,
    val remainingDistance: WearDistance? = null,
    val remainingTimeSeconds: Int? = null,
)
