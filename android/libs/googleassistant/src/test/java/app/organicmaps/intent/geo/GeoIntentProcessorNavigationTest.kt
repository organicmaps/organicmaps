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
@Config(shadows = [ShadowApiController::class])
class GeoIntentProcessorNavigationTest {
    @Before
    fun setUp() {
        TestApiControllerHelper.reset()
    }

    @Test
    fun processIntent_routesNavigationAndDirectionsAndSearch() {
        val navigationHandler = FakeNavigationActionHandler()
        val searchHandler = FakeSearchActionHandler()
        val processor = createProcessor(navigationHandler = navigationHandler, searchHandler = searchHandler)

        TestApiControllerHelper.stubLatLon(
            TestUriSamples.NAVIGATION_WITH_COORDINATES,
            listOf(1.1, 2.2).toDoubleArray(),
        )
        TestApiControllerHelper.stubLatLon(TestUriSamples.NAVIGATION_ADD_STOP, doubleArrayOf(0.0, 0.0))
        TestApiControllerHelper.stubLatLon(TestUriSamples.SEARCH_NEARBY, doubleArrayOf(0.0, 0.0))

        assertTrue(
            processor.processIntent(
                Intent("android.intent.action.NAVIGATE").apply {
                    data = Uri.parse(TestUriSamples.NAVIGATION_WITH_COORDINATES)
                },
            ),
        )

        assertTrue(
            processor.processIntent(
                Intent("androidx.car.app.action.NAVIGATE").apply {
                    data = Uri.parse(TestUriSamples.NAVIGATION_ADD_STOP)
                },
            ),
        )
        assertTrue(
            processor.processIntent(
                Intent(Intent.ACTION_VIEW).apply {
                    data = Uri.parse(TestUriSamples.SEARCH_NEARBY)
                },
            ),
        )

        assertEquals(2, navigationHandler.events.size)
        assertEquals("navigate:1.1,2.2,Starbucks on Main Street,[],${TravelMode.Walk}", navigationHandler.events[0])
        assertEquals("addStop:0.0,0.0,1600 Amphitheatre parkway", navigationHandler.events[1])
        assertEquals(listOf("search:0.0,0.0,restaurants nearby"), searchHandler.events)
    }

    @Test
    fun processIntent_returnsFalseForUnsupportedIntent() {
        val processor = createProcessor()

        assertFalse(processor.processIntent(Intent(Intent.ACTION_MAIN)))
    }

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
