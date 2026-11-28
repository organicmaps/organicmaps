package app.organicmaps.widget.colorpicker

import android.content.Context
import android.graphics.Color
import androidx.annotation.ColorInt
import app.organicmaps.R

internal fun Context.describeColor(@ColorInt color: Int): String = getString(
    R.string.color_picker_rgb_value,
    Color.red(color),
    Color.green(color),
    Color.blue(color),
)

internal fun Context.describeColor(@ColorInt color: Int, index: Int): String =
    getString(R.string.color_picker_color_description, index + 1, describeColor(color))
