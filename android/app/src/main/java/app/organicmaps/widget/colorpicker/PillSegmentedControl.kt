package app.organicmaps.widget.colorpicker

import android.animation.ValueAnimator
import android.content.Context
import android.graphics.Canvas
import android.graphics.Paint
import android.graphics.RectF
import android.util.AttributeSet
import android.util.TypedValue
import android.view.Gravity
import android.view.animation.AccelerateDecelerateInterpolator
import android.widget.LinearLayout
import android.widget.TextView
import app.organicmaps.R
import app.organicmaps.util.ThemeUtils

class PillSegmentedControl @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0,
) : LinearLayout(context, attrs, defStyleAttr) {

    fun interface OnSegmentSelectedListener {
        fun onSegmentSelected(index: Int)
    }

    var onSegmentSelectedListener: OnSegmentSelectedListener? = null

    private val bgPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = ThemeUtils.getColor(context, android.R.attr.colorControlHighlight)
    }
    private val pillPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = ThemeUtils.getColor(context, R.attr.windowBackgroundForced)
    }
    private val bgRect = RectF()
    private val pillRect = RectF()
    private val cornerRadius = resources.getDimension(R.dimen.corner_radius_large)
    private val pillPadding = resources.getDimension(R.dimen.color_picker_pill_padding)

    private val labels = mutableListOf<TextView>()
    private var selectedIndex = 0
    private var animatedPillLeft = 0f
    private var pillAnimator: ValueAnimator? = null

    init {
        orientation = HORIZONTAL
        setWillNotDraw(false)
        val pad = pillPadding.toInt()
        setPadding(pad, pad, pad, pad)
    }

    fun setSegments(vararg titles: String) {
        labels.clear()
        removeAllViews()
        titles.forEachIndexed { i, title ->
            val tv = TextView(context).apply {
                text = title
                gravity = Gravity.CENTER
                setTextSize(TypedValue.COMPLEX_UNIT_SP, 14f)
                setTextColor(ThemeUtils.getColor(context, android.R.attr.textColorPrimary))
                isClickable = true
                setOnClickListener { selectSegment(i, animate = true) }
            }
            labels.add(tv)
            addView(tv, LayoutParams(0, LayoutParams.MATCH_PARENT, 1f))
        }
    }

    fun selectSegment(index: Int, animate: Boolean = false) {
        if (index !in labels.indices) return
        if (index == selectedIndex && pillAnimator == null) return
        selectedIndex = index

        val targetLeft = getPillLeft(index)
        if (animate && width > 0) {
            pillAnimator?.cancel()
            pillAnimator = ValueAnimator.ofFloat(animatedPillLeft, targetLeft).apply {
                duration = ANIMATION_DURATION_MS
                interpolator = AccelerateDecelerateInterpolator()
                addUpdateListener {
                    animatedPillLeft = it.animatedValue as Float
                    invalidate()
                }
                start()
            }
        } else {
            animatedPillLeft = targetLeft
            invalidate()
        }

        onSegmentSelectedListener?.onSegmentSelected(index)
    }

    override fun onDetachedFromWindow() {
        pillAnimator?.cancel()
        super.onDetachedFromWindow()
    }

    override fun onSizeChanged(w: Int, h: Int, oldw: Int, oldh: Int) {
        super.onSizeChanged(w, h, oldw, oldh)
        animatedPillLeft = getPillLeft(selectedIndex)
    }

    override fun onDraw(canvas: Canvas) {
        val w = width.toFloat()
        val h = height.toFloat()

        // Background pill
        bgRect.set(0f, 0f, w, h)
        canvas.drawRoundRect(bgRect, cornerRadius, cornerRadius, bgPaint)

        // Selected pill
        if (labels.isNotEmpty()) {
            val pillW = getPillWidth()
            pillRect.set(
                animatedPillLeft + pillPadding,
                pillPadding,
                animatedPillLeft + pillW - pillPadding,
                h - pillPadding,
            )
            canvas.drawRoundRect(pillRect, cornerRadius - pillPadding, cornerRadius - pillPadding, pillPaint)
        }

        super.onDraw(canvas)
    }

    private fun getPillWidth(): Float {
        if (labels.isEmpty()) return 0f
        return (width.toFloat() - paddingLeft - paddingRight) / labels.size
    }

    private fun getPillLeft(index: Int): Float = paddingLeft + getPillWidth() * index

    companion object {
        private const val ANIMATION_DURATION_MS = 200L
    }
}
