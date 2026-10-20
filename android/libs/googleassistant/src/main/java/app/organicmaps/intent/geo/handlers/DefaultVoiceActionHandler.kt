package app.organicmaps.intent.geo.handlers

import app.organicmaps.sdk.util.log.Logger

class DefaultVoiceActionHandler : VoiceActionHandler {
    override fun processVoiceGuidanceAction(enable: Boolean) {
        Logger.d(TAG, "Not implemented: process voice guidance action enable=$enable")
    }

    override fun speakTrafficReport() {
        Logger.d(TAG, "Not implemented: traffic report")
    }

    override fun speakCurrentRoad() {
        Logger.d(TAG, "Not implemented: query current road")
    }

    override fun speakDestination() {
        Logger.d(TAG, "Not implemented: query destination")
    }

    override fun speakEta() {
        Logger.d(TAG, "Not implemented: show ETA to destination")
    }

    override fun speakDistanceToDestination() {
        Logger.d(TAG, "Not implemented: show distance to destination")
    }

    override fun speakTimeToDestination() {
        Logger.d(TAG, "Not implemented: show time to destination")
    }

    override fun speakNextTurn() {
        Logger.d(TAG, "Not implemented: query next turn")
    }

    override fun speakDistanceToNextTurn() {
        Logger.d(TAG, "Not implemented: show distance to next turn")
    }

    override fun speakTimeToNextTurn() {
        Logger.d(TAG, "Not implemented: show time to next turn")
    }

    companion object {
        private val TAG: String = DefaultVoiceActionHandler::class.java.simpleName
    }
}
