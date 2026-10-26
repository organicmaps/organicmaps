package app.organicmaps.routing

import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.appcompat.view.ContextThemeWrapper
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import app.organicmaps.R
import app.organicmaps.sdk.routing.TransitStepInfo
import app.organicmaps.sdk.routing.TransitStepType
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test
import org.junit.runner.RunWith

/**
 * Binds [TransitDetailsAdapter] rows in a laid-out RecyclerView and checks what the rider sees: transfer
 * headings, stop names after a holder is reused for another row, and the stops disclosure after rapid taps.
 */
@RunWith(AndroidJUnit4::class)
class TransitDetailsAdapterBindTest {
    private val instrumentation = InstrumentationRegistry.getInstrumentation()
    private val context = ContextThemeWrapper(instrumentation.targetContext, R.style.MwmTheme)

    @Test
    fun sameStopTransfer_isDrawnOnceAsTheNextRideHeader() = onMain {
        val recycler = layOut(listOf(ride("A", "B"), ride("B", "C")))

        assertFalse(row(recycler, 0).isShown(R.id.exit_at))
        assertTrue(row(recycler, 1).isShown(R.id.transfer_header_row))
        assertEquals("B", row(recycler, 1).text(R.id.transfer_header))
        assertFalse(row(recycler, 1).isShown(R.id.board_at))
    }

    @Test
    fun transferBetweenDifferentlyNamedStops_keepsBothNames() = onMain {
        val recycler = layOut(listOf(ride("A", "Incoming"), ride("Connecting", "C")))

        assertEquals("Incoming", row(recycler, 0).text(R.id.exit_at))
        assertFalse(row(recycler, 1).isShown(R.id.transfer_header_row))
        assertEquals("Connecting", row(recycler, 1).text(R.id.board_at))
    }

    @Test
    fun unnamedStops_showNoEmptyHeadingsOrTransfer() = onMain {
        val recycler = layOut(listOf(ride("A", ""), ride("", "C")))

        assertFalse(row(recycler, 0).isShown(R.id.exit_at))
        assertFalse(row(recycler, 1).isShown(R.id.transfer_header_row))
        assertFalse(row(recycler, 1).isShown(R.id.board_at))
        assertEquals("C", row(recycler, 1).text(R.id.exit_at))
    }

    @Test
    fun reusedHolder_dropsThePreviousRowsTransfer() = onMain {
        val steps = listOf(ride("A", "B"), ride("B", "C"), ride("D", "E"))
        val recycler = layOut(steps)
        val adapter = recycler.adapter as TransitDetailsAdapter
        val holder = recycler.findViewHolderForAdapterPosition(1)!!

        // Rebind the transfer row's holder as the following, unjoined ride.
        adapter.onBindViewHolder(holder, 2)

        assertFalse(holder.itemView.isShown(R.id.transfer_header_row))
        assertEquals("D", holder.itemView.text(R.id.board_at))
        assertEquals("E", holder.itemView.text(R.id.exit_at))
    }

    @Test
    fun rapidCollapseAndExpand_endsExpanded() {
        lateinit var recycler: RecyclerView
        onMain {
            recycler = layOut(listOf(ride("A", "D", arrayOf("B", "C"))))
            val summary = row(recycler, 0).findViewById<View>(R.id.ride_summary_row)
            summary.performClick() // expand
            summary.performClick() // collapse, interrupting the expand
            summary.performClick() // expand again, interrupting the collapse
        }
        // Let the 150 ms height tween finish on the main thread.
        Thread.sleep(600)
        onMain {
            val stops = row(recycler, 0).findViewById<TextView>(R.id.intermediate_stops)
            assertEquals(View.VISIBLE, stops.visibility)
            assertEquals(ViewGroup.LayoutParams.WRAP_CONTENT, stops.layoutParams.height)
            assertEquals("B\nC", stops.text.toString())
        }
    }

    private fun layOut(steps: List<TransitStepInfo>): RecyclerView {
        val recycler = RecyclerView(context)
        recycler.layoutManager = LinearLayoutManager(context)
        recycler.itemAnimator = null
        // The native duration formatter needs the initialized core, which this test does not start.
        recycler.adapter = TransitDetailsAdapter(steps, null, emptyMap()) { "$it s" }
        recycler.measure(
            View.MeasureSpec.makeMeasureSpec(1080, View.MeasureSpec.EXACTLY),
            View.MeasureSpec.makeMeasureSpec(4000, View.MeasureSpec.EXACTLY),
        )
        recycler.layout(0, 0, 1080, 4000)
        return recycler
    }

    private fun row(recycler: RecyclerView, position: Int): View =
        recycler.findViewHolderForAdapterPosition(position)!!.itemView

    private fun View.isShown(id: Int) = findViewById<View>(id).visibility == View.VISIBLE

    private fun View.text(id: Int) = findViewById<TextView>(id).text.toString()

    private fun onMain(block: () -> Unit) {
        var error: Throwable? = null
        instrumentation.runOnMainSync {
            try {
                block()
            } catch (t: Throwable) {
                error = t
            }
        }
        error?.let { throw it }
    }

    // TransitStepInfo's constructor is private (JNI-only); build test instances reflectively rather
    // than widening the production API.
    private fun ride(startStop: String, endStop: String, intermediates: Array<String>? = null): TransitStepInfo {
        val ctor = TransitStepInfo::class.java.getDeclaredConstructor(
            Int::class.javaPrimitiveType, String::class.java, String::class.java, Int::class.javaPrimitiveType,
            String::class.java, Int::class.javaPrimitiveType, Int::class.javaPrimitiveType, String::class.java,
            String::class.java, Int::class.javaPrimitiveType, Array<String>::class.java, String::class.java,
        )
        ctor.isAccessible = true
        val stopCount = 1 + (intermediates?.size ?: 0)
        return ctor.newInstance(
            TransitStepType.BUS.ordinal, null, null, 600, "7", 0xFFAA0000.toInt(), 0, startStop, endStop,
            stopCount, intermediates, "7",
        )
    }
}
