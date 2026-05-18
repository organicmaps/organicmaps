package app.organicmaps.widget

import android.content.Context
import android.graphics.Color
import android.graphics.Outline
import android.graphics.Rect
import android.graphics.drawable.GradientDrawable
import android.view.Gravity
import android.view.View
import android.view.ViewGroup
import android.view.ViewOutlineProvider
import android.widget.ArrayAdapter
import android.widget.ImageView
import android.widget.LinearLayout
import android.widget.ListView
import android.widget.PopupWindow
import android.widget.TextView
import androidx.annotation.ColorInt
import androidx.core.content.ContextCompat
import androidx.core.graphics.ColorUtils
import androidx.core.widget.TextViewCompat
import app.organicmaps.R
import app.organicmaps.util.ThemeUtils
import java.util.function.IntConsumer

/**
 * Reusable rounded popup with an arrow pointing toward the anchor view.
 * Used for selection lists shown from the Place Page (e.g., public-transit route options,
 * track-candidates selector).
 */
object ArrowPopup {

    data class Item @JvmOverloads constructor(
        val label: CharSequence,
        @ColorInt val color: Int = 0,
        val isSelected: Boolean = false,
    )

    enum class RowStyle {
        /** Tint the entire row with a pastel of the item color (used for public-transit routes). */
        PASTEL_BACKGROUND,

        /** Color dot on the left, label in the middle, checkmark on the right when selected. */
        COLOR_ICON_LEFT,
    }

    /**
     * @param widthAnchor View that defines the popup's horizontal extent (its width minus margins).
     * @param arrowAnchor Optional View whose horizontal center receives the arrow tip.
     *                    Defaults to `null` — center the arrow under [widthAnchor].
     */
    @JvmStatic
    @JvmOverloads
    fun show(
        context: Context,
        widthAnchor: View,
        items: List<Item>,
        rowStyle: RowStyle,
        onSelect: IntConsumer,
        arrowAnchor: View? = null,
    ): PopupWindow? {
        if (items.isEmpty()) {
            return null
        }

        val res = context.resources
        val radius = res.getDimension(R.dimen.corner_radius_medium)
        val arrowWidth = res.getDimension(R.dimen.routes_popup_arrow_width)
        val arrowHeight = res.getDimension(R.dimen.routes_popup_arrow_height)
        val arrowHeightPx = arrowHeight.toInt()
        val margin = res.getDimension(R.dimen.routes_popup_margin).toInt()
        val popupWidth = widthAnchor.width - 2 * margin
        if (popupWidth <= 0) {
            return null
        }

        val directionAnchor = arrowAnchor ?: widthAnchor
        val dirLoc = directionAnchor.locationOnScreen()
        val displayFrame = Rect().also(directionAnchor::getWindowVisibleDisplayFrame)
        val spaceAbove = dirLoc[1] - displayFrame.top
        val spaceBelow = displayFrame.bottom - dirLoc[1] - directionAnchor.height
        val arrowOnTop = spaceBelow > spaceAbove

        val widthLoc = widthAnchor.locationOnScreen()

        val bgDrawable = ArrowPopupBackground(
            bgColor = ThemeUtils.getColor(context, R.attr.cardBackground),
            shadowColor = ContextCompat.getColor(context, R.color.black_12),
            radius = radius,
            arrowWidth = arrowWidth,
            arrowHeight = arrowHeight,
            shadowDy = res.getDimension(R.dimen.routes_popup_shadow_offset),
            arrowOnTop = arrowOnTop,
        )

        val listView = ListView(context).apply {
            divider = null
            dividerHeight = 0
            adapter = buildAdapter(context, items, rowStyle)
            outlineProvider = object : ViewOutlineProvider() {
                override fun getOutline(view: View, outline: Outline) {
                    outline.setRoundRect(0, 0, view.width, view.height, radius)
                }
            }
            clipToOutline = true
        }

        val maxHeight = (if (arrowOnTop) spaceBelow else spaceAbove) - margin
        if (maxHeight <= 0) {
            return null
        }
        listView.measure(
            View.MeasureSpec.makeMeasureSpec(popupWidth, View.MeasureSpec.AT_MOST),
            View.MeasureSpec.makeMeasureSpec(maxHeight - arrowHeightPx, View.MeasureSpec.AT_MOST),
        )
        val popupHeight = listView.measuredHeight + arrowHeightPx
        if (popupHeight <= 0) {
            return null
        }

        val popup = PopupWindow(listView, popupWidth, popupHeight).apply {
            setBackgroundDrawable(bgDrawable)
            elevation = res.getDimension(R.dimen.routes_popup_elevation)
            isFocusable = true
            isOutsideTouchable = true
            animationStyle = android.R.style.Animation_Dialog
        }

        listView.setOnItemClickListener { _, _, position, _ ->
            onSelect.accept(position)
            popup.dismiss()
        }

        val popupX = (widthLoc[0] + margin)
            .coerceIn(displayFrame.left + margin, displayFrame.right - popupWidth - margin)
        val popupY = if (arrowOnTop) {
            dirLoc[1] + directionAnchor.height + margin
        } else {
            dirLoc[1] - popupHeight - margin
        }

        if (arrowAnchor != null) {
            val arrowTipScreenX = dirLoc[0] + arrowAnchor.width / 2
            val minCx = radius + arrowWidth / 2f
            val maxCx = popupWidth - radius - arrowWidth / 2f
            // Keep the arrow inside the rounded body so it never overlaps a corner.
            bgDrawable.arrowCx = (arrowTipScreenX - popupX).toFloat().coerceIn(minCx, maxCx)
        }

        popup.showAtLocation(widthAnchor, Gravity.NO_GRAVITY, popupX, popupY)
        return popup
    }

