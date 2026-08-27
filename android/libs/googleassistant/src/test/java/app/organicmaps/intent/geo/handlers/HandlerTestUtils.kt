package app.organicmaps.intent.geo.handlers

import app.organicmaps.intent.geo.FakeAvoidActionHandler
import app.organicmaps.intent.geo.FakeControlActionHandler
import app.organicmaps.intent.geo.FakeNavigationActionHandler
import app.organicmaps.intent.geo.FakeReportActionHandler
import app.organicmaps.intent.geo.FakeSearchActionHandler
import app.organicmaps.intent.geo.FakeVoiceActionHandler
import app.organicmaps.intent.geo.GeoIntentProcessor

/**
 * Shared factory for tests under the `handlers` package. Each test supplies the fake handler it
 * cares about; every other handler stays a no-op fake so the processor can be constructed without
 * boilerplate in every test file.
 */
@Suppress("LongParameterList")
internal fun createProcessor(
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
