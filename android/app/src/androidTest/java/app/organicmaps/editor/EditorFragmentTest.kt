package app.organicmaps.editor

import android.content.Intent
import android.os.SystemClock
import android.view.View
import android.widget.EditText
import androidx.recyclerview.widget.RecyclerView
import androidx.test.core.app.ActivityScenario
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import app.organicmaps.MwmApplication
import app.organicmaps.R
import app.organicmaps.sdk.editor.Editor
import app.organicmaps.sdk.editor.data.Language
import java.util.concurrent.Callable
import java.util.concurrent.CountDownLatch
import java.util.concurrent.FutureTask
import java.util.concurrent.TimeUnit
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Assume.assumeTrue
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith

/**
 * Edits a new cafe in [EditorActivity]. CI doesn't run app instrumentation tests, so run it by hand;
 * it is skipped unless a downloaded map covers [LAT], [LON].
 */
@RunWith(AndroidJUnit4::class)
class EditorFragmentTest {
    private val instrumentation = InstrumentationRegistry.getInstrumentation()
    private val app = MwmApplication.from(instrumentation.targetContext)
    private var noobAlertShown = false

    @Before
    fun setUp() {
        val ready = CountDownLatch(1)
        instrumentation.runOnMainSync {
            app.initOrganicMaps()
            app.mapsReadiness.runWhenReady(ready::countDown)
        }
        assertTrue("Core initialization did not complete", ready.await(30, TimeUnit.SECONDS))
        assumeTrue("Download the map that covers $LAT, $LON", onMain { Editor.nativeCreateMapObject(CAFE, LAT, LON) })

        // A successful Save then stops at the confirmation dialog instead of storing the edit.
        val prefs = MwmApplication.prefs(app)
        noobAlertShown = prefs.contains(EditorHostFragment.NOOB_ALERT_SHOWN)
        prefs.edit().remove(EditorHostFragment.NOOB_ALERT_SHOWN).apply()
    }

    @After
    fun tearDown() {
        if (noobAlertShown) {
            MwmApplication.prefs(app).edit().putBoolean(EditorHostFragment.NOOB_ALERT_SHOWN, true).commit()
        }
    }

    @Test
    fun invalidTranslation_staysVisibleAndBlocksSaveAfterRecreation() {
        val (position, mandatoryCount) = onMain {
            Editor.nativeSetNames(arrayOf(Editor.nativeMakeLocalizedName(TRANSLATION_LANG, TRANSLATION)))
            val names = Editor.nativeGetNamesDataSource()
            names.names.indexOfFirst { it.lang == TRANSLATION_LANG } to names.mandatoryNamesCount
        }
        assertTrue("No $TRANSLATION_LANG name", position >= 0)
        assumeTrue("$TRANSLATION_LANG is a mandatory name language", position >= mandatoryCount)

        launchEditor().use { scenario ->
            scenario.onActivity { it.findViewById<View>(R.id.show_additional_names).performClick() }
            waitUntil(scenario, "The translation row is not shown") { nameInput(it, position) != null }
            scenario.onActivity { requireNotNull(nameInput(it, position)).setText(BAD_NAME) }

            scenario.recreate()

            waitUntil(scenario, "The translation row is hidden after recreation") { nameInput(it, position) != null }
            scenario.onActivity { activity ->
                assertEquals(BAD_NAME, nameInput(activity, position)?.text?.toString())
                activity.findViewById<View>(R.id.save).performClick()
                assertEquals(TRANSLATION, nativeName(TRANSLATION_LANG))
            }
        }
    }

    @Test
    fun addedEmptyLanguage_survivesRecreation() {
        assumeTrue(
            "$TRANSLATION_LANG is already a name language",
            onMain { Editor.nativeGetNamesDataSource().names.none { it.lang == TRANSLATION_LANG } },
        )

        launchEditor().use { scenario ->
            var position = -1
            scenario.onActivity { activity ->
                val host = activity.supportFragmentManager.fragments.filterIsInstance<EditorHostFragment>().single()
                host.onLanguageSelected(Language(TRANSLATION_LANG, TRANSLATION_LANG))
                position = host.namesAsArray.size - 1
            }
            waitUntil(scenario, "The new language row is not shown") { nameInput(it, position) != null }

            scenario.recreate()

            waitUntil(scenario, "The new language row is hidden after recreation") { nameInput(it, position) != null }
            scenario.onActivity { assertEquals("", nameInput(it, position)?.text?.toString()) }
        }
    }

    private fun launchEditor(): ActivityScenario<EditorActivity> = ActivityScenario.launch(
        Intent(app, EditorActivity::class.java).putExtra(EditorActivity.EXTRA_NEW_OBJECT, true),
    )

    private fun nameInput(activity: EditorActivity, position: Int): EditText? =
        activity.findViewById<RecyclerView>(R.id.recycler).layoutManager
            ?.findViewByPosition(position)
            ?.findViewById(R.id.input)

    private fun nativeName(lang: String): String? =
        Editor.nativeGetNamesDataSource().names.firstOrNull { it.lang == lang }?.name

    private fun waitUntil(
        scenario: ActivityScenario<EditorActivity>,
        message: String,
        condition: (EditorActivity) -> Boolean,
    ) {
        val deadline = SystemClock.uptimeMillis() + TIMEOUT_MS
        var met = false
        while (!met) {
            assertTrue(message, SystemClock.uptimeMillis() < deadline)
            scenario.onActivity { met = condition(it) }
            if (!met) SystemClock.sleep(POLL_MS)
        }
    }

    private fun <T> onMain(block: () -> T): T {
        val task = FutureTask(Callable { block() })
        instrumentation.runOnMainSync(task)
        return task.get()
    }

    private companion object {
        const val CAFE = "amenity-cafe"
        const val LAT = 52.2318
        const val LON = 21.0067
        const val TRANSLATION_LANG = "de"
        const val TRANSLATION = "Kaffee"
        const val BAD_NAME = "Bad_Name"
        const val TIMEOUT_MS = 5_000L
        const val POLL_MS = 50L
    }
}
