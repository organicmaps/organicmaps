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

    /** Speed limit to display, must be in [[MIN_VALUE], [MAX_VALUE]]. */
    var speedLimit: Int = valueText.toInt()
        set(value) {
            require(value in MIN_VALUE..MAX_VALUE) {
                "Speed limit must be in [$MIN_VALUE, $MAX_VALUE], but was $value"
            }
            if (field == value) {
                return
            }
            field = value
            valueText = value.toString()
            invalidate()
        }
}
