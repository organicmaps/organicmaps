package app.organicmaps.widget.colorpicker

import android.content.Context
import android.graphics.Color
import android.graphics.drawable.GradientDrawable
import android.text.Editable
import android.text.InputFilter
import android.util.AttributeSet
import android.util.TypedValue
import android.view.Gravity
import android.view.View
import android.widget.EditText
import android.widget.LinearLayout
import android.widget.TextView
import androidx.annotation.ColorInt
import androidx.annotation.StringRes
import androidx.core.widget.doAfterTextChanged
import app.organicmaps.R
import app.organicmaps.util.ThemeUtils

class ColorSlidersView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0,
) : LinearLayout(context, attrs, defStyleAttr) {

    fun interface OnColorChangedListener {
        fun onColorChanged(@ColorInt color: Int)
    }

    var onColorChangedListener: OnColorChangedListener? = null

    private val redSlider: RoundSlider
    private val greenSlider: RoundSlider
    private val blueSlider: RoundSlider
    private val redValue: TextView
    private val greenValue: TextView
    private val blueValue: TextView
    private val hexInput: EditText
    private val hexRow: View
    private val sliderGroups = mutableListOf<LinearLayout>()

    private var red = 255
    private var green = 0
    private var blue = 0
    private var updatingFromCode = false
    private var isEditingHex = false

    private val marginBase = resources.getDimensionPixelSize(R.dimen.margin_base)
    private val marginHalf = resources.getDimensionPixelSize(R.dimen.margin_half)

    init {
        orientation = VERTICAL
        clipChildren = false
        clipToPadding = false

        val (rs, rv) = addSliderRow(Channel.RED)
        redSlider = rs
        redValue = rv
        val (gs, gv) = addSliderRow(Channel.GREEN)
        greenSlider = gs
        greenValue = gv
        val (bs, bv) = addSliderRow(Channel.BLUE)
        blueSlider = bs
        blueValue = bv
        val (hi, hr) = addHexRow()
        hexInput = hi
        hexRow = hr

        val sliderListener = RoundSlider.OnValueChangedListener { slider, value ->
            when (slider) {
                redSlider -> red = value
                greenSlider -> green = value
                blueSlider -> blue = value
            }
            updateUI(updateSliders = false)
            onColorChangedListener?.onColorChanged(getColor())
        }
        redSlider.onValueChangedListener = sliderListener
        greenSlider.onValueChangedListener = sliderListener
        blueSlider.onValueChangedListener = sliderListener
    }

    private fun isUserAdjusting() =
        redSlider.isTracking || greenSlider.isTracking || blueSlider.isTracking || isEditingHex

    fun setColor(@ColorInt color: Int) {
        if (isUserAdjusting()) return
        val r = Color.red(color)
        val g = Color.green(color)
        val b = Color.blue(color)
        if (r == red && g == green && b == blue) return
        red = r
        green = g
        blue = b
        updateUI(updateSliders = true)
    }

    @ColorInt
    fun getColor(): Int = Color.rgb(red, green, blue)

    fun setCompact(compact: Boolean) {
        sliderGroups.forEachIndexed { i, group ->
            group.layoutParams = if (compact) {
                LayoutParams(LayoutParams.MATCH_PARENT, 0, 1f)
            } else {
                LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT).apply {
                    if (i > 0) topMargin = marginBase
                }
            }
        }
        (hexRow.layoutParams as? MarginLayoutParams)?.topMargin =
            if (compact) marginHalf else resources.getDimensionPixelSize(R.dimen.color_picker_hex_row_margin_top)
        requestLayout()
    }

    private fun updateUI(updateSliders: Boolean) {
        updatingFromCode = true
        if (updateSliders) {
            redSlider.value = red
            greenSlider.value = green
            blueSlider.value = blue
        }
        val currentColor = getColor()
        redSlider.thumbColor = currentColor
        greenSlider.thumbColor = currentColor
        blueSlider.thumbColor = currentColor
        redSlider.setGradient(Color.rgb(0, green, blue), Color.rgb(255, green, blue))
        greenSlider.setGradient(Color.rgb(red, 0, blue), Color.rgb(red, 255, blue))
        blueSlider.setGradient(Color.rgb(red, green, 0), Color.rgb(red, green, 255))
        redValue.text = red.toString()
        greenValue.text = green.toString()
        blueValue.text = blue.toString()
        if (!isEditingHex) {
            hexInput.setText("%02X%02X%02X".format(red, green, blue))
        }
        updatingFromCode = false
    }

    private fun addSliderRow(channel: Channel): Pair<RoundSlider, TextView> {
        val group = LinearLayout(context).apply {
            orientation = VERTICAL
            gravity = Gravity.CENTER_VERTICAL
            clipChildren = false
            clipToPadding = false
        }

        val labelView = TextView(context).apply {
            text = context.getString(channel.labelRes)
            setTextColor(ThemeUtils.getColor(context, android.R.attr.textColorSecondary))
            setTextSize(TypedValue.COMPLEX_UNIT_SP, 12f)
        }
        group.addView(labelView, LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT))

        val row = LinearLayout(context).apply {
            orientation = HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
            clipChildren = false
            clipToPadding = false
        }

        val slider = RoundSlider(context)
        val thumbDiameter = resources.getDimensionPixelSize(R.dimen.color_preset_circle_size)
        val stroke = resources.getDimensionPixelSize(R.dimen.color_picker_selector_stroke)
        val sliderHeight = thumbDiameter + stroke * 2 + resources.getDimensionPixelSize(R.dimen.margin_quarter)
        row.addView(slider, LayoutParams(0, sliderHeight, 1f))

        val valueText = TextView(context).apply {
            setTextSize(TypedValue.COMPLEX_UNIT_SP, 18f)
            setTextColor(ThemeUtils.getColor(context, android.R.attr.textColorPrimary))
            gravity = Gravity.CENTER
            minWidth = resources.getDimensionPixelSize(R.dimen.color_picker_slider_value_min_width)
            background = GradientDrawable().apply {
                setColor(ThemeUtils.getColor(context, R.attr.windowBackgroundForced))
                cornerRadius = resources.getDimension(R.dimen.corner_radius_small)
            }
            val pad = resources.getDimensionPixelSize(R.dimen.margin_quarter_plus)
            setPadding(pad, pad, pad, pad)
        }
        row.addView(
            valueText,
            LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT).apply {
                marginStart = marginHalf
            },
        )

        group.addView(
            row,
            LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT).apply {
                topMargin = resources.getDimensionPixelSize(R.dimen.color_picker_selector_stroke)
            },
        )

        val isFirst = sliderGroups.isEmpty()
        addView(
            group,
            LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT).apply {
                if (!isFirst) topMargin = marginBase
            },
        )
        sliderGroups.add(group)

        return slider to valueText
    }

    private fun addHexRow(): Pair<EditText, View> {
        val row = LinearLayout(context).apply {
            orientation = HORIZONTAL
            gravity = Gravity.END or Gravity.CENTER_VERTICAL
        }

        val label = TextView(context).apply {
            text = context.getString(R.string.color_picker_hex_label)
            setTextColor(ThemeUtils.getColor(context, android.R.attr.colorAccent))
            setTextSize(TypedValue.COMPLEX_UNIT_SP, 16f)
        }
        row.addView(label)

        val input = EditText(context).apply {
            setTextSize(TypedValue.COMPLEX_UNIT_SP, 18f)
            setTextColor(ThemeUtils.getColor(context, android.R.attr.textColorPrimary))
            filters = arrayOf(
                InputFilter.LengthFilter(6),
                InputFilter.AllCaps(),
                InputFilter { source, start, end, _, _, _ ->
                    val filtered = source.subSequence(start, end).filter { it in "0123456789ABCDEFabcdef" }
                    if (filtered.length == end - start) null else filtered
                },
            )
            isSingleLine = true
            gravity = Gravity.CENTER
            minWidth = resources.getDimensionPixelSize(R.dimen.color_picker_hex_input_min_width)
            background = GradientDrawable().apply {
                setColor(ThemeUtils.getColor(context, R.attr.windowBackgroundForced))
                cornerRadius = resources.getDimension(R.dimen.corner_radius_small)
            }
            val pad = marginHalf
            setPadding(pad, pad, pad, pad)
        }
        row.addView(
            input,
            LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT).apply {
                marginStart = marginHalf
            },
        )

        addView(
            row,
            LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT).apply {
                topMargin = resources.getDimensionPixelSize(R.dimen.color_picker_hex_row_margin_top)
            },
        )
        input.doAfterTextChanged(::applyHexInput)

        return input to row
    }

    private fun applyHexInput(editable: Editable?) {
        if (updatingFromCode) return
        val parsed = editable?.toString()?.takeIf { it.length == 6 }?.toLongOrNull(16) ?: return
        isEditingHex = true
        try {
            red = ((parsed shr 16) and 0xFF).toInt()
            green = ((parsed shr 8) and 0xFF).toInt()
            blue = (parsed and 0xFF).toInt()
            updateUI(updateSliders = true)
            onColorChangedListener?.onColorChanged(getColor())
        } finally {
            isEditingHex = false
        }
    }

    private enum class Channel(@StringRes val labelRes: Int) {
        RED(R.string.color_picker_red),
        GREEN(R.string.color_picker_green),
        BLUE(R.string.color_picker_blue),
    }
}
