package app.organicmaps.syria

import android.annotation.SuppressLint
import android.app.Activity
import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.Path
import android.graphics.Rect
import android.graphics.RectF
import android.view.LayoutInflater
import android.view.MotionEvent
import android.view.View
import android.view.ViewGroup
import android.widget.CheckBox
import android.widget.FrameLayout
import android.widget.TextView
import androidx.annotation.IdRes
import androidx.annotation.StringRes
import androidx.core.view.ViewCompat
import androidx.core.view.WindowInsetsCompat
import app.organicmaps.R
import com.google.android.material.button.MaterialButton

/**
 * On-screen guide of the main map: dims the screen, spotlights one button at a time and explains it.
 * Opens by itself on the first visits until the user finishes it or ticks "don't show again"; the help
 * button above the traffic light opens it any time.
 */
object SyriaTutorial {
    /** One spotlighted button; with [tapTarget], tapping it in the spotlight presses it and goes on. */
    class Step(
        @IdRes val viewId: Int,
        @StringRes val title: Int,
        @StringRes val text: Int,
        val tapTarget: Boolean = false,
    )

    private val STEPS = listOf(
        Step(R.id.syria_recording_button, R.string.syria_tutorial_recording_title, R.string.syria_tutorial_recording_text),
        Step(R.id.syria_connectivity, R.string.syria_tutorial_internet_title, R.string.syria_tutorial_internet_text),
        Step(R.id.btn_search, R.string.syria_tutorial_search_title, R.string.syria_tutorial_search_text),
        Step(R.id.layers_button, R.string.syria_tutorial_layers_title, R.string.syria_tutorial_layers_text),
        Step(R.id.zoom_buttons_container, R.string.syria_tutorial_zoom_title, R.string.syria_tutorial_zoom_text),
        Step(R.id.my_position, R.string.syria_tutorial_position_title, R.string.syria_tutorial_position_text),
        Step(R.id.btn_bookmarks, R.string.syria_tutorial_bookmarks_title, R.string.syria_tutorial_bookmarks_text),
        Step(R.id.menu_button, R.string.syria_tutorial_menu_title, R.string.syria_tutorial_menu_text),
        Step(R.id.syria_help_button, R.string.syria_tutorial_help_title, R.string.syria_tutorial_help_text),
    )

    private var shownAutomatically = false

    /** Once per app start, until the guide was finished or the user asked not to see it again. */
    @JvmStatic
    fun showOnFirstVisits(activity: Activity, mapButtons: View) {
        if (shownAutomatically || SyriaPrefs.isTutorialDone(activity)) return
        shownAutomatically = true
        // After the map and its buttons are laid out.
        mapButtons.postDelayed({ if (!activity.isFinishing && mapButtons.isAttachedToWindow) show(activity, mapButtons) }, START_DELAY_MS)
    }

    @JvmStatic
    fun show(activity: Activity, mapButtons: View) {
        val targets = STEPS.mapNotNull { step -> mapButtons.findViewById<View>(step.viewId)?.let { step to it } }
        // Finishing the guide counts as seen; skipping shows it again next time unless the box is ticked.
        spotlight(activity, targets, showDontShow = true) { finished, dontShow ->
            if (finished || dontShow) SyriaPrefs.markTutorialDone(activity) else SyriaPrefs.resetTutorial(activity)
        }
    }

