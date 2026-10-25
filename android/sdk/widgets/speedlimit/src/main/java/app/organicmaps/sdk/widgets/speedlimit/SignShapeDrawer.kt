package app.organicmaps.sdk.widgets.speedlimit

import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.Path
import android.graphics.RectF
import androidx.annotation.ColorInt
import androidx.core.graphics.ColorUtils

/**
 * Draws the background of a road sign, its optional outline and outer frame.
 *
 * The sign is always a square of [update]'s size, rounded according to the requested [Shape].
 */
internal class SignShapeDrawer {
    private val backgroundPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        style = Paint.Style.FILL
        color = Color.WHITE
    }

    private val outlinePaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        style = Paint.Style.FILL
        color = Color.RED
    }

    private val borderPaint = Paint(Paint.ANTI_ALIAS_FLAG)
    private val framePath = Path()
    private val outlinePath = Path()
    private val outlineBounds = RectF()
    private var outlineCornerRadius = 0f
    private val innerBounds = RectF()
    private var innerCornerRadius = 0f

    /** Outline centerline bounds used to position the text. */
    val bounds = RectF()

    // Outer edge of the outline, used for hit testing.
    private val outerBounds = RectF()
    private var outerCornerRadius = 0f

    /** Size of the square area bounded by the inner edge of the outline. */
    var innerSize = 0f
        private set

    @get:ColorInt
    var backgroundColor: Int
        get() = backgroundPaint.color
        set(@ColorInt value) {
            backgroundPaint.color = value
        }

    @get:ColorInt
    var outlineColor: Int
        get() = outlinePaint.color
        set(@ColorInt value) {
            outlinePaint.color = value
        }

    @get:ColorInt
    var frameColor: Int = Color.WHITE

    /** Outer frame width as a fraction of the sign size, zero means no frame. */
    var frameWidthRatio: Float = 0f

    /** Outline width as a fraction of the sign size, zero means no outline. */
    var outlineWidthRatio: Float = DEFAULT_OUTLINE_WIDTH_RATIO

    /** Corner radius as a fraction of the sign size, see [Shape.cornerRadiusRatio]. */
    var cornerRadiusRatio: Float = Shape.Circle.cornerRadiusRatio

    /**
     * Recalculates the sign geometry.
     *
     * @param left left edge of the square area occupied by the sign.
     * @param top top edge of the square area occupied by the sign.
     * @param size side of the square area occupied by the sign.
     */
    fun update(left: Float, top: Float, size: Float) {
        val frameWidth = size * frameWidthRatio
        val outlineWidth = size * outlineWidthRatio
        outlinePaint.strokeWidth = outlineWidth
        innerSize = size - 2f * (frameWidth + outlineWidth)

        val inset = frameWidth + outlineWidth * 0.5f
        bounds.set(left + inset, top + inset, left + size - inset, top + size - inset)

        // The ratio is relative to the outer edge of the sign, so that the visible corners match
        // the ones of a container background of the same size, regardless of the outline width.
        outerBounds.set(left, top, left + size, top + size)
        outerCornerRadius = size * cornerRadiusRatio

        outlineBounds.set(outerBounds)
        outlineBounds.inset(frameWidth, frameWidth)
        outlineCornerRadius = (outerCornerRadius - frameWidth).coerceAtLeast(0f)
        framePath.rewind()
        framePath.fillType = Path.FillType.EVEN_ODD
        framePath.addRoundRect(outerBounds, outerCornerRadius, outerCornerRadius, Path.Direction.CW)
        framePath.addRoundRect(outlineBounds, outlineCornerRadius, outlineCornerRadius, Path.Direction.CW)

        innerBounds.set(outlineBounds)
        innerBounds.inset(outlineWidth, outlineWidth)
        innerCornerRadius = (outlineCornerRadius - outlineWidth).coerceAtLeast(0f)
        outlinePath.rewind()
        outlinePath.fillType = Path.FillType.EVEN_ODD
        outlinePath.addRoundRect(outlineBounds, outlineCornerRadius, outlineCornerRadius, Path.Direction.CW)
        outlinePath.addRoundRect(innerBounds, innerCornerRadius, innerCornerRadius, Path.Direction.CW)
    }

    /** Tests whether the point is inside the visible sign, including its outline and frame. */
    fun contains(x: Float, y: Float): Boolean = isInsideRoundRect(outerBounds, outerCornerRadius, x, y)

    fun draw(canvas: Canvas) {
        if (frameWidthRatio > 0f) {
            drawBorder(canvas, outerBounds, outerCornerRadius, framePath, frameColor)
        }
        if (outlinePaint.strokeWidth > 0f) {
            drawBorder(canvas, outlineBounds, outlineCornerRadius, outlinePath, outlinePaint.color)
        }
        canvas.drawRoundRect(innerBounds, innerCornerRadius, innerCornerRadius, backgroundPaint)
    }

    private fun drawBorder(canvas: Canvas, bounds: RectF, radius: Float, path: Path, @ColorInt color: Int) {
        // Precompose each border over the background, not over the other border.
        borderPaint.color = ColorUtils.compositeColors(color, backgroundPaint.color)
        if (Color.alpha(backgroundPaint.color) == 255) {
            canvas.drawRoundRect(bounds, radius, radius, borderPaint)
        } else {
            canvas.drawPath(path, borderPaint)
        }
    }

    private companion object {
        const val DEFAULT_OUTLINE_WIDTH_RATIO = 0.07f
    }
}
