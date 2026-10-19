package app.organicmaps.routing

import android.content.Context
import android.content.res.Resources
import android.graphics.Canvas
import android.graphics.Paint
import android.util.DisplayMetrics
import android.view.View
import androidx.recyclerview.widget.RecyclerView
import app.organicmaps.R
import app.organicmaps.util.ThemeUtils
import org.junit.Assert.assertEquals
import org.junit.Test
import org.mockito.Mockito.doReturn
import org.mockito.Mockito.mock
import org.mockito.Mockito.mockConstruction
import org.mockito.Mockito.mockStatic
import org.mockito.Mockito.mockingDetails

class RoutePointsConnectorItemDecorationTest {
    @Test
    fun connectorsMatchTheFullRouteAtEveryScrollOffset() {
        // A list lower than a row also shows the case when the only row on screen is "Add destination".
        for (viewportHeight in listOf(40, 120)) {
            for (pointCount in listOf(3, 6, 10)) {
                val itemCount = pointCount + 1
                val adapter = mock(ManageRouteAdapter::class.java)
                doReturn(itemCount).`when`(adapter).itemCount
                doReturn(ManageRouteAdapter.TYPE_ADD_BUTTON).`when`(adapter).getItemViewType(itemCount - 1)
                val decoration = newDecoration(adapter)
                val positions = 0 until itemCount
                for (scroll in 0..itemCount * ROW_HEIGHT - viewportHeight) {
                    val attached = positions.filter {
                        rowTop(it, scroll) < viewportHeight &&
                            rowTop(it, scroll) + ROW_HEIGHT > 0
                    }
                    assertEquals(
                        "$pointCount points in $viewportHeight px scrolled by $scroll",
                        draw(decoration, adapter, positions.toList(), scroll, viewportHeight),
                        draw(decoration, adapter, attached, scroll, viewportHeight),
                    )
                }
            }
        }
    }

    private fun newDecoration(adapter: ManageRouteAdapter): RoutePointsConnectorItemDecoration {
        val metrics = mock(DisplayMetrics::class.java)
        metrics.density = 1f
        val resources = mock(Resources::class.java)
        doReturn(metrics).`when`(resources).displayMetrics
        val context = mock(Context::class.java)
        doReturn(resources).`when`(context).resources
        mockStatic(ThemeUtils::class.java).use {
            mockConstruction(Paint::class.java).use {
                return RoutePointsConnectorItemDecoration(context, adapter)
            }
        }
    }

    private fun rowTop(position: Int, scroll: Int) = position * ROW_HEIGHT - scroll

    // Returns the connector parts drawn inside the viewport.
    private fun draw(
        decoration: RoutePointsConnectorItemDecoration,
        adapter: ManageRouteAdapter,
        positions: List<Int>,
        scroll: Int,
        viewportHeight: Int,
    ): List<String> {
        val parent = mock(RecyclerView::class.java)
        doReturn(positions.size).`when`(parent).childCount
        positions.forEachIndexed { index, position ->
            val icon = mock(View::class.java)
            doReturn(ICON_LEFT).`when`(icon).left
            doReturn(ICON_SIZE).`when`(icon).width
            doReturn(ICON_TOP).`when`(icon).top
            doReturn(ICON_TOP + ICON_SIZE).`when`(icon).bottom
            val row = mock(View::class.java)
            doReturn(rowTop(position, scroll).toFloat()).`when`(row).y
            doReturn(ROW_HEIGHT).`when`(row).height
            doReturn(icon).`when`(row).findViewById<View>(R.id.type_icon)
            val holder =
                if (position == adapter.itemCount - 1) {
                    mock(ManageRouteAdapter.AddStopViewHolder::class.java)
                } else {
                    mock(RecyclerView.ViewHolder::class.java)
                }
            doReturn(adapter).`when`(holder).bindingAdapter
            doReturn(position).`when`(holder).bindingAdapterPosition
            doReturn(row).`when`(parent).getChildAt(index)
            doReturn(holder).`when`(parent).getChildViewHolder(row)
        }
        val canvas = mock(Canvas::class.java)
        decoration.onDraw(canvas, parent, RecyclerView.State())
        return mockingDetails(canvas).invocations.mapNotNull {
            val args = it.arguments
            when (it.method.name) {
                "drawLine" -> {
                    val top = maxOf(args[1] as Float, 0f)
                    val bottom = minOf(args[3] as Float, viewportHeight.toFloat())
                    if (top < bottom) "line ${args[0]}: $top..$bottom" else null
                }

                "drawCircle" -> {
                    val y = args[1] as Float
                    val radius = args[2] as Float
                    if (y + radius > 0 && y - radius < viewportHeight) "dot ${args[0]}: $y" else null
                }

                else -> null
            }
        }.sorted()
    }

    companion object {
        private const val ROW_HEIGHT = 50
        private const val ICON_LEFT = 8
        private const val ICON_TOP = 13
        private const val ICON_SIZE = 24
    }
}