    /** Spotlights the visible ones of [steps] one after the other over the whole [activity]. */
    fun spotlight(
        activity: Activity,
        steps: List<Pair<Step, View>>,
        showDontShow: Boolean,
        onClose: (finished: Boolean, dontShow: Boolean) -> Unit,
    ) {
        val root = activity.window.decorView as ViewGroup
        if (root.findViewWithTag<View>(TAG) != null) return
        val targets = steps.filter { (_, view) -> view.isShown && view.width > 0 }
        if (targets.isEmpty()) return
        val overlay = Overlay(activity, targets, showDontShow, onClose)
        overlay.tag = TAG
        root.addView(overlay, ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT))
    }

    @SuppressLint("ViewConstructor")
    private class Overlay(
        context: Context,
        private val targets: List<Pair<Step, View>>,
        showDontShow: Boolean,
        private val onClose: (finished: Boolean, dontShow: Boolean) -> Unit,
    ) : FrameLayout(context) {
        private val dimPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply { color = DIM_COLOR }
        private val ringPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            style = Paint.Style.STROKE
            color = Color.WHITE
            strokeWidth = dp(2f)
        }
        private val hole = RectF()
        private val path = Path()
        private val card: View = LayoutInflater.from(context).inflate(R.layout.syria_tutorial_card, this, false)
        private val dontShow: CheckBox = card.findViewById(R.id.tutorial_dont_show)
        private var index = 0

        init {
            setWillNotDraw(false)
            isClickable = true
            addView(card)
            card.findViewById<MaterialButton>(R.id.tutorial_skip).setOnClickListener { close(finished = false) }
            card.findViewById<MaterialButton>(R.id.tutorial_back).setOnClickListener { go(index - 1) }
            card.findViewById<MaterialButton>(R.id.tutorial_next).setOnClickListener {
                if (index == targets.lastIndex) close(finished = true) else go(index + 1)
            }
            dontShow.isChecked = SyriaPrefs.isTutorialDone(context)
            dontShow.visibility = if (showDontShow) VISIBLE else GONE
            go(0)
        }

        private fun go(newIndex: Int) {
            index = newIndex.coerceIn(0, targets.lastIndex)
            val (step, _) = targets[index]
            card.findViewById<TextView>(R.id.tutorial_step).text =
                context.getString(R.string.syria_tutorial_step, index + 1, targets.size)
            card.findViewById<TextView>(R.id.tutorial_title).setText(step.title)
            card.findViewById<TextView>(R.id.tutorial_text).setText(step.text)
            card.findViewById<View>(R.id.tutorial_hint).visibility = if (step.tapTarget) VISIBLE else GONE
            card.findViewById<View>(R.id.tutorial_back).visibility = if (index > 0) VISIBLE else INVISIBLE
            card.findViewById<MaterialButton>(R.id.tutorial_next)
                .setText(if (index == targets.lastIndex) R.string.syria_tutorial_done else R.string.syria_tutorial_next)
            requestLayout()
            invalidate()
        }

        private fun close(finished: Boolean) {
            (parent as? ViewGroup)?.removeView(this)
            onClose(finished, dontShow.isChecked)
        }

        private fun targetRect(): RectF {
            val view = targets[index].second
            val target = IntArray(2).also { view.getLocationInWindow(it) }
            val self = IntArray(2).also { getLocationInWindow(it) }
            val pad = dp(PAD_DP)
            val r = Rect(target[0] - self[0], target[1] - self[1], 0, 0)
            r.right = r.left + view.width
            r.bottom = r.top + view.height
            return RectF(r.left - pad, r.top - pad, r.right + pad, r.bottom + pad)
        }

        override fun onLayout(changed: Boolean, left: Int, top: Int, right: Int, bottom: Int) {
            super.onLayout(changed, left, top, right, bottom)
            hole.set(targetRect())
            // The card goes below the spotlight when the button is in the upper half, above it otherwise.
            val margin = dp(MARGIN_DP).toInt()
            val bars = ViewCompat.getRootWindowInsets(this)?.getInsets(WindowInsetsCompat.Type.systemBars())
            val minTop = margin + (bars?.top ?: 0)
            val maxTop = maxOf(minTop, height - card.measuredHeight - margin - (bars?.bottom ?: 0))
            val cardTop = if (hole.centerY() < height / 2f) {
                (hole.bottom + margin).toInt()
            } else {
                (hole.top - margin - card.measuredHeight).toInt()
            }.coerceIn(minTop, maxTop)
            card.layout(margin, cardTop, width - margin, cardTop + card.measuredHeight)
        }

        override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
            super.onMeasure(widthMeasureSpec, heightMeasureSpec)
            val margin = dp(MARGIN_DP).toInt()
            card.measure(
                MeasureSpec.makeMeasureSpec(measuredWidth - 2 * margin, MeasureSpec.EXACTLY),
                MeasureSpec.makeMeasureSpec(measuredHeight, MeasureSpec.AT_MOST),
            )
        }

        override fun onDraw(canvas: Canvas) {
            val radius = dp(RADIUS_DP)
            path.reset()
            path.fillType = Path.FillType.EVEN_ODD
            path.addRect(0f, 0f, width.toFloat(), height.toFloat(), Path.Direction.CW)
            path.addRoundRect(hole, radius, radius, Path.Direction.CW)
            canvas.drawPath(path, dimPaint)
            canvas.drawRoundRect(hole, radius, radius, ringPaint)
        }

        // The screen underneath must not react while the guide is open, except the button to tap.
        @SuppressLint("ClickableViewAccessibility")
        override fun onTouchEvent(event: MotionEvent): Boolean {
            val (step, view) = targets[index]
            if (step.tapTarget && event.action == MotionEvent.ACTION_UP && hole.contains(event.x, event.y)) {
                if (index == targets.lastIndex) close(finished = true) else go(index + 1)
                view.performClick()
            }
            return true
        }

        private fun dp(value: Float): Float = value * resources.displayMetrics.density
    }

    private const val TAG = "syria_tutorial"
    private const val START_DELAY_MS = 1500L
    private const val DIM_COLOR = 0xB3000000.toInt()
    private const val PAD_DP = 6f
    private const val RADIUS_DP = 16f
    private const val MARGIN_DP = 16f
}
