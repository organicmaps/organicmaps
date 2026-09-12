package app.organicmaps.wear.protocol

/** Persistent navigation mode mirrored from the phone to the watch. */
class WearNavigationState private constructor(val mode: WearNavigationMode) {
    companion object {
        fun normal() = WearNavigationState(WearNavigationMode.NORMAL)

        fun navigation() = WearNavigationState(WearNavigationMode.NAVIGATION)
    }
}
