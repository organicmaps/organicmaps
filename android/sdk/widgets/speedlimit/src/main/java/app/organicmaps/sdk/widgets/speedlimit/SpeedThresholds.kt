package app.organicmaps.sdk.widgets.speedlimit

/**
 * How much the speed limit may be exceeded, in the units of the shown speed, before the widget
 * switches to [Mode.Warning] and then to [Mode.Alert].
 *
 * Entry requires an excess strictly greater than the threshold. An active mode clears when the
 * excess is at or below its threshold minus hysteresis.
 *
 * @param warning excess above which [Mode.Warning] starts.
 * @param alert excess above which [Mode.Alert] starts, must not be less than [warning].
 * @param warningHysteresis amount below [warning] at which an active warning clears.
 * @param alertHysteresis amount below [alert] at which an active alert clears.
 */
data class SpeedThresholds @JvmOverloads constructor(
    val warning: Int,
    val alert: Int,
    val warningHysteresis: Int = 0,
    val alertHysteresis: Int = 0,
) {
    init {
        require(warning >= 0) { "Warning threshold must not be negative, but was $warning" }
        require(alert >= warning) { "Alert threshold $alert must not be less than warning $warning" }
        require(warningHysteresis in 0..warning) { "Warning hysteresis must be between 0 and $warning" }
        require(alertHysteresis in 0..alert) { "Alert hysteresis must be between 0 and $alert" }
    }

    /**
     * Selects the mode for the given speed, [Mode.Normal] when the speed limit is unknown or is
     * not exceeded by more than the thresholds. Pass the last returned mode as [previousMode]
     * to retain warning and alert until their respective reset thresholds are reached.
     */
    @JvmOverloads
    fun modeFor(speed: Int, speedLimit: Int?, previousMode: Mode = Mode.Normal): Mode {
        if (speedLimit == null || speedLimit <= 0) {
            return Mode.Normal
        }
        val warningThreshold = if (previousMode != Mode.Normal) warning - warningHysteresis else warning
        val alertThreshold = if (previousMode == Mode.Alert) alert - alertHysteresis else alert
        val excess = speed.toLong() - speedLimit
        return when {
            excess > alertThreshold -> Mode.Alert
            excess > warningThreshold -> Mode.Warning
            else -> Mode.Normal
        }
    }

    companion object {
        /**
         * Warns at more than 5 and alerts at more than 10 units above the limit.
         * For a limit of 50, entry speeds are 56 and 61; alert resets at 57, warning at 53.
         */
        @JvmField
        val DEFAULT = SpeedThresholds(warning = 5, alert = 10, warningHysteresis = 2, alertHysteresis = 3)
    }
}
