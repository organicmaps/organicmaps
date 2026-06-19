package app.organicmaps.widget

import android.content.Context
import android.graphics.Canvas
import android.util.AttributeSet
import android.view.Gravity
import androidx.appcompat.widget.AppCompatRadioButton
import androidx.core.content.ContextCompat
import androidx.core.widget.CompoundButtonCompat
import app.organicmaps.R

class RoutingToolbarButton @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0,
) : AppCompatRadioButton(context, attrs, defStyleAttr) {
    init {
        setBackgroundResource(R.drawable.routing_toolbar_button)
        buttonTintList = ContextCompat.getColorStateList(context, R.color.routing_toolbar_icon_tint)
        setPadding(0, 0, 0, 0)
        compoundDrawablePadding = 0
        gravity = Gravity.CENTER
    }

    // RadioButton draws the button drawable at the start edge; on a stretched (weight=1) cell
    // inside the routing pill, translate horizontally so the icon stays centered.
    override fun onDraw(canvas: Canvas) {
        val drawable = CompoundButtonCompat.getButtonDrawable(this) ?: return super.onDraw(canvas)
        val shift = (width - drawable.intrinsicWidth) / 2
        if (shift <= 0) return super.onDraw(canvas)
        val dx = if (layoutDirection == LAYOUT_DIRECTION_RTL) -shift else shift
        canvas.save()
        canvas.translate(dx.toFloat(), 0f)
        super.onDraw(canvas)
        canvas.restore()
    }
}
