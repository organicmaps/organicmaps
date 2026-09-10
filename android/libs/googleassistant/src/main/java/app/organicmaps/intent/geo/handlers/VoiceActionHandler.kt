package app.organicmaps.intent.geo.handlers

interface VoiceActionHandler {
    /**
     * Enables or disables navigation voice guidance.
     *
     * @param enable `true` to enable voice guidance, `false` to mute it.
     */
    fun processVoiceGuidanceAction(enable: Boolean)

    /**
     * Speaks the current traffic report.
     */
    fun speakTrafficReport()

    /**
     * Speaks the current road name.
     */
    fun speakCurrentRoad()

    /**
     * Speaks the current destination name during navigation.
     */
    fun speakDestination()

    /**
     * Speaks the estimated time of arrival during navigation.
     */
    fun speakEta()

    /**
     * Speaks remaining distance to destination during navigation.
     */
    fun speakDistanceToDestination()

    /**
     * Speaks remaining travel time to destination during navigation.
     */
    fun speakTimeToDestination()

    /**
     * Speaks the next maneuver during navigation.
     */
    fun speakNextTurn()

    /**
     * Speaks distance to the next maneuver during navigation.
     */
    fun speakDistanceToNextTurn()

    /**
     * Speaks time remaining to the next maneuver during navigation.
     */
    fun speakTimeToNextTurn()
}
