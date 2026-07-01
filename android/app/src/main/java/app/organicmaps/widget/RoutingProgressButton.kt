package app.organicmaps.widget

import android.animation.ValueAnimator
import android.content.Context
import android.graphics.Canvas
import android.graphics.Paint
import android.graphics.Path
import android.graphics.RectF
import android.util.AttributeSet
import android.view.animation.AccelerateDecelerateInterpolator
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
        private set

    private var displayedProgress = 0
    private var isPending = false

    private var animator: ValueAnimator? = null

    fun setBuildProgress(progress: Int) {
        val clamped = progress.coerceIn(0, 100)
        buildProgress = clamped
        if (isPending && clamped == 0) {
            return
        }
        isPending = false
        cancelAnimator()
        if (clamped == displayedProgress) return
        displayedProgress = clamped
        invalidate()
    }

    fun setPending(pending: Boolean) {
        if (!pending && !isPending) return
        isPending = pending
        cancelAnimator()
        if (pending) {
            displayedProgress = 0
            animator = ValueAnimator.ofFloat(0f, 1f).apply {
                duration = BREATH_HALF_CYCLE_MS
                interpolator = AccelerateDecelerateInterpolator()
                repeatMode = ValueAnimator.REVERSE
                repeatCount = ValueAnimator.INFINITE
                addUpdateListener {
                    displayedProgress = ((it.animatedValue as Float) * BREATH_PEAK).toInt()
                    invalidate()
                }
                start()
            }
        } else {
            invalidate()
        }
    }

    private fun cancelAnimator() {
        animator?.cancel()
        animator = null
    }

    override fun onSizeChanged(w: Int, h: Int, oldw: Int, oldh: Int) {
        super.onSizeChanged(w, h, oldw, oldh)
        clipRect.set(0f, 0f, w.toFloat(), h.toFloat())
        clipPath.rewind()
        clipPath.addRoundRect(clipRect, cornerRadius, cornerRadius, Path.Direction.CW)
    }

    override fun onDraw(canvas: Canvas) {
        if (displayedProgress > 0) {
            val fillWidth = width * displayedProgress / 100f
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

    override fun onDetachedFromWindow() {
        cancelAnimator()
        super.onDetachedFromWindow()
    }

    companion object {
        private const val BREATH_HALF_CYCLE_MS = 900L
        private const val BREATH_PEAK = 100
    }
}
