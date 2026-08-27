package app.organicmaps.intent.geo.handlers

import android.content.Intent
import android.net.Uri
import app.organicmaps.intent.geo.FakeVoiceActionHandler
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner

/** [VoiceActionHandler] is reached via `geo.action:` intents, one per supported `act` value. */
@RunWith(RobolectricTestRunner::class)
class VoiceActionHandlerTest {
    @Test
    fun distanceToDestination() = assertAction("distance_to_destination", "distanceToDestination")

    @Test
    fun distanceToNextTurn() = assertAction("distance_to_next_turn", "distanceToNextTurn")

    @Test
    fun eta() = assertAction("eta", "eta")

    @Test
    fun mute() = assertAction("mute", "voice:false")

    @Test
    fun unmute() = assertAction("unmute", "voice:true")

    @Test
    fun queryCurrentRoad() = assertAction("query_current_road", "currentRoad")

    @Test
    fun queryDestination() = assertAction("query_destination", "destination")

    @Test
    fun queryNextTurn() = assertAction("query_next_turn", "nextTurn")

    @Test
    fun timeToDestination() = assertAction("time_to_destination", "timeToDestination")

    @Test
    fun timeToNextTurn() = assertAction("time_to_next_turn", "timeToNextTurn")

    @Test
    fun trafficReport() = assertAction("traffic_report", "trafficReport")

    private fun assertAction(act: String, expectedEvent: String) {
        val handler = FakeVoiceActionHandler()
        val processor = createProcessor(voiceHandler = handler)

        assertTrue(
            processor.processIntent(
                Intent(Intent.ACTION_VIEW).apply { data = Uri.parse("geo.action:?act=$act") },
            ),
        )

        assertEquals(listOf(expectedEvent), handler.events)
    }
}
