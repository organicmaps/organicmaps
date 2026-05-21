package app.organicmaps.widget.colorpicker

import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.LinearGradient
import android.graphics.Paint
import android.graphics.Path
import android.graphics.RectF
import android.graphics.Shader
import android.util.AttributeSet
import android.view.MotionEvent
import android.view.View
import androidx.annotation.ColorInt
import androidx.core.graphics.withSave
import app.organicmaps.R
import app.organicmaps.util.ThemeUtils

class SpectrumColorView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0,
) : View(context, attrs, defStyleAttr) {

    fun interface OnColorChangedListener {
        fun onColorChanged(@ColorInt color: Int)
    }

    private val huePaint = Paint(Paint.ANTI_ALIAS_FLAG)
    private val whitePaint = Paint(Paint.ANTI_ALIAS_FLAG)
    private val blackPaint = Paint(Paint.ANTI_ALIAS_FLAG)
    private val rect = RectF()
    private val clipPath = Path()

    private var hue = 0f
    private var saturation = 1f
    private var value = 1f

    private val selectorRadius = resources.getDimensionPixelSize(R.dimen.color_preset_circle_size) / 2f
    private val cornerRadius = resources.getDimension(R.dimen.corner_radius_medium)

    private var shaderDirty = true
    private var isTracking = false
    private val hsvTemp = FloatArray(3)

    var onColorChangedListener: OnColorChangedListener? = null

    private val selectorPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        style = Paint.Style.STROKE
        strokeWidth = resources.getDimension(R.dimen.color_picker_selector_stroke)
        color = ThemeUtils.getColor(context, android.R.attr.colorForeground)
    }

    fun setColor(@ColorInt color: Int) {
        if (isTracking) return
        Color.colorToHSV(color, hsvTemp)
        // For black (V=0), HSV gives S=0 but on spectrum black is at S=1,V=0 (right edge)
        if (hsvTemp[2] == 0f) hsvTemp[1] = 1f
        if (hue == hsvTemp[0] && saturation == hsvTemp[1] && value == hsvTemp[2]) return
        hue = hsvTemp[0]
        saturation = hsvTemp[1]
        value = hsvTemp[2]
        invalidate()
    }

    @ColorInt
    fun getSelectedColor(): Int {
        hsvTemp[0] = hue
        hsvTemp[1] = saturation
        hsvTemp[2] = value
        return Color.HSVToColor(hsvTemp)
    }

    private inline val contentWidth get() = width - paddingLeft - paddingRight
    private inline val contentHeight get() = height - paddingTop - paddingBottom

    override fun onSizeChanged(w: Int, h: Int, oldw: Int, oldh: Int) {
        super.onSizeChanged(w, h, oldw, oldh)
        shaderDirty = true
    }

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        val width = MeasureSpec.getSize(widthMeasureSpec)
        val heightMode = MeasureSpec.getMode(heightMeasureSpec)
        val height = if (heightMode == MeasureSpec.EXACTLY) {
            MeasureSpec.getSize(heightMeasureSpec)
        } else {
            (width * ASPECT_RATIO).toInt()
        }
        setMeasuredDimension(width, height)
    }

    override fun onDraw(canvas: Canvas) {
        val w = contentWidth
        val h = contentHeight
        if (w <= 0 || h <= 0) return

        val left = paddingLeft.toFloat()
        val top = paddingTop.toFloat()
        rect.set(left, top, left + w, top + h)

        if (shaderDirty) {
            shaderDirty = false
            buildShaders(left, top, w, h)
        }

        canvas.withSave {
            clipPath.reset()
            clipPath.addRoundRect(rect, cornerRadius, cornerRadius, Path.Direction.CW)
            clipPath(clipPath)

            drawRect(rect, huePaint)
            drawRect(left, top, left + w / 2f, top + h, whitePaint)
            drawRect(left + w / 2f, top, left + w, top + h, blackPaint)
        }

        val selectorY = (top + hue / 360f * h).coerceIn(top, top + h)
        val selectorX = hsvToX(left, w).coerceIn(left, left + w)
        canvas.drawCircle(selectorX, selectorY, selectorRadius, selectorPaint)
    }

    private fun buildShaders(left: Float, top: Float, w: Int, h: Int) {
        val hsv = floatArrayOf(0f, 1f, 1f)
        val hueColors = IntArray(7) { i ->
            hsv[0] = i * 60f
            Color.HSVToColor(hsv)
        }
        huePaint.shader = LinearGradient(
            left,
            top,
            left,
            top + h,
            hueColors,
            null,
            Shader.TileMode.CLAMP,
        )
        whitePaint.shader = LinearGradient(
            left,
            top,
            left + w / 2f,
            top,
            0xFFFFFFFF.toInt(),
            0x00FFFFFF,
            Shader.TileMode.CLAMP,
        )
        blackPaint.shader = LinearGradient(
            left + w / 2f,
            top,
            left + w,
            top,
            0x00000000,
            0xFF000000.toInt(),
            Shader.TileMode.CLAMP,
        )
    }

    // X axis: left=white (S=0,V=1), middle=color (S=1,V=1), right=black (S=1,V=0)
    private fun hsvToX(left: Float, w: Int): Float {
        val halfW = w / 2f
        return if (value >= saturation) {
            left + saturation * halfW
        } else {
            left + halfW + (1f - value) * halfW
        }
    }

    override fun onTouchEvent(event: MotionEvent): Boolean = when (event.actionMasked) {
        MotionEvent.ACTION_DOWN -> {
            isTracking = true
            parent?.requestDisallowInterceptTouchEvent(true)
            updateFromTouch(event.x, event.y)
            true
        }

        MotionEvent.ACTION_MOVE -> {
            updateFromTouch(event.x, event.y)
            true
        }

        MotionEvent.ACTION_UP -> {
            updateFromTouch(event.x, event.y)
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

    private fun updateFromTouch(x: Float, y: Float) {
        val w = contentWidth
        val h = contentHeight
        if (w <= 0 || h <= 0) return

        val clampedX = (x - paddingLeft).coerceIn(0f, w.toFloat())
        val clampedY = (y - paddingTop).coerceIn(0f, h.toFloat())

        hue = clampedY / h * 360f

        val halfW = w / 2f
        if (clampedX <= halfW) {
            saturation = clampedX / halfW
            value = 1f
        } else {
            saturation = 1f
            value = 1f - (clampedX - halfW) / halfW
        }
        invalidate()
        onColorChangedListener?.onColorChanged(getSelectedColor())
    }

    companion object {
        // Match ColorGridView aspect ratio: 10 rows / 12 cols
        private const val ASPECT_RATIO = 10f / 12f
    }
}
