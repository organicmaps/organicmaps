package app.organicmaps.wear.protocol

/** A phone-formatted distance value with its separately encoded display unit. */
data class WearDistance(val value: String, val unit: WearDistanceUnit)
