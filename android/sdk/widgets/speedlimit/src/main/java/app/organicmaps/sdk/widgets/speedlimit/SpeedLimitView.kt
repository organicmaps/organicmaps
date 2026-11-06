package app.organicmaps.sdk.widgets.speedlimit

import android.content.Context
import android.util.AttributeSet
import androidx.annotation.AttrRes
import androidx.annotation.StyleRes

/**
 * A speed limit road sign, by default a value inside a white shape with a red outline.
 *
 * Colors are taken from the palette of the current [shape] and [mode], see
 * [R.styleable.OmWidgetSignView]. Restyle all instances via the `om_widget_speed_limit_view_style`
 * theme attribute, or a single one via `style` in the layout.
 */
class SpeedLimitView
@JvmOverloads
constructor(
    context: Context,
    attrs: AttributeSet? = null,
    @AttrRes defStyleAttr: Int = R.attr.om_widget_speed_limit_view_style,
    @StyleRes defStyleRes: Int = R.style.OmWidget_SpeedLimitView,
) : SignView(context, attrs, defStyleAttr, defStyleRes) {
    /** Shape of the speed-limit sign, configurable independently of the current speed. */
    override var shape: Shape
        get() = super.shape
        public set(value) {
            super.shape = value
        }

    /** Positive speed limit up to [MAX_VALUE], or null when unavailable (–). */
    var speedLimit: Int? = null
        set(value) {
            require(value == null || value in 1..MAX_VALUE) { "Speed limit must be between 1 and $MAX_VALUE" }
            if (field == value) {
                return
            }
            field = value
            valueText = value?.let(::formatValue) ?: "–"
            invalidate()
        }

    init {
        valueText = "–"
        if (isInEditMode) {
            speedLimit = 90
        }
    }
}
