package app.organicmaps.sdk.widgets.speedlimit

import android.content.Context
import android.util.AttributeSet
import androidx.annotation.AttrRes
import androidx.annotation.StyleRes

/**
 * A current speed sign: a value with the units below it, always drawn inside a rounded square.
 * Shape and corner-radius style attributes are ignored.
 *
 * Colors are taken from the palette of the current [shape] and [mode], the units default to
 * `om_widget_units` of the style, see [R.styleable.OmWidgetCurrentSpeedView].
 */
internal class CurrentSpeedView
@JvmOverloads
constructor(
    context: Context,
    attrs: AttributeSet? = null,
    @AttrRes defStyleAttr: Int = R.attr.om_widget_current_speed_view_style,
    @StyleRes defStyleRes: Int = R.style.OmWidget_CurrentSpeedView_Combined,
) : SignView(context, attrs, defStyleAttr, defStyleRes, fixedShape = Shape.Square) {
    /** Current speed; values outside [[MIN_VALUE], [MAX_VALUE]] are displayed as unavailable (–). */
    var speed: Int = valueText.toInt()
        set(value) {
            if (field == value) {
                return
            }
            field = value
            valueText = formatValue(value)
            invalidate()
        }

    /** Localized speed units, e.g. "km/h". The text size is fitted to them, see [SignTextDrawer]. */
    var units: String = ""
        set(value) {
            if (field == value) {
                return
            }
            field = value
            // Both rows are fitted together, so the value size depends on the units as well.
            updateGeometry()
            invalidate()
        }

    override val unitsText: String get() = units

    init {
        if (isInEditMode) {
            speed = 60
        }
        val attributes = context.theme.obtainStyledAttributes(
            attrs,
            R.styleable.OmWidgetCurrentSpeedView,
            defStyleAttr,
            defStyleRes,
        )
        try {
            units = attributes.getString(R.styleable.OmWidgetCurrentSpeedView_om_widget_units).orEmpty()
        } finally {
            attributes.recycle()
        }
    }
}
