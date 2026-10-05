package app.organicmaps.routing

import android.content.Context
import android.graphics.Canvas
import android.graphics.Paint
import android.view.View
import androidx.recyclerview.widget.RecyclerView
import app.organicmaps.R
import app.organicmaps.util.ThemeUtils

class RoutePointsConnectorItemDecoration(context: Context, private val sectionAdapter: RecyclerView.Adapter<*>) :
    RecyclerView.ItemDecoration() {
    private val density = context.resources.displayMetrics.density
    private val iconGapPx = 3 * density
    private val dotRadiusPx = 1.75f * density
    private val paint =
        Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = ThemeUtils.getColor(context, android.R.attr.textColorSecondary)
            strokeWidth = 2 * density
        }
    private val rows = ArrayList<View>()

    override fun onDraw(canvas: Canvas, parent: RecyclerView, state: RecyclerView.State) {
        rows.clear()
        for (i in 0 until parent.childCount) {
            val child = parent.getChildAt(i)
            if (parent.getChildViewHolder(child)?.bindingAdapter === sectionAdapter) rows.add(child)
        }
        rows.sortBy { it.y }
        for (i in 1 until rows.size) {
            val upperRow = rows[i - 1]
            val lowerRow = rows[i]
            val upperIcon = upperRow.findViewById<View>(R.id.type_icon)
            val lowerIcon = lowerRow.findViewById<View>(R.id.type_icon)
            val x = upperRow.left + upperIcon.left + upperIcon.width / 2f
            val top = upperRow.y + upperIcon.bottom + iconGapPx
            val bottom = lowerRow.y + lowerIcon.top - iconGapPx
            if (bottom <= top) continue
            if (parent.getChildViewHolder(lowerRow) is ManageRouteAdapter.AddStopViewHolder) {
                drawThreeDots(canvas, x, top, bottom)
            } else {
                canvas.drawLine(x, top, x, bottom, paint)
            }
        }
    }

    private fun drawThreeDots(canvas: Canvas, x: Float, top: Float, bottom: Float) {
        val step = (bottom - top - 2 * dotRadiusPx) / 2
        for (i in 0..2) {
            canvas.drawCircle(x, top + dotRadiusPx + i * step, dotRadiusPx, paint)
        }
    }
}
