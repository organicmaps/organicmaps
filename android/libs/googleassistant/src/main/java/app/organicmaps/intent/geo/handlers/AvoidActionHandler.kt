package app.organicmaps.intent.geo.handlers

interface AvoidActionHandler {
    /**
     * Updates the ferries preference for route planning.
     *
     *
     * This may trigger route recalculation.
     *
     * @param allow `true` to allow ferries, `false` to avoid ferries.
     */
    fun processFerriesAction(allow: Boolean)

    /**
     * Updates the highways preference for route planning.
     *
     *
     * This may trigger route recalculation.
     *
     * @param allow `true` to allow highways, `false` to avoid highways.
     */
    fun processHighwaysAction(allow: Boolean)

    /**
     * Updates the tolls preference for route planning.
     *
     *
     * This may trigger route recalculation.
     *
     * @param allow `true` to allow toll roads, `false` to avoid toll roads.
     */
    fun processTollsAction(allow: Boolean)
}
