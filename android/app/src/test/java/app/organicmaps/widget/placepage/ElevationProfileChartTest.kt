package app.organicmaps.widget.placepage

import android.app.Application
import android.os.SystemClock
import android.view.MotionEvent
import android.view.View
import android.widget.FrameLayout
import com.github.mikephil.charting.data.Entry
import com.github.mikephil.charting.data.LineData
import com.github.mikephil.charting.data.LineDataSet
import com.github.mikephil.charting.highlight.Highlight
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotEquals
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner
import org.robolectric.RuntimeEnvironment
import org.robolectric.annotation.Config

@RunWith(RobolectricTestRunner::class)
@Config(sdk = [34], application = Application::class)
class ElevationProfileChartTest {
    private val finished = mutableListOf<Float>()
    private lateinit var chart: ElevationProfileChart

    @Before
    fun showChart() {
        val context = RuntimeEnvironment.getApplication()
        chart = ElevationProfileChart(context)
        chart.data = LineData(LineDataSet((0..10).map { Entry(it * 10f, it.toFloat()) }, "profile"))
        // The chart asks its parent to keep the gesture.
        FrameLayout(context).addView(chart)
        chart.measure(exactly(WIDTH), exactly(HEIGHT))
        chart.layout(0, 0, WIDTH, HEIGHT)
        chart.highlightValue(Highlight(START_X, 0f, 0), false)
        chart.setOnSelectionFinishedListener { finished.add(it) }
    }

    @Test
    fun cancelledDragRestoresItsStartWithoutReporting() {
        drag(MotionEvent.ACTION_CANCEL)
        assertEquals(START_X, chart.highlightX, 0f)
        assertEquals(emptyList<Float>(), finished)
        assertFalse(chart.isSelectionInProgress)
    }

    @Test
    fun releasedDragReportsWhereTheFingerLiftsOnce() {
        // The last move stays inside the profile, the finger lifts past its finish.
        val moved = drag(MotionEvent.ACTION_UP, WIDTH.toFloat())
        assertNotEquals(FINISH_X, moved, 0f)
        assertEquals(FINISH_X, chart.highlightX, 0f)
        assertEquals(listOf(FINISH_X), finished)
        assertFalse(chart.isSelectionInProgress)
    }

    /** @return the highlight X after the move. */
    private fun drag(lastAction: Int, releaseX: Float = WIDTH * 0.75f): Float {
        val y = HEIGHT / 2f
        touch(MotionEvent.ACTION_DOWN, WIDTH * 0.25f, y)
        touch(MotionEvent.ACTION_MOVE, WIDTH * 0.75f, y)
        assertTrue(chart.isSelectionInProgress)
        val moved = chart.highlightX
        assertNotEquals(START_X, moved, 0f)
        touch(lastAction, releaseX, y)
        return moved
    }

    private fun touch(action: Int, x: Float, y: Float) {
        val time = SystemClock.uptimeMillis()
        val event = MotionEvent.obtain(time, time, action, x, y, 0)
        chart.onTouchEvent(event)
        event.recycle()
    }

    private fun exactly(size: Int) = View.MeasureSpec.makeMeasureSpec(size, View.MeasureSpec.EXACTLY)

    private companion object {
        const val WIDTH = 1000
        const val HEIGHT = 300
        const val START_X = 25f
        const val FINISH_X = 100f
    }
}
