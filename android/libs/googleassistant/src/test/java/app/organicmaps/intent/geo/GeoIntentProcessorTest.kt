package app.organicmaps.intent.geo

import android.content.Intent
import android.net.Uri
import app.organicmaps.intent.geo.handlers.AvoidActionHandler
import app.organicmaps.intent.geo.handlers.ControlActionHandler
import app.organicmaps.intent.geo.handlers.NavigationActionHandler
import app.organicmaps.intent.geo.handlers.ReportActionHandler
import app.organicmaps.intent.geo.handlers.SearchActionHandler
import app.organicmaps.intent.geo.handlers.VoiceActionHandler
import app.organicmaps.intent.geo.navigation.TravelMode
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner
import org.robolectric.annotation.Config

@RunWith(RobolectricTestRunner::class)
@Config(shadows = [ApiControllerMock::class])
class GeoIntentProcessorTest {
    @Before
    fun setUp() {
        ApiControllerMock.reset()
    }

    // Detailed per-parameter coverage of each handler lives in the `handlers` test package
    // (e.g. NavigationActionHandlerTest, SearchActionHandlerTest). These tests only verify that
    // the processor correctly dispatches to the right handler for each supported intent/action pair.

    @Test
    fun processIntent_dispatchesNavigateActionToNavigationHandler() {
        val navigationHandler = FakeNavigationActionHandler()
        val processor = createProcessor(navigationHandler = navigationHandler)

        ApiControllerMock.mockLatLon(
            TestUriSamples.NAVIGATION_WITH_COORDINATES,
            listOf(1.1, 2.2).toDoubleArray(),
        )

        assertTrue(
            processor.processIntent(
                Intent("android.intent.action.NAVIGATE").apply {
                    data = Uri.parse(TestUriSamples.NAVIGATION_WITH_COORDINATES)
                },
            ),
        )

        assertEquals(
            "navigate:1.1,2.2,Starbucks on Main Street,[],${TravelMode.Walk}",
            navigationHandler.events.single(),
        )
    }

    @Test
    fun processIntent_dispatchesCarAppNavigateActionToNavigationHandler() {
        val navigationHandler = FakeNavigationActionHandler()
        val processor = createProcessor(navigationHandler = navigationHandler)

        ApiControllerMock.mockLatLon(TestUriSamples.NAVIGATION_ADD_STOP, doubleArrayOf(Double.NaN, Double.NaN))

        assertTrue(
            processor.processIntent(
                Intent("androidx.car.app.action.NAVIGATE").apply {
                    data = Uri.parse(TestUriSamples.NAVIGATION_ADD_STOP)
                },
            ),
        )

        assertEquals("addStop:NaN,NaN,1600 Amphitheatre parkway", navigationHandler.events.single())
    }

    @Test
    fun processIntent_dispatchesViewActionToSearchHandler() {
        val searchHandler = FakeSearchActionHandler()
        val processor = createProcessor(searchHandler = searchHandler)

        ApiControllerMock.mockLatLon(TestUriSamples.SEARCH_NEARBY, doubleArrayOf(Double.NaN, Double.NaN))

        assertTrue(
            processor.processIntent(
                Intent(Intent.ACTION_VIEW).apply {
                    data = Uri.parse(TestUriSamples.SEARCH_NEARBY)
                },
            ),
        )

        assertEquals(listOf("search:NaN,NaN,restaurants nearby"), searchHandler.events)
    }

    @Test
    fun processIntent_returnsFalseForUnsupportedIntent() {
        val processor = createProcessor()

        assertFalse(processor.processIntent(Intent(Intent.ACTION_MAIN)))
    }

    @Suppress("LongParameterList")
    private fun createProcessor(
        avoidHandler: AvoidActionHandler = FakeAvoidActionHandler(),
        controlHandler: ControlActionHandler = FakeControlActionHandler(),
        navigationHandler: NavigationActionHandler = FakeNavigationActionHandler(),
        reportHandler: ReportActionHandler = FakeReportActionHandler(),
        searchHandler: SearchActionHandler = FakeSearchActionHandler(),
        voiceHandler: VoiceActionHandler = FakeVoiceActionHandler(),
    ) = GeoIntentProcessor(
        avoidHandler,
        controlHandler,
        navigationHandler,
        reportHandler,
        searchHandler,
        voiceHandler,
    )
}
