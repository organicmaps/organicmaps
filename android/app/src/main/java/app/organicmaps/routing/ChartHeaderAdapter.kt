package app.organicmaps.routing

import android.view.View
import android.view.ViewGroup
import android.widget.FrameLayout
import androidx.recyclerview.widget.RecyclerView
import app.organicmaps.R
import app.organicmaps.util.MultiTouchDelegate

// Single-slot adapter (getItemCount = 1) that hosts a shared headerView across the adapter's lifetime.
// The same headerView instance is re-parented into a fresh FrameLayout host on each onCreateViewHolder
// (e.g. config change), so detaching from any previous parent before re-attach is required.
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
        val smallInset = host.resources.getDimensionPixelSize(R.dimen.margin_quarter)
        val largeInset = host.resources.getDimensionPixelSize(R.dimen.margin_half_plus)
        // Split the gap between targets; vertical expansion uses the header's surrounding spacing.
        val touchDelegate = MultiTouchDelegate(host)
        // Reverse already has a 48 dp target; keep its bounds while expanding route options.
        touchDelegate.addTarget(headerView.findViewById(R.id.reverse_route_btn), 0, 0, 0, 0)
        touchDelegate.addTarget(
            headerView.findViewById(R.id.driving_options_btn_img),
            smallInset,
            smallInset,
            largeInset,
            largeInset,
        )
        host.touchDelegate = touchDelegate
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
