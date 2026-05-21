package app.organicmaps.widget.colorpicker

import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.LinearGradient
import android.graphics.Paint
import android.graphics.RectF
import android.graphics.Shader
import android.util.AttributeSet
import android.view.MotionEvent
import android.view.View
import androidx.annotation.ColorInt
import app.organicmaps.R
import app.organicmaps.util.ThemeUtils

/** Custom rounded slider with gradient track and circular thumb. */
class RoundSlider @JvmOverloads constructor(context: Context, attrs: AttributeSet? = null, defStyleAttr: Int = 0) :
    View(context, attrs, defStyleAttr) {

    fun interface OnValueChangedListener {
        fun onValueChanged(slider: RoundSlider, value: Int)
    }

    var onValueChangedListener: OnValueChangedListener? = null
    var isTracking = false
        private set

    var value: Int = 0
        set(v) {
            val clamped = v.coerceIn(0, 255)
            if (field == clamped) return
            field = clamped
            invalidate()
        }

    @ColorInt
    var thumbColor: Int = Color.WHITE
        set(value) {
            if (field == value) return
            field = value
            invalidate()
        }

    private val trackPaint = Paint(Paint.ANTI_ALIAS_FLAG)
    private val thumbFillPaint = Paint(Paint.ANTI_ALIAS_FLAG)
    private val selectorStroke = resources.getDimension(R.dimen.color_picker_selector_stroke)
    private val thumbStrokePaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        style = Paint.Style.STROKE
        strokeWidth = selectorStroke
        color = ThemeUtils.getColor(context, android.R.attr.colorForeground)
    }
    private val trackRect = RectF()

    private var startColor = Color.BLACK
    private var endColor = Color.WHITE
    private var shaderDirty = true

    private val trackCornerRadius = resources.getDimension(R.dimen.color_picker_slider_track_corner_radius)
    private val trackHeight = resources.getDimension(R.dimen.color_picker_slider_track_height)
    private var thumbRadius = resources.getDimensionPixelSize(R.dimen.color_preset_circle_size) / 2f

    override fun onSizeChanged(w: Int, h: Int, oldw: Int, oldh: Int) {
        super.onSizeChanged(w, h, oldw, oldh)
        thumbRadius = (h / 2f - selectorStroke).coerceAtLeast(0f)
        shaderDirty = true
        invalidate()
    }

    fun setGradient(@ColorInt start: Int, @ColorInt end: Int) {
        if (start == startColor && end == endColor) return
        startColor = start
        endColor = end
        shaderDirty = true
        invalidate()
    }

    override fun onDraw(canvas: Canvas) {
        val w = width.toFloat()
        val h = height.toFloat()
        if (w <= 0f || h <= 0f) return
        val centerY = h / 2f
        val trackH = trackHeight

        trackRect.set(0f, centerY - trackH / 2, w, centerY + trackH / 2)
        if (shaderDirty) {
            shaderDirty = false
            trackPaint.shader = LinearGradient(0f, 0f, w, 0f, startColor, endColor, Shader.TileMode.CLAMP)
        }
        canvas.drawRoundRect(trackRect, trackCornerRadius, trackCornerRadius, trackPaint)

        val fraction = value / 255f
        val thumbX = thumbRadius + (w - thumbRadius * 2) * fraction
        val thumbY = centerY
        thumbFillPaint.color = thumbColor
        canvas.drawCircle(thumbX, thumbY, thumbRadius, thumbFillPaint)
        canvas.drawCircle(thumbX, thumbY, thumbRadius, thumbStrokePaint)
    }

    override fun onTouchEvent(event: MotionEvent): Boolean = when (event.actionMasked) {
        MotionEvent.ACTION_DOWN -> {
            isTracking = true
            parent?.requestDisallowInterceptTouchEvent(true)
            updateFromTouch(event.x)
            true
        }

        MotionEvent.ACTION_MOVE -> {
            updateFromTouch(event.x)
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

    private fun updateFromTouch(x: Float) {
        val w = width.toFloat()
        val trackStart = thumbRadius
        val trackEnd = w - thumbRadius
        if (trackEnd <= trackStart) return
        val newValue = ((x - trackStart) / (trackEnd - trackStart) * 255).toInt().coerceIn(0, 255)
        if (newValue != value) {
            value = newValue
            onValueChangedListener?.onValueChanged(this, value)
        }
    }
}
