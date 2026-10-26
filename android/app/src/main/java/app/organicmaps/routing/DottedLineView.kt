package app.organicmaps.routing

import android.content.Context
import android.graphics.Canvas
import android.graphics.Paint
import android.util.AttributeSet
import android.util.TypedValue
import android.view.View
import app.organicmaps.R
import app.organicmaps.util.ThemeUtils

/**
 * Four dots, two above and two below the view's center, used as the walking-segment connector on the
 * transit details timeline (the dotted counterpart to the ride legs' solid colored bar). The dots keep a
 * fixed span around the centered walk icon whatever the row height.
 */
class DottedLineView(context: Context, attrs: AttributeSet?) : View(context, attrs) {
    private val paint = Paint(Paint.ANTI_ALIAS_FLAG).apply { color = ThemeUtils.getColor(context, R.attr.secondary) }
    private val radius = dp(2.5f)
    private val step = radius * 2 + dp(4f)

    // Distance from the centre to the first dot: clears the walk icon that sits in the middle.
    private val gap = dp(18f)

    private fun dp(value: Float) =
        TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, value, resources.displayMetrics)

    override fun onDraw(canvas: Canvas) {
        val cx = width / 2f
        val center = height / 2f
        for (i in 0 until DOTS_PER_SIDE) {
            val offset = gap + i * step
            canvas.drawCircle(cx, center - offset, radius, paint)
            canvas.drawCircle(cx, center + offset, radius, paint)
        }
    }

    private companion object {
        // Two dots above the central walk-icon gap and two below it.
        const val DOTS_PER_SIDE = 2
    }
}
