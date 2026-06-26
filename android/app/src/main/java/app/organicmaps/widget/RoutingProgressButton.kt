package app.organicmaps.widget

import android.content.Context
import android.graphics.Canvas
import android.graphics.Paint
import android.graphics.Path
import android.graphics.RectF
import android.util.AttributeSet
import androidx.appcompat.widget.AppCompatButton
import androidx.core.content.ContextCompat
import androidx.core.graphics.withClip
import app.organicmaps.R

class RoutingProgressButton @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = android.R.attr.buttonStyle,
) : AppCompatButton(context, attrs, defStyleAttr) {

    private val fillPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        style = Paint.Style.FILL
        color = ContextCompat.getColor(context, R.color.button_accent_normal)
    }
    private val clipPath = Path()
    private val clipRect = RectF()
    private val cornerRadius = resources.getDimension(R.dimen.corner_radius_small)

    var buildProgress: Int = 0
        set(value) {
            val clamped = value.coerceIn(0, 100)
            if (clamped == field) return
            field = clamped
            invalidate()
        }

    override fun onSizeChanged(w: Int, h: Int, oldw: Int, oldh: Int) {
        super.onSizeChanged(w, h, oldw, oldh)
        clipRect.set(0f, 0f, w.toFloat(), h.toFloat())
        clipPath.rewind()
        clipPath.addRoundRect(clipRect, cornerRadius, cornerRadius, Path.Direction.CW)
    }

    override fun onDraw(canvas: Canvas) {
        if (buildProgress > 0) {
            val fillWidth = width * buildProgress / 100f
            canvas.withClip(clipPath) {
                if (layoutDirection == LAYOUT_DIRECTION_RTL) {
                    drawRect(width - fillWidth, 0f, width.toFloat(), height.toFloat(), fillPaint)
                } else {
                    drawRect(0f, 0f, fillWidth, height.toFloat(), fillPaint)
                }
            }
        }
        super.onDraw(canvas)
    }
}
