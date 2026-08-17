package app.organicmaps.routing

import android.content.Context
import android.graphics.Canvas
import android.graphics.Paint
import android.graphics.RectF
import androidx.core.content.ContextCompat
import androidx.recyclerview.widget.RecyclerView
import app.organicmaps.R

class RoundedSectionItemDecoration(context: Context, private val sectionAdapter: RecyclerView.Adapter<*>) :
    RecyclerView.ItemDecoration() {
    private val cornerRadius = context.resources.getDimension(R.dimen.corner_radius_large)
    private val paint =
        Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = ContextCompat.getColor(context, R.color.routing_bottom_manage_route_background)
        }
    private val rect = RectF()

    override fun onDraw(canvas: Canvas, parent: RecyclerView, state: RecyclerView.State) {
        var top = Int.MAX_VALUE
        var bottom = Int.MIN_VALUE
        var found = false
        for (i in 0 until parent.childCount) {
            val child = parent.getChildAt(i)
            val vh = parent.getChildViewHolder(child)
            if (vh != null && vh.bindingAdapter === sectionAdapter) {
                if (child.top < top) top = child.top
                if (child.bottom > bottom) bottom = child.bottom
                found = true
            }
        }
        if (!found) return
        val left = parent.paddingLeft.toFloat()
        val right = (parent.width - parent.paddingRight).toFloat()
        rect.set(left, top.toFloat(), right, bottom.toFloat())
        canvas.drawRoundRect(rect, cornerRadius, cornerRadius, paint)
    }
}
