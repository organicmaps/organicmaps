package app.organicmaps.routing

import android.content.Context
import android.util.AttributeSet
import android.view.MotionEvent
import android.widget.LinearLayout

/** Takes all touches while clickable, so a tap on the leg strip is a tap on the tile. */
class TransitSummaryTile(context: Context, attrs: AttributeSet?) : LinearLayout(context, attrs) {
    override fun onInterceptTouchEvent(ev: MotionEvent) = isClickable
}
