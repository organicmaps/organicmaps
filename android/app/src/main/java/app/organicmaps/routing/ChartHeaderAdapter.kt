package app.organicmaps.routing

import android.view.View
import android.view.ViewGroup
import android.widget.FrameLayout
import androidx.recyclerview.widget.RecyclerView

class ChartHeaderAdapter(private val headerView: View) : RecyclerView.Adapter<ChartHeaderAdapter.HeaderViewHolder>() {
    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): HeaderViewHolder {
        val host = FrameLayout(parent.context).apply {
            layoutParams = RecyclerView.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT,
            ).apply {
                marginStart = -parent.paddingStart
                marginEnd = -parent.paddingEnd
            }
        }
        (headerView.parent as? ViewGroup)?.removeView(headerView)
        host.addView(headerView)
        return HeaderViewHolder(host)
    }

    override fun onBindViewHolder(holder: HeaderViewHolder, position: Int) {
        // Intentionally empty: the chart panel is permanently attached to the host in onCreateViewHolder.
    }

    override fun getItemCount(): Int = 1

    class HeaderViewHolder(itemView: View) : RecyclerView.ViewHolder(itemView) {
        init {
            setIsRecyclable(false)
        }
    }
}
