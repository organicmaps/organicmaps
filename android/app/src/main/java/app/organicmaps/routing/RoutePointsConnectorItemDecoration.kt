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
        var firstPosition = Int.MAX_VALUE
        var lastPosition = RecyclerView.NO_POSITION
        for (i in 0 until parent.childCount) {
            val child = parent.getChildAt(i)
            val holder = parent.getChildViewHolder(child)
            if (holder?.bindingAdapter !== sectionAdapter) continue
            rows.add(child)
            // A row that is animating out has no position.
            val position = holder.bindingAdapterPosition
            if (position != RecyclerView.NO_POSITION) {
                firstPosition = minOf(firstPosition, position)
                lastPosition = maxOf(lastPosition, position)
            }
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
            val dotted = parent.getChildViewHolder(lowerRow) is ManageRouteAdapter.AddStopViewHolder
            drawConnector(canvas, x, top, bottom, dotted)
        }
        if (lastPosition == RecyclerView.NO_POSITION) return
        // Off-screen rows are recycled. All rows share the height and icon placement, so the connectors to them
        // are continued to where their icons would be.
        if (firstPosition > 0) {
            val row = rows.first()
            val icon = row.findViewById<View>(R.id.type_icon)
            val x = row.left + icon.left + icon.width / 2f
            val top = row.y - row.height + icon.bottom + iconGapPx
            val bottom = row.y + icon.top - iconGapPx
            val dotted = parent.getChildViewHolder(row) is ManageRouteAdapter.AddStopViewHolder
            drawConnector(canvas, x, top, bottom, dotted)
        }
        if (lastPosition < sectionAdapter.itemCount - 1) {
            val row = rows.last()
            val icon = row.findViewById<View>(R.id.type_icon)
            val x = row.left + icon.left + icon.width / 2f
            val top = row.y + icon.bottom + iconGapPx
            val bottom = row.y + row.height + icon.top - iconGapPx
            val dotted = sectionAdapter.getItemViewType(lastPosition + 1) == ManageRouteAdapter.TYPE_ADD_BUTTON
            drawConnector(canvas, x, top, bottom, dotted)
        }
    }

    private fun drawConnector(canvas: Canvas, x: Float, top: Float, bottom: Float, dotted: Boolean) {
        if (bottom <= top) return
        if (dotted) {
            drawThreeDots(canvas, x, top, bottom)
        } else {
            canvas.drawLine(x, top, x, bottom, paint)
        }
    }

    private fun drawThreeDots(canvas: Canvas, x: Float, top: Float, bottom: Float) {
        val step = (bottom - top - 2 * dotRadiusPx) / 2
        for (i in 0..2) {
            canvas.drawCircle(x, top + dotRadiusPx + i * step, dotRadiusPx, paint)
        }
    }
}
