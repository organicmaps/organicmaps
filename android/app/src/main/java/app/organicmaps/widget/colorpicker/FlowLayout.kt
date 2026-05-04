package app.organicmaps.widget.colorpicker

import android.content.Context
import android.util.AttributeSet
import android.view.ViewGroup
import androidx.core.view.children
import androidx.core.view.isGone

class FlowLayout @JvmOverloads constructor(context: Context, attrs: AttributeSet? = null, defStyleAttr: Int = 0) :
    ViewGroup(context, attrs, defStyleAttr) {

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        val maxWidth = MeasureSpec.getSize(widthMeasureSpec) - paddingLeft - paddingRight
        var x = 0
        var y = 0
        var rowHeight = 0

        for (child in children) {
            if (child.isGone) continue

            measureChildWithMargins(child, widthMeasureSpec, 0, heightMeasureSpec, y)
            val lp = child.layoutParams as MarginLayoutParams
            val startMargin = lp.marginStart
            val endMargin = lp.marginEnd
            val childWidth = child.measuredWidth + startMargin + endMargin
            val childHeight = child.measuredHeight + lp.topMargin + lp.bottomMargin

            if (x + childWidth > maxWidth && x > 0) {
                y += rowHeight
                x = 0
                rowHeight = 0
            }

            x += childWidth
            rowHeight = maxOf(rowHeight, childHeight)
        }

        val totalHeight = y + rowHeight + paddingTop + paddingBottom
        setMeasuredDimension(
            resolveSize(maxWidth + paddingLeft + paddingRight, widthMeasureSpec),
            resolveSize(totalHeight, heightMeasureSpec),
        )
    }

    override fun onLayout(changed: Boolean, l: Int, t: Int, r: Int, b: Int) {
        val maxWidth = r - l - paddingLeft - paddingRight
        var x = 0
        var y = 0
        var rowHeight = 0
        val isRtl = layoutDirection == LAYOUT_DIRECTION_RTL

        for (child in children) {
            if (child.isGone) continue

            val lp = child.layoutParams as MarginLayoutParams
            val childWidth = child.measuredWidth
            val childHeight = child.measuredHeight
            val startMargin = lp.marginStart
            val endMargin = lp.marginEnd
            val totalChildWidth = childWidth + startMargin + endMargin
            val totalChildHeight = childHeight + lp.topMargin + lp.bottomMargin

            if (x + totalChildWidth > maxWidth && x > 0) {
                y += rowHeight
                x = 0
                rowHeight = 0
            }

            val left = if (isRtl) {
                r - l - paddingRight - x - endMargin - childWidth
            } else {
                paddingLeft + x + startMargin
            }
            val top = paddingTop + y + lp.topMargin
            child.layout(left, top, left + childWidth, top + childHeight)

            x += totalChildWidth
            rowHeight = maxOf(rowHeight, totalChildHeight)
        }
    }

    override fun generateDefaultLayoutParams(): LayoutParams =
        MarginLayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT)

    override fun generateLayoutParams(attrs: AttributeSet): LayoutParams = MarginLayoutParams(context, attrs)

    override fun generateLayoutParams(p: LayoutParams): LayoutParams = MarginLayoutParams(p)

    override fun checkLayoutParams(p: LayoutParams): Boolean = p is MarginLayoutParams
}
