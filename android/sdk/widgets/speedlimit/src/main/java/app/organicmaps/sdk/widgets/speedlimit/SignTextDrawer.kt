package app.organicmaps.sdk.widgets.speedlimit

import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.Rect
import android.graphics.RectF
import android.graphics.Typeface
import androidx.annotation.ColorInt
import kotlin.math.hypot
import kotlin.math.max

/**
 * Draws a numeric value with optional units inside a sign shape.
 *
 * All text sizes are precomputed in [update] from reference texts, never from the current value,
 * so the text does not jump when the value changes:
 * - without units the value is centered and fitted per digit count, so 2-digit values, which are
 *   wider when scaled to the same height, are not needlessly small;
 * - with units the sign is square, and all values share the size and baseline fitted for the
 *   widest, 3-digit value above the bottom-aligned units.
 */
internal class SignTextDrawer {
    private val valuePaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        textAlign = Paint.Align.CENTER
        typeface = Typeface.create(Typeface.DEFAULT, Typeface.BOLD)
        color = Color.BLACK
    }

    private val unitsPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        textAlign = Paint.Align.CENTER
        typeface = Typeface.DEFAULT
        color = Color.BLACK
    }

    private val textBounds = Rect()

    private var unitsOrNull: String? = null
    private var centerX = 0f
    private var shortValueTextSize = 0f
    private var shortValueBaselineY = 0f
    private var longValueTextSize = 0f
    private var longValueBaselineY = 0f
    private var unitsTextSize = 0f
    private var unitsBaselineY = 0f

    @get:ColorInt
    var color: Int
        get() = valuePaint.color
        set(@ColorInt value) {
            valuePaint.color = value
            unitsPaint.color = value
        }

    /**
     * Recalculates the text sizes and baselines.
     *
     * @param shape shape of a value-only sign; signs with units are always square.
     * @param shapeRect bounds of the shape outline, used to center the text.
     * @param innerSize size of the square/circle area bounded by the inner edge of the outline.
     * @param units units drawn below the value, no units row is drawn when it is null or empty.
     */
    fun update(shape: Shape, shapeRect: RectF, innerSize: Float, units: String? = null) {
        unitsOrNull = units?.takeIf { it.isNotEmpty() }
        centerX = shapeRect.centerX()

        val unitsText = unitsOrNull
        if (unitsText == null) {
            updateValueOnly(shape, shapeRect, innerSize)
        } else {
            updateValueWithUnits(shapeRect, innerSize, unitsText)
        }
    }

    fun draw(canvas: Canvas, value: String) {
        if (value.isEmpty()) {
            return
        }

        val isShortValue = value.length <= SHORT_REFERENCE_VALUE.length
        valuePaint.textSize = if (isShortValue) shortValueTextSize else longValueTextSize
        canvas.drawText(value, centerX, if (isShortValue) shortValueBaselineY else longValueBaselineY, valuePaint)

        unitsOrNull?.let { units ->
            unitsPaint.textSize = unitsTextSize
            canvas.drawText(units, centerX, unitsBaselineY, unitsPaint)
        }
    }

    private fun updateValueOnly(shape: Shape, shapeRect: RectF, innerSize: Float) {
        shortValueTextSize = fitValueTextSize(SHORT_REFERENCE_VALUE, shape, innerSize)
        shortValueBaselineY = centeredBaselineY(shortValueTextSize, shapeRect)
        longValueTextSize = fitValueTextSize(LONG_REFERENCE_VALUE, shape, innerSize)
        longValueBaselineY = centeredBaselineY(longValueTextSize, shapeRect)
    }

    private fun updateValueWithUnits(shapeRect: RectF, innerSize: Float, units: String) {
        // Measure the two rows at the reference size, their ratio does not depend on the scale.
        valuePaint.textSize = REFERENCE_TEXT_SIZE
        valuePaint.getTextBounds(LONG_REFERENCE_VALUE, 0, LONG_REFERENCE_VALUE.length, textBounds)
        val valueWidth = textBounds.width().toFloat()
        val valueHeight = textBounds.height().toFloat()

        unitsPaint.textSize = REFERENCE_TEXT_SIZE * UNITS_TEXT_SIZE_RATIO
        unitsPaint.getTextBounds(units, 0, units.length, textBounds)
        val unitsWidth = textBounds.width().toFloat()
        val unitsHeight = textBounds.height().toFloat()

        val rowsGap = REFERENCE_TEXT_SIZE * ROWS_GAP_RATIO
        val blockWidth = max(valueWidth, unitsWidth)
        val blockHeight = valueHeight + rowsGap + unitsHeight
        val scale = fitScale(Shape.Square, innerSize, blockWidth, blockHeight)

        unitsTextSize = REFERENCE_TEXT_SIZE * scale * UNITS_TEXT_SIZE_RATIO

        unitsPaint.textSize = unitsTextSize
        unitsPaint.getTextBounds(units, 0, units.length, textBounds)
        val scaledUnitsTop = textBounds.top.toFloat()
        val scaledUnitsBottom = textBounds.bottom.toFloat()

        val scaledGap = rowsGap * scale

        val halfSize = innerSize * TEXT_FILL_RATIO * 0.5f
        unitsBaselineY = shapeRect.centerY() + halfSize - scaledUnitsBottom
        updateValueAboveUnits(shapeRect, innerSize, unitsBaselineY + scaledUnitsTop - scaledGap)
    }

    private fun updateValueAboveUnits(shapeRect: RectF, innerSize: Float, bottom: Float) {
        valuePaint.textSize = REFERENCE_TEXT_SIZE
        valuePaint.getTextBounds(LONG_REFERENCE_VALUE, 0, LONG_REFERENCE_VALUE.length, textBounds)
        val width = textBounds.width().toFloat()
        val height = textBounds.height().toFloat()
        val halfSize = innerSize * TEXT_FILL_RATIO * 0.5f
        val bottomOffset = bottom - shapeRect.centerY()
        var scale = fitScale(Shape.Square, innerSize, width, height)

        if (height * scale * 0.5f > bottomOffset) {
            scale = ((bottomOffset + halfSize) / height).coerceAtMost(scale)
        }

        longValueTextSize = REFERENCE_TEXT_SIZE * scale
        shortValueTextSize = longValueTextSize
        valuePaint.textSize = longValueTextSize
        valuePaint.getTextBounds(LONG_REFERENCE_VALUE, 0, LONG_REFERENCE_VALUE.length, textBounds)
        val availableTop = shapeRect.centerY() - halfSize
        val availableBottom = (shapeRect.centerY() + halfSize).coerceAtMost(bottom)
        // Center the ink in the space above the units, not against its bottom edge.
        longValueBaselineY = (availableTop + availableBottom - textBounds.top - textBounds.bottom) * 0.5f
        shortValueBaselineY = longValueBaselineY
    }

    private fun fitValueTextSize(reference: String, shape: Shape, innerSize: Float): Float {
        valuePaint.textSize = REFERENCE_TEXT_SIZE
        valuePaint.getTextBounds(reference, 0, reference.length, textBounds)
        val scale = fitScale(shape, innerSize, textBounds.width().toFloat(), textBounds.height().toFloat())
        return REFERENCE_TEXT_SIZE * scale
    }

    /** Scale that fits a block of the given size into the inner area of the shape. */
    private fun fitScale(shape: Shape, innerSize: Float, blockWidth: Float, blockHeight: Float): Float {
        if (blockWidth <= 0f || blockHeight <= 0f) {
            return 0f
        }

        val scale = when (shape) {
            // The largest rectangle with the block aspect ratio inscribed into the inner circle:
            // (scale * w)^2 + (scale * h)^2 = diameter^2.
            Shape.Circle -> innerSize / hypot(blockWidth, blockHeight)

            Shape.Square -> (innerSize / blockWidth).coerceAtMost(innerSize / blockHeight)
        }
        return scale * TEXT_FILL_RATIO
    }

    /** Centers the reference digit ink box, not the current value's one, to keep the baseline stable. */
    private fun centeredBaselineY(textSize: Float, shapeRect: RectF): Float {
        valuePaint.textSize = textSize
        valuePaint.getTextBounds(REFERENCE_DIGIT, 0, REFERENCE_DIGIT.length, textBounds)
        return shapeRect.centerY() - (textBounds.top + textBounds.bottom) * 0.5f
    }

    private companion object {
        /** Digits are tabular in the used fonts, '8' has the largest ink box. */
        const val REFERENCE_DIGIT = "8"
        const val SHORT_REFERENCE_VALUE = "$REFERENCE_DIGIT$REFERENCE_DIGIT"
        const val LONG_REFERENCE_VALUE = "$SHORT_REFERENCE_VALUE$REFERENCE_DIGIT"
        const val REFERENCE_TEXT_SIZE = 100f

        /** Small margin so the text does not touch the outline. */
        const val TEXT_FILL_RATIO = 0.9f
        const val UNITS_TEXT_SIZE_RATIO = 0.45f
        const val ROWS_GAP_RATIO = 0.1f
    }
}