    private fun buildAdapter(context: Context, items: List<Item>, rowStyle: RowStyle): ArrayAdapter<Item> {
        val res = context.resources
        val padH = res.getDimensionPixelSize(R.dimen.margin_base)
        val padV = res.getDimensionPixelSize(R.dimen.margin_half_plus)
        val defaultTextColor = ThemeUtils.getColor(context, android.R.attr.textColorPrimary)

        return when (rowStyle) {
            RowStyle.PASTEL_BACKGROUND -> pastelAdapter(context, items, padH, padV, defaultTextColor)
            RowStyle.COLOR_ICON_LEFT -> colorIconLeftAdapter(context, items, padH, padV, defaultTextColor)
        }
    }

    private fun pastelAdapter(
        context: Context,
        items: List<Item>,
        padH: Int,
        padV: Int,
        @ColorInt defaultTextColor: Int,
    ): ArrayAdapter<Item> = object : ArrayAdapter<Item>(context, 0, items) {
        override fun getView(position: Int, convertView: View?, parent: ViewGroup): View {
            val tv = (convertView as? TextView) ?: TextView(context).apply {
                setPadding(padH, padV, padH, padV)
                TextViewCompat.setTextAppearance(this, androidx.appcompat.R.style.TextAppearance_AppCompat_Body1)
            }
            val item = getItem(position) ?: return tv
            tv.text = item.label
            if (item.color != 0) {
                tv.setBackgroundColor(ColorUtils.blendARGB(item.color, Color.WHITE, 0.75f))
                // Pastel background stays light in both themes — keep text readable on it.
                tv.setTextColor(Color.BLACK)
            } else {
                tv.setBackgroundColor(Color.TRANSPARENT)
                tv.setTextColor(defaultTextColor)
            }
            return tv
        }
    }

    private fun colorIconLeftAdapter(
        context: Context,
        items: List<Item>,
        padH: Int,
        padV: Int,
        @ColorInt defaultTextColor: Int,
    ): ArrayAdapter<Item> {
        val res = context.resources
        val iconSizePx = res.getDimensionPixelSize(R.dimen.margin_half_double_plus)
        val gapPx = res.getDimensionPixelSize(R.dimen.margin_half_plus)
        val checkTint = ThemeUtils.getColor(context, androidx.appcompat.R.attr.colorAccent)

        return object : ArrayAdapter<Item>(context, 0, items) {
            override fun getView(position: Int, convertView: View?, parent: ViewGroup): View {
                val cached = (convertView as? LinearLayout)?.tag as? RowHolder
                val row: LinearLayout
                val holder: RowHolder
                if (cached != null) {
                    row = convertView
                    holder = cached
                } else {
                    row = LinearLayout(context).apply {
                        orientation = LinearLayout.HORIZONTAL
                        gravity = Gravity.CENTER_VERTICAL
                        setPadding(padH, padV, padH, padV)
                    }
                    val iconDrawable = GradientDrawable().apply { shape = GradientDrawable.OVAL }
                    val icon = ImageView(context).also { iv ->
                        iv.setImageDrawable(iconDrawable)
                        row.addView(iv, LinearLayout.LayoutParams(iconSizePx, iconSizePx).apply { marginEnd = gapPx })
                    }
                    val label = TextView(context).also { tv ->
                        TextViewCompat.setTextAppearance(tv, androidx.appcompat.R.style.TextAppearance_AppCompat_Body1)
                        row.addView(tv, LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f))
                    }
                    val check = ImageView(context).apply {
                        setImageResource(R.drawable.ic_check)
                        setColorFilter(checkTint)
                    }
                    row.addView(
                        check,
                        LinearLayout.LayoutParams(
                            ViewGroup.LayoutParams.WRAP_CONTENT,
                            ViewGroup.LayoutParams.WRAP_CONTENT,
                        ).apply { marginStart = gapPx },
                    )
                    holder = RowHolder(icon, label, check, iconDrawable)
                    row.tag = holder
                }

                val item = getItem(position) ?: return row
                holder.iconDrawable.setColor(if (item.color == 0) Color.TRANSPARENT else item.color)
                holder.label.text = item.label
                holder.label.setTextColor(defaultTextColor)
                holder.check.visibility = if (item.isSelected) View.VISIBLE else View.INVISIBLE
                return row
            }
        }
    }

    private data class RowHolder(
        val icon: ImageView,
        val label: TextView,
        val check: ImageView,
        val iconDrawable: GradientDrawable,
    )

    private fun View.locationOnScreen(): IntArray = IntArray(2).also { getLocationOnScreen(it) }
}
