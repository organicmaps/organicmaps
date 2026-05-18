package app.organicmaps.widget

import android.graphics.Canvas
import android.graphics.ColorFilter
import android.graphics.Outline
import android.graphics.Paint
import android.graphics.Path
import android.graphics.PixelFormat
import android.graphics.Rect
import android.graphics.RectF
import android.graphics.drawable.Drawable
import android.util.SizeF
import androidx.annotation.ColorInt

/**
 * Rounded rectangle popup background with a triangle arrow pointing toward the anchor,
 * plus a soft shadow behind the arrow to match the popup's elevation shadow.
 */
class ArrowPopupBackground(
    @ColorInt bgColor: Int,
    @ColorInt shadowColor: Int,
    private val radius: Float,
    private val arrowSize: SizeF,
    private val shadowDy: Float,
    private val arrowOnTop: Boolean,
) : Drawable() {
    private val paint = Paint(Paint.ANTI_ALIAS_FLAG).apply { color = bgColor }
    private val shadowPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply { color = shadowColor }
    private val path = Path()
    private val rect = RectF()

    /**
     * Horizontal offset of the arrow tip from the popup's left edge, in pixels.
     * `null` (default) means "draw the arrow at the popup's horizontal center".
     */
    var arrowCx: Float? = null
        set(value) {
            field = value
            invalidateSelf()
        }

    override fun draw(canvas: Canvas) {
        val b = bounds
        val cx = arrowCx?.let { b.left + it } ?: b.exactCenterX()
        val arrowBase = if (arrowOnTop) b.top + arrowSize.height else b.bottom - arrowSize.height
        val arrowTip = if (arrowOnTop) b.top.toFloat() else b.bottom.toFloat()
        val shadowTip = if (arrowOnTop) b.top - shadowDy else b.bottom + shadowDy

        // Body: rounded rectangle.
        if (arrowOnTop) {
            rect.set(b.left.toFloat(), arrowBase, b.right.toFloat(), b.bottom.toFloat())
        } else {
            rect.set(b.left.toFloat(), b.top.toFloat(), b.right.toFloat(), arrowBase)
        }
        path.reset()
        path.addRoundRect(rect, radius, radius, Path.Direction.CW)
        canvas.drawPath(path, paint)

        // Arrow shadow.
        path.reset()
        path.moveTo(cx - arrowSize.width / 2 - shadowDy, arrowBase)
        path.lineTo(cx, shadowTip)
        path.lineTo(cx + arrowSize.width / 2 + shadowDy, arrowBase)
        path.close()
        canvas.drawPath(path, shadowPaint)

        // Arrow triangle.
        path.reset()
        path.moveTo(cx - arrowSize.width / 2, arrowBase)
        path.lineTo(cx, arrowTip)
        path.lineTo(cx + arrowSize.width / 2, arrowBase)
        path.close()
        canvas.drawPath(path, paint)
    }

    override fun getPadding(padding: Rect): Boolean {
        val h = arrowSize.height.toInt()
        if (arrowOnTop) {
            padding.set(0, h, 0, 0)
        } else {
            padding.set(0, 0, 0, h)
        }
        return true
    }

    override fun getOutline(outline: Outline) {
        val b = bounds
        val h = arrowSize.height.toInt()
        if (arrowOnTop) {
            outline.setRoundRect(b.left, b.top + h, b.right, b.bottom, radius)
        } else {
            outline.setRoundRect(b.left, b.top, b.right, b.bottom - h, radius)
        }
    }

    override fun setAlpha(alpha: Int) {
        paint.alpha = alpha
    }

    override fun setColorFilter(colorFilter: ColorFilter?) {
        paint.colorFilter = colorFilter
    }

    @Suppress("OVERRIDE_DEPRECATION")
    override fun getOpacity(): Int = PixelFormat.TRANSLUCENT
}
