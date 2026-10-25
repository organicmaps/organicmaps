package app.organicmaps.sdk.widgets.speedlimit

/**
 * Controls a [CombinedSpeedView]: updates the shown values and keeps the [Mode] of the widget in
 * sync with how much the current speed exceeds the speed limit, see [SpeedThresholds].
 *
 * Starts with an unknown speed limit, hiding the limit sign and resetting the mode to [Mode.Normal].
 *
 * The speed, the limit and the thresholds must be in the same units, the ones shown by [units].
 */
class CombinedSpeedViewController
@JvmOverloads
constructor(
    private val view: CombinedSpeedView,
    thresholds: SpeedThresholds = SpeedThresholds.DEFAULT,
) {
    /** How much the speed limit may be exceeded before warning and alerting about it. */
    var thresholds: SpeedThresholds = thresholds
        set(value) {
            if (field == value) {
                return
            }
            field = value
            updateMode(Mode.Normal)
        }

    /** The last known speed limit, null when it is unknown for the current road. */
    var speedLimit: Int? = null
        set(value) {
            if (field == value) {
                return
            }
            field = value
            updateSpeedLimit()
        }

    /** Whether the known speed limit is shown, see [showSpeedLimit] and [hideSpeedLimit]. */
    var isSpeedLimitVisible: Boolean = true
        set(value) {
            if (field == value) {
                return
            }
            field = value
            updateSpeedLimit()
        }

    /** Current speed, in the units of [units]. */
    var speed: Int
        get() = view.speed
        set(value) {
            view.speed = value
            updateMode()
        }

    /** Localized units of the speed and of the speed limit, e.g. "km/h". */
    var units: String
        get() = view.units
        set(value) {
            view.units = value
        }

    /** Shape of the speed-limit sign; the current speed is always square. */
    var shape: Shape
        get() = view.shape
        set(value) {
            view.shape = value
        }

    init {
        updateSpeedLimit()
    }

    /** Shows the speed limit again, it stays hidden while it is unknown. */
    fun showSpeedLimit() {
        isSpeedLimitVisible = true
    }

    /** Hides the speed limit, keeping the last known value for [showSpeedLimit]. */
    fun hideSpeedLimit() {
        isSpeedLimitVisible = false
    }

    private fun updateSpeedLimit() {
        view.setSpeedLimit(shownSpeedLimit())
        updateMode(Mode.Normal)
    }

    // The mode warns about exceeding the limit that is shown, so a hidden one is not compared.
    private fun updateMode(previousMode: Mode = view.mode) {
        view.mode = thresholds.modeFor(speed, shownSpeedLimit(), previousMode)
    }

    private fun shownSpeedLimit(): Int? = speedLimit.takeIf { isSpeedLimitVisible }
}
