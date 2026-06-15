package app.organicmaps.routing

import android.content.Context
import android.graphics.Canvas
import android.graphics.drawable.ColorDrawable
import android.graphics.drawable.Drawable
import androidx.recyclerview.widget.RecyclerView
import app.organicmaps.R
import app.organicmaps.util.ThemeUtils

class SectionDividerItemDecoration(context: Context, private val sectionAdapter: RecyclerView.Adapter<*>) :
    RecyclerView.ItemDecoration() {
    private val divider: Drawable =
        ColorDrawable(ThemeUtils.getColor(context, androidx.appcompat.R.attr.dividerHorizontal))
    private val heightPx = context.resources.getDimensionPixelSize(R.dimen.divider_height)
    private val insetStartPx = context.resources.getDimensionPixelSize(R.dimen.margin_double_and_half)

    override fun onDraw(canvas: Canvas, parent: RecyclerView, state: RecyclerView.State) {
        val sectionItemCount = sectionAdapter.itemCount
        val left = parent.paddingLeft + insetStartPx
        val right = parent.width - parent.paddingRight
        for (i in 0 until parent.childCount) {
            val child = parent.getChildAt(i)
            val vh = parent.getChildViewHolder(child)
            val pos = vh?.bindingAdapterPosition ?: -1
            val inSection = vh?.bindingAdapter === sectionAdapter
            if (inSection && pos in 0 until sectionItemCount - 1) {
                val top = child.bottom
                divider.setBounds(left, top, right, top + heightPx)
                divider.draw(canvas)
            }
        }
    }
}
