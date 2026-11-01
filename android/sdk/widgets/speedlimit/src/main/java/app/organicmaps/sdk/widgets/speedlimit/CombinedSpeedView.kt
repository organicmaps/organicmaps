package app.organicmaps.sdk.widgets.speedlimit

import android.annotation.SuppressLint
import android.content.Context
import android.content.res.TypedArray
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.RectF
import android.util.AttributeSet
import android.view.MotionEvent
import android.view.View
import android.view.ViewGroup
import androidx.annotation.AttrRes
import androidx.annotation.StyleRes

/**
 * Shows the current speed and, optionally, the speed limit next to it, on a common background.
 *
 * The current speed is always visible, hide the whole widget to hide it. The speed limit is hidden
 * when it is unknown, see [setSpeedLimit].
 *
 * Use `wrap_content` width and a fixed height for horizontal positions, or a fixed width and
 * `wrap_content` height for top/bottom, to keep the sign size when the speed limit is hidden.
 *
 * The signs are created in the code, so that they can be styled from here: set
 * `om_widget_current_speed_view_style` and `om_widget_speed_limit_view_style` on this view
 * or in its style. Without them the signs use the styles from the theme, as standalone ones do.
 */
class CombinedSpeedView
@JvmOverloads
constructor(
    context: Context,
    attrs: AttributeSet? = null,
    @AttrRes defStyleAttr: Int = R.attr.om_widget_combined_speed_view_style,
    @StyleRes defStyleRes: Int = R.style.OmWidget_CombinedSpeedView,
) : ViewGroup(context, attrs, defStyleAttr, defStyleRes) {
    /** Position of the current speed relative to the speed limit; only Start/End follow layout direction. */
    enum class CurrentSpeedPosition {
        Top,
        Bottom,
        Left,
        Right,
        Start,
        End,
    }

    private val fillPaint: Paint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        style = Paint.Style.FILL
        color = Color.GRAY
    }

    private val currentSpeedView: CurrentSpeedView
    private val speedLimitView: SpeedLimitView
    private val connectorBounds = RectF()
    private var backgroundPalette = SignPalette.of(SignColors.DEFAULT)

    var currentSpeedPosition: CurrentSpeedPosition = CurrentSpeedPosition.Left
        set(value) {
            if (field == value) {
                return
            }
            field = value
            requestLayout()
            invalidate()
        }

    private val isVertical: Boolean
        get() = currentSpeedPosition == CurrentSpeedPosition.Top || currentSpeedPosition == CurrentSpeedPosition.Bottom

    private val isCurrentSpeedFirst: Boolean
        get() = when (currentSpeedPosition) {
            CurrentSpeedPosition.Top, CurrentSpeedPosition.Left -> true
            CurrentSpeedPosition.Bottom, CurrentSpeedPosition.Right -> false
            CurrentSpeedPosition.Start -> layoutDirection != LAYOUT_DIRECTION_RTL
            CurrentSpeedPosition.End -> layoutDirection == LAYOUT_DIRECTION_RTL
        }

    /** Shape of the speed-limit sign; the current speed is always square. */
    var shape: Shape = Shape.Circle
        set(value) {
            if (field == value) {
                return
            }
            field = value
            speedLimitView.shape = value
            invalidate()
        }

    /** State of both signs, selects their colors and the common background color. */
    var mode: Mode = Mode.Normal
        set(value) {
            if (field == value) {
                return
            }
            field = value
            currentSpeedView.mode = value
            speedLimitView.mode = value
            applyBackgroundColor()
            invalidate()
        }

    init {
        setWillNotDraw(false)

        // Read twice: the styles set on this very view must win over the ones from the theme,
        // while the ones coming from a default style must not, see signStyle().
        val ownAttributes = context.theme.obtainStyledAttributes(attrs, R.styleable.OmWidgetCombinedSpeedView, 0, 0)
        val attributes = context.theme.obtainStyledAttributes(
            attrs,
            R.styleable.OmWidgetCombinedSpeedView,
            defStyleAttr,
            defStyleRes,
        )
        try {
            val currentSpeedStyle = signStyle(
                ownAttributes,
                attributes,
                R.styleable.OmWidgetCombinedSpeedView_om_widget_current_speed_view_style,
                R.attr.om_widget_current_speed_view_style,
                R.style.OmWidget_CurrentSpeedView_Combined,
            )
            currentSpeedView = CurrentSpeedView(
                context,
                attrs = null,
                defStyleAttr = currentSpeedStyle.defStyleAttr,
                defStyleRes = currentSpeedStyle.defStyleRes,
            )

            val speedLimitStyle = signStyle(
                ownAttributes,
                attributes,
                R.styleable.OmWidgetCombinedSpeedView_om_widget_speed_limit_view_style,
                R.attr.om_widget_speed_limit_view_style,
                R.style.OmWidget_SpeedLimitView_Combined,
            )
            speedLimitView = SpeedLimitView(
                context,
                attrs = null,
                defStyleAttr = speedLimitStyle.defStyleAttr,
                defStyleRes = speedLimitStyle.defStyleRes,
            )

            val backgroundColor = attributes.getColor(
                R.styleable.OmWidgetCombinedSpeedView_om_widget_background_color,
                SignColors.DEFAULT.background,
            )
            backgroundPalette = SignPalette.from(
                context,
                attributes.getResourceId(R.styleable.OmWidgetCombinedSpeedView_om_widget_background_palette, 0),
                SignColors.DEFAULT.copy(background = backgroundColor),
            )
            shape = Shape.entries.toTypedArray()[
                attributes.getInt(R.styleable.OmWidgetCombinedSpeedView_om_widget_shape, shape.ordinal),
            ]
            mode = Mode.entries.toTypedArray()[
                attributes.getInt(R.styleable.OmWidgetCombinedSpeedView_om_widget_mode, mode.ordinal),
            ]
            currentSpeedPosition = CurrentSpeedPosition.entries[
                attributes.getInt(
                    R.styleable.OmWidgetCombinedSpeedView_om_widget_current_speed_position,
                    currentSpeedPosition.ordinal,
                ),
            ]
            if (isInEditMode) {
                currentSpeedView.speed = 60
                setSpeedLimit(90)
            } else {
                setSpeedLimit(null)
            }
        } finally {
            attributes.recycle()
            ownAttributes.recycle()
        }

        speedLimitView.shape = shape
        currentSpeedView.mode = mode
        speedLimitView.mode = mode
        applyBackgroundColor()

        currentSpeedView.isClickable = false
        speedLimitView.isClickable = false
        addView(currentSpeedView)
        addView(speedLimitView)
    }

    private fun applyBackgroundColor() {
        fillPaint.color = backgroundPalette[mode].background
    }

    /** Current speed, shown by the always visible sign. */
    var speed: Int
        get() = currentSpeedView.speed
        set(value) {
            currentSpeedView.speed = value
        }

    /** Localized units of the speed, e.g. "km/h". */
    var units: String
        get() = currentSpeedView.units
        set(value) {
            currentSpeedView.units = value
        }

    /** Sets the speed limit, null hides it, e.g. when it is unknown for the current road. */
    fun setSpeedLimit(speedLimit: Int?) {
        if (speedLimit == null) {
            speedLimitView.visibility = GONE
            return
        }
        speedLimitView.speedLimit = speedLimit
        speedLimitView.visibility = VISIBLE
    }

    override fun onDraw(canvas: Canvas) {
        if (currentSpeedView.visibility != VISIBLE || speedLimitView.visibility != VISIBLE) {
            return
        }

        canvas.drawRect(connectorBounds, fillPaint)
    }

    // The composite owns gestures, even though its child signs also consume touches on their own.
    override fun onInterceptTouchEvent(event: MotionEvent): Boolean = true

    @SuppressLint("ClickableViewAccessibility")
    override fun onTouchEvent(event: MotionEvent): Boolean {
        if (event.actionMasked == MotionEvent.ACTION_DOWN && !containsPoint(event.x, event.y)) {
            return false
        }
        if (isClickable || isLongClickable) {
            super.onTouchEvent(event)
        }
        return true
    }

    @SuppressLint("UseKtx")
    private fun containsPoint(x: Float, y: Float): Boolean {
        if (currentSpeedView.visibility == VISIBLE &&
            currentSpeedView.containsPoint(x - currentSpeedView.left, y - currentSpeedView.top)
        ) {
            return true
        }
        if (speedLimitView.visibility == VISIBLE &&
            speedLimitView.containsPoint(x - speedLimitView.left, y - speedLimitView.top)
        ) {
            return true
        }
        return currentSpeedView.visibility == VISIBLE && speedLimitView.visibility == VISIBLE &&
            connectorBounds.contains(x, y)
    }

    @SuppressLint("UseKtx")
    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        val signsCount = if (speedLimitView.visibility == GONE) 1 else 2
        val columns = if (isVertical) 1 else signsCount
        val rows = if (isVertical) signsCount else 1

        val horizontalPadding = paddingLeft + paddingRight
        val verticalPadding = paddingTop + paddingBottom
        val availableWidth = availableSize(widthMeasureSpec, horizontalPadding)
        val availableHeight = availableSize(heightMeasureSpec, verticalPadding)

        // Every sign is a square of the same size.
        val signSize = if (availableWidth == UNBOUNDED && availableHeight == UNBOUNDED) {
            if (isVertical) suggestedMinimumWidth else suggestedMinimumHeight
        } else {
            val widthPerSign = if (availableWidth == UNBOUNDED) UNBOUNDED else availableWidth / columns
            val heightPerSign = if (availableHeight == UNBOUNDED) UNBOUNDED else availableHeight / rows
            widthPerSign.coerceAtMost(heightPerSign)
        }.coerceAtLeast(0)

        val signSpec = MeasureSpec.makeMeasureSpec(signSize, MeasureSpec.EXACTLY)
        forEachVisibleSign { it.measure(signSpec, signSpec) }

        setMeasuredDimension(
            resolveSize(columns * signSize + horizontalPadding, widthMeasureSpec),
            resolveSize(rows * signSize + verticalPadding, heightMeasureSpec),
        )
    }

    override fun onRtlPropertiesChanged(layoutDirection: Int) {
        super.onRtlPropertiesChanged(layoutDirection)
        requestLayout()
        invalidate()
    }

    @SuppressLint("UseKtx")
    override fun onLayout(changed: Boolean, left: Int, top: Int, right: Int, bottom: Int) {
        val signsCount = if (speedLimitView.visibility == GONE) 1 else 2
        val signSize = currentSpeedView.measuredWidth
        val contentWidth = (width - paddingLeft - paddingRight).coerceAtLeast(0)
        val contentHeight = (height - paddingTop - paddingBottom).coerceAtLeast(0)
        val columns = if (isVertical) 1 else signsCount
        val rows = if (isVertical) signsCount else 1
        var signLeft = paddingLeft + (contentWidth - columns * signSize) / 2
        var signTop = paddingTop + (contentHeight - rows * signSize) / 2
        val signs = if (isCurrentSpeedFirst) {
            listOf(currentSpeedView, speedLimitView)
        } else {
            listOf(speedLimitView, currentSpeedView)
        }
        for (sign in signs) {
            if (sign.visibility == GONE) {
                continue
            }
            sign.layout(signLeft, signTop, signLeft + signSize, signTop + signSize)
            if (isVertical) {
                signTop += signSize
            } else {
                signLeft += signSize
            }
        }

        if (currentSpeedView.visibility == VISIBLE && speedLimitView.visibility == VISIBLE) {
            if (isVertical) {
                val currentSpeedCenter = currentSpeedView.top + signSize * 0.5f
                val speedLimitCenter = speedLimitView.top + signSize * 0.5f
                connectorBounds.set(
                    currentSpeedView.left.toFloat(),
                    minOf(currentSpeedCenter, speedLimitCenter),
                    currentSpeedView.right.toFloat(),
                    maxOf(currentSpeedCenter, speedLimitCenter),
                )
            } else {
                val currentSpeedCenter = currentSpeedView.left + signSize * 0.5f
                val speedLimitCenter = speedLimitView.left + signSize * 0.5f
                connectorBounds.set(
                    minOf(currentSpeedCenter, speedLimitCenter),
                    currentSpeedView.top.toFloat(),
                    maxOf(currentSpeedCenter, speedLimitCenter),
                    currentSpeedView.bottom.toFloat(),
                )
            }
        } else {
            connectorBounds.setEmpty()
        }
    }

    private inline fun forEachVisibleSign(action: (View) -> Unit) {
        for (view in listOf(currentSpeedView, speedLimitView)) {
            if (view.visibility != GONE) {
                action(view)
            }
        }
    }

    private fun availableSize(measureSpec: Int, padding: Int): Int {
        if (MeasureSpec.getMode(measureSpec) == MeasureSpec.UNSPECIFIED) {
            return UNBOUNDED
        }
        return (MeasureSpec.getSize(measureSpec) - padding).coerceAtLeast(0)
    }

    private companion object {
        /** Not limited by the parent, see [MeasureSpec.UNSPECIFIED]. */
        const val UNBOUNDED = Int.MAX_VALUE

        /** No style is set, so the next one in the resolution order is used. */
        const val NO_STYLE = 0

        /**
         * Resolves the style of a nested sign, keeping the usual resolution order:
         * the style set on this view, then the one from the theme attribute, then the one from
         * the default style of this view, then the library default.
         *
         * @param ownAttributes attributes of this view without any default style applied.
         * @param attributes attributes of this view with the default style applied.
         * @param index index of the sign style attribute.
         * @param themeAttr theme attribute that styles standalone signs of the same kind.
         * @param libraryStyle style used when neither of the above is set.
         */
        fun signStyle(
            ownAttributes: TypedArray,
            attributes: TypedArray,
            index: Int,
            @AttrRes themeAttr: Int,
            @StyleRes libraryStyle: Int,
        ): SignStyle {
            val ownStyle = ownAttributes.getResourceId(index, NO_STYLE)
            if (ownStyle != NO_STYLE) {
                // Set on this very view, nothing may override it.
                return SignStyle(defStyleAttr = NO_STYLE, defStyleRes = ownStyle)
            }

            // The theme attribute is checked first, the default style is only a fallback for it.
            val styleFromDefaultStyle = attributes.getResourceId(index, NO_STYLE)
            return SignStyle(
                defStyleAttr = themeAttr,
                defStyleRes = if (styleFromDefaultStyle != NO_STYLE) styleFromDefaultStyle else libraryStyle,
            )
        }
    }

    /** Style arguments of a nested sign, see [signStyle]. */
    private data class SignStyle(@param:AttrRes val defStyleAttr: Int, @param:StyleRes val defStyleRes: Int)
}
