package app.organicmaps.sdk.widgets.speedlimit

import android.content.Context
import android.graphics.Color
import androidx.annotation.ColorInt
import androidx.annotation.StyleRes

/** Colors of a road sign for a single shape + mode combination. */
internal data class SignColors(
    @param:ColorInt val background: Int,
    @param:ColorInt val outline: Int,
    @param:ColorInt val text: Int,
    @param:ColorInt val frame: Int = Color.WHITE,
) {
    companion object {
        val DEFAULT = SignColors(background = Color.WHITE, outline = Color.RED, text = Color.BLACK)

        /** Reads a color set from a style that applies [R.styleable.OmWidgetSignColors]. */
        fun from(context: Context, @StyleRes styleRes: Int, defaults: SignColors): SignColors {
            if (styleRes == 0) {
                return defaults
            }

            val attrs = context.obtainStyledAttributes(styleRes, R.styleable.OmWidgetSignColors)
            try {
                return SignColors(
                    background = attrs.getColor(
                        R.styleable.OmWidgetSignColors_om_widget_background_color,
                        defaults.background,
                    ),
                    outline = attrs.getColor(R.styleable.OmWidgetSignColors_om_widget_outline_color, defaults.outline),
                    text = attrs.getColor(R.styleable.OmWidgetSignColors_om_widget_text_color, defaults.text),
                    frame = attrs.getColor(R.styleable.OmWidgetSignColors_om_widget_frame_color, defaults.frame),
                )
            } finally {
                attrs.recycle()
            }
        }
    }
}

/** Color sets of a single shape, one per [Mode]. */
internal class SignPalette private constructor(private val colorsByMode: Map<Mode, SignColors>) {
    operator fun get(mode: Mode): SignColors = colorsByMode.getValue(mode)

    companion object {
        /** A palette that uses the same colors in every mode. */
        fun of(colors: SignColors): SignPalette = SignPalette(Mode.entries.associateWith { colors })

        /** Reads a palette from a style that applies [R.styleable.OmWidgetSignPalette]. */
        fun from(context: Context, @StyleRes styleRes: Int, defaults: SignColors): SignPalette {
            if (styleRes == 0) {
                return of(defaults)
            }

            val attrs = context.obtainStyledAttributes(styleRes, R.styleable.OmWidgetSignPalette)
            try {
                return SignPalette(
                    mapOf(
                        Mode.Normal to SignColors.from(
                            context,
                            attrs.getResourceId(R.styleable.OmWidgetSignPalette_om_widget_colors_normal, 0),
                            defaults,
                        ),
                        Mode.Warning to SignColors.from(
                            context,
                            attrs.getResourceId(R.styleable.OmWidgetSignPalette_om_widget_colors_warning, 0),
                            defaults,
                        ),
                        Mode.Alert to SignColors.from(
                            context,
                            attrs.getResourceId(R.styleable.OmWidgetSignPalette_om_widget_colors_alert, 0),
                            defaults,
                        ),
                    ),
                )
            } finally {
                attrs.recycle()
            }
        }
    }
}
