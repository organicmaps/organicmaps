package app.organicmaps.routing

import android.content.Context
import android.content.res.Resources
import android.graphics.Canvas
import android.graphics.Paint
import android.graphics.RectF
import android.view.View
import androidx.recyclerview.widget.RecyclerView
import app.organicmaps.R
import org.junit.Assert.assertEquals
import org.junit.Test
import org.mockito.Mockito.clearInvocations
import org.mockito.Mockito.doReturn
import org.mockito.Mockito.mock
import org.mockito.Mockito.mockConstruction
import org.mockito.Mockito.mockingDetails

class RoundedSectionItemDecorationTest {
    @Test
    fun backgroundMatchesTheFullRouteAtEveryScrollOffset() {
        for (itemCount in listOf(4, 7, 11)) {
            val adapter = mock(ManageRouteAdapter::class.java)
            doReturn(itemCount).`when`(adapter).itemCount
            val (decoration, rect) = newDecoration(adapter)
            val positions = 0 until itemCount
            for (scroll in 0..itemCount * ROW_HEIGHT - VIEWPORT_HEIGHT) {
                val attached = positions.filter {
                    rowTop(it, scroll) < VIEWPORT_HEIGHT &&
                        rowTop(it, scroll) + ROW_HEIGHT > 0
                }
                assertEquals(
                    "$itemCount rows scrolled by $scroll",
                    draw(decoration, rect, adapter, positions.toList(), scroll),
                    draw(decoration, rect, adapter, attached, scroll),
                )
            }
        }
    }

    private fun newDecoration(adapter: ManageRouteAdapter): Pair<RoundedSectionItemDecoration, RectF> {
        val resources = mock(Resources::class.java)
        doReturn(CORNER_RADIUS).`when`(resources).getDimension(R.dimen.corner_radius_large)
        val context = mock(Context::class.java)
        doReturn(resources).`when`(context).resources
        mockConstruction(Paint::class.java).use {
            mockConstruction(RectF::class.java).use { rects ->
                return RoundedSectionItemDecoration(context, adapter) to rects.constructed().single()
            }
        }
    }

    private fun rowTop(position: Int, scroll: Int) = position * ROW_HEIGHT - scroll

    // Returns the background bounds, cut a corner radius off-screen: the background looks the same beyond that.
    private fun draw(
        decoration: RoundedSectionItemDecoration,
        rect: RectF,
        adapter: ManageRouteAdapter,
        positions: List<Int>,
        scroll: Int,
    ): String {
        val parent = mock(RecyclerView::class.java)
        doReturn(VIEWPORT_HEIGHT).`when`(parent).height
        doReturn(positions.size).`when`(parent).childCount
        positions.forEachIndexed { index, position ->
            val row = mock(View::class.java)
            doReturn(rowTop(position, scroll)).`when`(row).top
            doReturn(rowTop(position, scroll) + ROW_HEIGHT).`when`(row).bottom
            val holder = mock(RecyclerView.ViewHolder::class.java)
            doReturn(adapter).`when`(holder).bindingAdapter
            doReturn(position).`when`(holder).bindingAdapterPosition
            doReturn(row).`when`(parent).getChildAt(index)
            doReturn(holder).`when`(parent).getChildViewHolder(row)
        }
        clearInvocations(rect)
        decoration.onDraw(mock(Canvas::class.java), parent, RecyclerView.State())
        val bounds = mockingDetails(rect).invocations.single { it.method.name == "set" }.arguments
        val top = maxOf(bounds[1] as Float, -CORNER_RADIUS)
        val bottom = minOf(bounds[3] as Float, VIEWPORT_HEIGHT + CORNER_RADIUS)
        return "$top..$bottom"
    }

    companion object {
        private const val ROW_HEIGHT = 50
        private const val VIEWPORT_HEIGHT = 120
        private const val CORNER_RADIUS = 16f
    }
}
