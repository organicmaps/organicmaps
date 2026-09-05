package app.organicmaps.wear.protocol

/**
 * Navigation state mirrored from the phone to the watch.
 *
 * The object keeps the current mode-only payload behind a state type, allowing the wire schema
 * and publisher contract to evolve without replacing the type passed between them.
 */
class WearNavigationState private constructor(val mode: WearNavigationMode) {
    companion object {
        fun of(mode: WearNavigationMode) = WearNavigationState(mode)

        fun normal() = of(WearNavigationMode.NORMAL)

        fun navigation() = of(WearNavigationMode.NAVIGATION)
    }
}
