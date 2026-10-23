package app.organicmaps.screenshots

import androidx.test.core.app.ActivityScenario
import androidx.test.espresso.Espresso.onView
import androidx.test.espresso.action.ViewActions.click
import androidx.test.espresso.matcher.ViewMatchers.isDisplayed
import androidx.test.espresso.matcher.ViewMatchers.withId
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import app.organicmaps.MwmActivity
import app.organicmaps.R
import org.junit.Test
import org.junit.runner.RunWith
import java.io.File
import java.io.FileOutputStream

/**
 * Espresso UI Test for Automated App Store Screenshot Generation.
 * Captures core app experiences across supported device form factors and locales.
 */
@RunWith(AndroidJUnit4::class)
class ScreenshotGeneratorTest {

    @Test
    fun captureAllStoreScreenshots() {
        val scenario = ActivityScenario.launch(MwmActivity::class.java)

        scenario.onActivity {
            // Wait for map surface rendering to stabilize
            Thread.sleep(2500)
            takeScreenshot("01_main_map")
        }

        // 2. Open Search / Categories
        try {
            onView(withId(R.id.search_button)).perform(click())
            Thread.sleep(1500)
            takeScreenshot("02_search_categories")
        } catch (_: Exception) {
            // Gracefully handle alternate layout configurations
        }

        // 3. Open Bookmarks & Tracks
        try {
            onView(withId(R.id.bookmarks_button)).perform(click())
            Thread.sleep(1500)
            takeScreenshot("03_bookmarks_tracks")
        } catch (_: Exception) {
            // Gracefully handle alternate layout configurations
        }

        scenario.close()
    }

    private fun takeScreenshot(name: String) {
        val uiAutomation = InstrumentationRegistry.getInstrumentation().uiAutomation
        val bitmap = uiAutomation.takeScreenshot() ?: return

        val context = InstrumentationRegistry.getInstrumentation().targetContext
        val outputDir = File(context.getExternalFilesDir(null), "screenshots")
        if (!outputDir.exists()) {
            outputDir.mkdirs()
        }

        val outputFile = File(outputDir, "${name}.png")
        FileOutputStream(outputFile).use { out ->
            bitmap.compress(android.graphics.Bitmap.CompressFormat.PNG, 100, out)
        }
    }
}
