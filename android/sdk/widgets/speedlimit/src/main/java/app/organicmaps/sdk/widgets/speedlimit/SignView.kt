package app.organicmaps.sdk.widgets.speedlimit

import android.annotation.SuppressLint
import android.content.Context
import android.graphics.Canvas
import android.util.AttributeSet
import android.view.MotionEvent
import android.view.View
import androidx.annotation.AttrRes
import androidx.annotation.StyleRes

/**
 * Base class for the road sign views: a square shape with a value inside, centered in the padded
 * area of the view.
 */
abstract class SignView
@JvmOverloads
constructor(
    context: Context,
    attrs: AttributeSet? = null,
    @AttrRes defStyleAttr: Int = 0,
    @StyleRes defStyleRes: Int = 0,
    fixedShape: Shape? = null,
) : View(context, attrs, defStyleAttr, defStyleRes) {
    private val shapeDrawer = SignShapeDrawer()
    private val textDrawer = SignTextDrawer()
    private var circlePalette = SignPalette.of(SignColors.DEFAULT)
    private var squarePalette = SignPalette.of(SignColors.DEFAULT)

    /** Text of the value, cached to avoid allocating a string on every frame. */
    protected var valueText: String = MIN_VALUE.toString()

    /** Units drawn below the value, null when the sign shows the value only. */
    protected open val unitsText: String? get() = null

    /** Shape of the sign, the colors of the current [mode] are applied from its palette. */
    open var shape: Shape = fixedShape ?: Shape.Circle
        protected set(value) {
            if (field == value) {
                return
            }
            field = value
            applyColors()
            applyCornerRadiusRatio()
            updateGeometry()
            invalidate()
        }

    /** State of the sign, selects the colors from the palette of the current [shape]. */
    var mode: Mode = Mode.Normal
        set(value) {
            if (field == value) {
                return
            }
            field = value
            applyColors()
            invalidate()
        }

    /**
     * Corner radius of the sign as a fraction of its size, the one of the current [shape] unless
     * it is overridden in the style.
     */
    val cornerRadiusRatio: Float get() = shapeDrawer.cornerRadiusRatio

    /** Corner radius ratio from the style, null when it follows the [shape]. */
    private var styledCornerRadiusRatio: Float? = null

    init {
        // The widget is placed on top of the map, so it must consume touches instead of letting
        // them through to the map below. Clicks are reported via setOnClickListener() as usual.
        isClickable = true

        val attributes = context.theme.obtainStyledAttributes(
            attrs,
            R.styleable.OmWidgetSignView,
            defStyleAttr,
            defStyleRes,
        )
        try {
            if (fixedShape == null) {
                shape = Shape.entries.toTypedArray()[
                    attributes.getInt(R.styleable.OmWidgetSignView_om_widget_shape, shape.ordinal),
                ]
                styledCornerRadiusRatio = attributes
                    .getFloat(R.styleable.OmWidgetSignView_om_widget_corner_radius_ratio, Float.NaN)
                    .takeUnless { it.isNaN() }
            }
            mode =
                Mode.entries.toTypedArray()[
                    attributes.getInt(
                        R.styleable.OmWidgetSignView_om_widget_mode,
                        mode.ordinal,
                    ),
                ]
            shapeDrawer.frameWidthRatio = attributes.getFloat(
                R.styleable.OmWidgetSignView_om_widget_frame_width_ratio,
                shapeDrawer.frameWidthRatio,
            )
            shapeDrawer.outlineWidthRatio = attributes.getFloat(
                R.styleable.OmWidgetSignView_om_widget_outline_width_ratio,
                shapeDrawer.outlineWidthRatio,
            )
            circlePalette = SignPalette.from(
                context,
                attributes.getResourceId(R.styleable.OmWidgetSignView_om_widget_circle_palette, 0),
                SignColors.DEFAULT,
            )
            squarePalette = SignPalette.from(
                context,
                attributes.getResourceId(R.styleable.OmWidgetSignView_om_widget_square_palette, 0),
                SignColors.DEFAULT,
            )
            if (isInEditMode) {
                valueText = attributes.getInt(R.styleable.OmWidgetSignView_om_widget_value, MIN_VALUE)
                    .coerceIn(MIN_VALUE, MAX_VALUE)
                    .toString()
            }
        } finally {
            attributes.recycle()
        }
        applyColors()
        applyCornerRadiusRatio()
    }

    /** Applies the corner radius of the current [shape], unless the style overrides it. */
    private fun applyCornerRadiusRatio() {
        shapeDrawer.cornerRadiusRatio = styledCornerRadiusRatio ?: shape.cornerRadiusRatio
    }

    /** Applies the colors of the current [shape] and [mode]. */
    private fun applyColors() {
        val colors = when (shape) {
            Shape.Circle -> circlePalette[mode]
            Shape.Square -> squarePalette[mode]
        }
        shapeDrawer.backgroundColor = colors.background
        shapeDrawer.frameColor = colors.frame
        shapeDrawer.outlineColor = colors.outline
        textDrawer.color = colors.text
    }

    override fun onDraw(canvas: Canvas) {
        shapeDrawer.draw(canvas)
        textDrawer.draw(canvas, valueText)
    }

    /** Tests the visible sign shape in this view's local coordinates. */
    internal fun containsPoint(x: Float, y: Float): Boolean = shapeDrawer.contains(x, y)

    // Clicks are detected and reported by View.onTouchEvent(), which is called below.
    @SuppressLint("ClickableViewAccessibility")
    override fun onTouchEvent(event: MotionEvent): Boolean {
        // Touches outside of the sign, e.g. in the corners of a circle one, are not consumed
        // and are dispatched further, to the map below.
        if (event.actionMasked == MotionEvent.ACTION_DOWN && !containsPoint(event.x, event.y)) {
            return false
        }
        return super.onTouchEvent(event)
    }

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        val horizontalPadding = paddingLeft + paddingRight
        val verticalPadding = paddingTop + paddingBottom

        // The sign is a square, so by default the view takes the smallest of the available sizes.
        val availableWidth = availableSize(widthMeasureSpec, horizontalPadding)
        val availableHeight = availableSize(heightMeasureSpec, verticalPadding)
        val contentSize = if (availableWidth == UNBOUNDED && availableHeight == UNBOUNDED) {
            suggestedMinimumHeight
        } else {
            availableWidth.coerceAtMost(availableHeight)
        }

        // An exact size must be obeyed, e.g. when the view is laid out with a layout_weight.
        // The square sign is centered inside such a non-square view, see updateGeometry().
        val width = if (MeasureSpec.getMode(widthMeasureSpec) == MeasureSpec.EXACTLY) {
            MeasureSpec.getSize(widthMeasureSpec)
        } else {
            contentSize + horizontalPadding
        }
        val height = if (MeasureSpec.getMode(heightMeasureSpec) == MeasureSpec.EXACTLY) {
            MeasureSpec.getSize(heightMeasureSpec)
        } else {
            contentSize + verticalPadding
        }
        setMeasuredDimension(width, height)
    }

    override fun onLayout(changed: Boolean, left: Int, top: Int, right: Int, bottom: Int) {
        super.onLayout(changed, left, top, right, bottom)
        updateGeometry()
    }

    /** Recalculates the sign and the text geometry, must be called when any of them changes. */
    protected fun updateGeometry() {
        val contentWidth = (width - paddingLeft - paddingRight).coerceAtLeast(0)
        val contentHeight = (height - paddingTop - paddingBottom).coerceAtLeast(0)
        val size = contentWidth.coerceAtMost(contentHeight).toFloat()

        // Center the square sign inside the padded area.
        val left = paddingLeft + (contentWidth - size) * 0.5f
        val top = paddingTop + (contentHeight - size) * 0.5f

        shapeDrawer.update(left, top, size)
        textDrawer.update(shape, shapeDrawer.bounds, shapeDrawer.innerSize, unitsText)
    }

    private fun availableSize(measureSpec: Int, padding: Int): Int {
        if (MeasureSpec.getMode(measureSpec) == MeasureSpec.UNSPECIFIED) {
            return UNBOUNDED
        }
        return (MeasureSpec.getSize(measureSpec) - padding).coerceAtLeast(0)
    }

    protected companion object {
        const val MIN_VALUE = 0
        const val MAX_VALUE = 999

        /** Not limited by the parent, see [MeasureSpec.UNSPECIFIED]. */
        private const val UNBOUNDED = Int.MAX_VALUE
    }
}
