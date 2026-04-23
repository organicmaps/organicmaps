package app.organicmaps.widget

import android.content.Context
import android.util.AttributeSet
import android.view.View
import androidx.core.content.res.use
import androidx.core.view.isInvisible
import androidx.core.view.isVisible

class BoundedSpace @JvmOverloads constructor(context: Context, attrs: AttributeSet? = null, defStyleAttr: Int = 0) :
    View(context, attrs, defStyleAttr) {

    private val maxWidthPx: Int =
        context.obtainStyledAttributes(attrs, intArrayOf(android.R.attr.maxWidth)).use {
            it.getDimensionPixelSize(0, Int.MAX_VALUE)
        }

    init {
        setWillNotDraw(true)
        if (isVisible) isInvisible = true
    }

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        val mode = MeasureSpec.getMode(widthMeasureSpec)
        val size = MeasureSpec.getSize(widthMeasureSpec)
        val capped = if (mode != MeasureSpec.UNSPECIFIED && size > maxWidthPx) {
            MeasureSpec.makeMeasureSpec(maxWidthPx, MeasureSpec.EXACTLY)
        } else {
            widthMeasureSpec
        }
        super.onMeasure(capped, heightMeasureSpec)
    }
}
