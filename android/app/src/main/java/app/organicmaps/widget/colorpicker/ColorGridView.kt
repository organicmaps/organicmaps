package app.organicmaps.widget.colorpicker

import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.Path
import android.graphics.RectF
import android.util.AttributeSet
import android.view.MotionEvent
import android.view.View
import androidx.annotation.ColorInt
import androidx.core.graphics.withSave
import app.organicmaps.R
import app.organicmaps.util.ThemeUtils

class ColorGridView @JvmOverloads constructor(context: Context, attrs: AttributeSet? = null, defStyleAttr: Int = 0) :
    View(context, attrs, defStyleAttr) {

    fun interface OnColorChangedListener {
        fun onColorChanged(@ColorInt color: Int)
    }

    var onColorChangedListener: OnColorChangedListener? = null

    private val cellPaint = Paint(Paint.ANTI_ALIAS_FLAG)
    private val selectorPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        style = Paint.Style.STROKE
        strokeWidth = resources.getDimension(R.dimen.color_picker_selector_stroke)
        color = ThemeUtils.getColor(context, android.R.attr.colorForeground)
    }
    private val clipRect = RectF()
    private val clipPath = Path()
    private val cornerRadius = resources.getDimension(R.dimen.corner_radius_medium)

    private val colors: Array<IntArray> = buildColorGrid()
    private var selectedRow = -1
    private var selectedCol = -1
    private var isTracking = false

    private inline val rows get() = colors.size
    private inline val cols get() = colors[0].size

    fun setColor(@ColorInt color: Int) {
        if (isTracking) return
        for (r in colors.indices) {
            for (c in colors[r].indices) {
                if (colors[r][c] == color) {
                    if (selectedRow == r && selectedCol == c) return
                    selectedRow = r
                    selectedCol = c
                    invalidate()
                    return
                }
            }
        }
        if (selectedRow == -1 && selectedCol == -1) return
        selectedRow = -1
        selectedCol = -1
        invalidate()
    }

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        val width = MeasureSpec.getSize(widthMeasureSpec)
        val heightMode = MeasureSpec.getMode(heightMeasureSpec)
        val height = if (heightMode == MeasureSpec.EXACTLY) {
            MeasureSpec.getSize(heightMeasureSpec)
        } else {
            val cellSize = (width - paddingLeft - paddingRight).toFloat() / cols
            (cellSize * rows).toInt() + paddingTop + paddingBottom
        }
        setMeasuredDimension(width, height)
    }

    override fun onDraw(canvas: Canvas) {
        val w = width - paddingLeft - paddingRight
        val h = height - paddingTop - paddingBottom
        if (w <= 0 || h <= 0) return

        val left = paddingLeft.toFloat()
        val top = paddingTop.toFloat()
        val cellW = w.toFloat() / cols
        val cellH = h.toFloat() / rows

        canvas.withSave {
            clipPath.reset()
            clipRect.set(left, top, left + w, top + h)
            clipPath.addRoundRect(
                clipRect,
                cornerRadius,
                cornerRadius,
                Path.Direction.CW,
            )
            clipPath(clipPath)

            for (r in 0 until rows) {
                for (c in 0 until cols) {
                    cellPaint.color = colors[r][c]
                    val x = left + c * cellW
                    val y = top + r * cellH
                    drawRect(x, y, x + cellW, y + cellH, cellPaint)
                }
            }
        }

        if (selectedRow >= 0 && selectedCol >= 0) {
            val x = left + selectedCol * cellW
            val y = top + selectedRow * cellH
            val inset = selectorPaint.strokeWidth / 2f
            canvas.drawRoundRect(
                x + inset,
                y + inset,
                x + cellW - inset,
                y + cellH - inset,
                cornerRadius / 2,
                cornerRadius / 2,
                selectorPaint,
            )
        }
    }

    override fun onTouchEvent(event: MotionEvent): Boolean = when (event.actionMasked) {
        MotionEvent.ACTION_DOWN -> {
            isTracking = true
            parent?.requestDisallowInterceptTouchEvent(true)
            selectFromTouch(event.x, event.y)
            true
        }

        MotionEvent.ACTION_MOVE -> {
            selectFromTouch(event.x, event.y)
            true
        }

        MotionEvent.ACTION_UP -> {
            isTracking = false
            performClick()
            parent?.requestDisallowInterceptTouchEvent(false)
            true
        }

        MotionEvent.ACTION_CANCEL -> {
            isTracking = false
            parent?.requestDisallowInterceptTouchEvent(false)
            true
        }

        else -> super.onTouchEvent(event)
    }

    private fun selectFromTouch(x: Float, y: Float) {
        val w = width - paddingLeft - paddingRight
        val h = height - paddingTop - paddingBottom
        if (w <= 0 || h <= 0) return

        val col = ((x - paddingLeft) / w * cols).toInt().coerceIn(0, cols - 1)
        val row = ((y - paddingTop) / h * rows).toInt().coerceIn(0, rows - 1)

        if (row == selectedRow && col == selectedCol) return
        selectedRow = row
        selectedCol = col
        invalidate()
        onColorChangedListener?.onColorChanged(colors[row][col])
    }

    companion object {
        private const val COLS = 12
        private const val COLOR_ROWS = 9

        // Hue values per column
        // cyan, blue, indigo, purple, red, red-orange, orange, amber, yellow, yellow-green, green, teal
        private val COL_HUES = floatArrayOf(190f, 220f, 250f, 280f, 350f, 15f, 30f, 45f, 55f, 75f, 120f, 150f)

        private fun buildColorGrid(): Array<IntArray> {
            val totalRows = 1 + COLOR_ROWS
            val hsv = FloatArray(3)
            return Array(totalRows) { r ->
                IntArray(COLS) { c ->
                    if (r == 0) {
                        // Gray row: white → black
                        hsv[0] = 0f
                        hsv[1] = 0f
                        hsv[2] = 1f - c.toFloat() / (COLS - 1)
                    } else {
                        // Color rows: top = saturated/bright, bottom = pastel/light
                        hsv[0] = COL_HUES[c]
                        val fraction = (r - 1).toFloat() / (COLOR_ROWS - 1)
                        hsv[1] = 1f - fraction * 0.8f
                        hsv[2] = 0.6f + fraction * 0.4f
                    }
                    Color.HSVToColor(hsv)
                }
            }
        }
    }
}
