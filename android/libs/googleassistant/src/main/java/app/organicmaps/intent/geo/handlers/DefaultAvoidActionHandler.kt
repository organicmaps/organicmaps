package app.organicmaps.intent.geo.handlers

import app.organicmaps.sdk.util.log.Logger

class DefaultAvoidActionHandler : AvoidActionHandler {
    override fun processFerriesAction(allow: Boolean) {
        Logger.d(TAG, "Not implemented: process ferries action allow=$allow")
    }

    override fun processHighwaysAction(allow: Boolean) {
        Logger.d(TAG, "Not implemented: process highways action allow=$allow")
    }

    override fun processTollsAction(allow: Boolean) {
        Logger.d(TAG, "Not implemented: process tolls action allow=$allow")
    }

    companion object {
        private val TAG: String = DefaultAvoidActionHandler::class.java.simpleName
    }
}
