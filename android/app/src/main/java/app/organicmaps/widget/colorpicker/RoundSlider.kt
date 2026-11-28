package app.organicmaps.widget.colorpicker

import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.LinearGradient
import android.graphics.Paint
import android.graphics.RectF
import android.graphics.Shader
import android.os.Bundle
import android.util.AttributeSet
import android.view.KeyEvent
import android.view.MotionEvent
import android.view.View
import android.view.accessibility.AccessibilityNodeInfo
import android.widget.SeekBar
import androidx.annotation.ColorInt
import androidx.core.view.ViewCompat
import androidx.core.view.accessibility.AccessibilityNodeInfoCompat
import app.organicmaps.R
import app.organicmaps.util.ThemeUtils
import kotlin.math.roundToInt

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
            ViewCompat.setStateDescription(this, clamped.toString())
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

    init {
        isFocusable = true
        importantForAccessibility = IMPORTANT_FOR_ACCESSIBILITY_YES
        ViewCompat.setStateDescription(this, value.toString())
    }

    override fun onInitializeAccessibilityNodeInfo(info: AccessibilityNodeInfo) {
        super.onInitializeAccessibilityNodeInfo(info)
        info.className = SeekBar::class.java.name
        info.rangeInfo = AccessibilityNodeInfo.RangeInfo.obtain(
            AccessibilityNodeInfo.RangeInfo.RANGE_TYPE_INT,
            0f,
            255f,
            value.toFloat(),
        )
        if (!isEnabled) return
        info.isScrollable = true
        if (value < 255) info.addAction(AccessibilityNodeInfo.ACTION_SCROLL_FORWARD)
        if (value > 0) info.addAction(AccessibilityNodeInfo.ACTION_SCROLL_BACKWARD)
        AccessibilityNodeInfoCompat.wrap(info).addAction(
            AccessibilityNodeInfoCompat.AccessibilityActionCompat.ACTION_SET_PROGRESS,
        )
    }

    override fun performAccessibilityAction(action: Int, arguments: Bundle?): Boolean {
        return when (action) {
            AccessibilityNodeInfo.ACTION_SCROLL_FORWARD -> changeValue(value + ACCESSIBILITY_STEP)

            AccessibilityNodeInfo.ACTION_SCROLL_BACKWARD -> changeValue(value - ACCESSIBILITY_STEP)

            AccessibilityNodeInfoCompat.AccessibilityActionCompat.ACTION_SET_PROGRESS.id -> {
                if (!isEnabled) return false
                val requested = arguments?.getFloat(
                    AccessibilityNodeInfoCompat.ACTION_ARGUMENT_PROGRESS_VALUE,
                    Float.NaN,
                ) ?: return false
                if (requested.isFinite()) changeValue(requested.coerceIn(0f, 255f).roundToInt()) else false
            }

            else -> super.performAccessibilityAction(action, arguments)
        }
    }

    override fun onKeyDown(keyCode: Int, event: KeyEvent): Boolean {
        if (!isEnabled || !event.hasNoModifiers()) return super.onKeyDown(keyCode, event)
        return when (keyCode) {
            KeyEvent.KEYCODE_DPAD_LEFT -> changeValue(value - 1)
            KeyEvent.KEYCODE_DPAD_RIGHT -> changeValue(value + 1)
            KeyEvent.KEYCODE_MOVE_HOME -> changeValue(0)
            KeyEvent.KEYCODE_MOVE_END -> changeValue(255)
            else -> super.onKeyDown(keyCode, event)
        }
    }

    private fun changeValue(requested: Int): Boolean {
        if (!isEnabled) return false
        val clamped = requested.coerceIn(0, 255)
        if (clamped == value) return false
        value = clamped
        onValueChangedListener?.onValueChanged(this, value)
        return true
    }

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

    override fun onTouchEvent(event: MotionEvent): Boolean = if (!isEnabled) {
        isTracking = false
        parent?.requestDisallowInterceptTouchEvent(false)
        super.onTouchEvent(event)
    } else {
        when (event.actionMasked) {
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
    }

    private fun updateFromTouch(x: Float) {
        val w = width.toFloat()
        val trackStart = thumbRadius
        val trackEnd = w - thumbRadius
        if (trackEnd <= trackStart) return
        val newValue = ((x - trackStart) / (trackEnd - trackStart) * 255).toInt().coerceIn(0, 255)
        changeValue(newValue)
    }

    companion object {
        // Approximately five percent of the channel range per accessibility scroll action.
        private const val ACCESSIBILITY_STEP = 13
    }
}
