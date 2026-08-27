package app.organicmaps.intent.geo.handlers

import android.content.Intent
import android.net.Uri
import app.organicmaps.intent.geo.FakeAvoidActionHandler
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner

/**
 * [AvoidActionHandler] is reached via `geo.action:` intents, one per allow/avoid combination of
 * ferries, highways and tolls.
 */
@RunWith(RobolectricTestRunner::class)
class AvoidActionHandlerTest {
    @Test
    fun allowFerries() = assertAction("allow_ferries", "ferries:true")

    @Test
    fun allowHighways() = assertAction("allow_highways", "highways:true")

    @Test
    fun allowTolls() = assertAction("allow_tolls", "tolls:true")

    @Test
    fun avoidFerries() = assertAction("avoid_ferries", "ferries:false")

    @Test
    fun avoidHighways() = assertAction("avoid_highways", "highways:false")

    @Test
    fun avoidTolls() = assertAction("avoid_tolls", "tolls:false")

    private fun assertAction(act: String, expectedEvent: String) {
        val handler = FakeAvoidActionHandler()
        val processor = createProcessor(avoidHandler = handler)

        assertTrue(
            processor.processIntent(
                Intent(Intent.ACTION_VIEW).apply { data = Uri.parse("geo.action:?act=$act") },
            ),
        )

        assertEquals(listOf(expectedEvent), handler.events)
    }
}
