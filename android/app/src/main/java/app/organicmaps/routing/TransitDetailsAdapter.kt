package app.organicmaps.routing

import android.animation.Animator
import android.animation.AnimatorListenerAdapter
import android.animation.ValueAnimator
import android.content.res.ColorStateList
import android.text.SpannableStringBuilder
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.view.animation.AccelerateDecelerateInterpolator
import android.widget.TextView
import androidx.annotation.DrawableRes
import androidx.annotation.VisibleForTesting
import androidx.core.view.ViewCompat
import androidx.core.view.isVisible
import androidx.core.view.updateLayoutParams
import androidx.core.widget.TextViewCompat
import androidx.recyclerview.widget.RecyclerView
import app.organicmaps.R
import app.organicmaps.sdk.routing.TransitStepInfo
import app.organicmaps.sdk.routing.TransitStepType
import app.organicmaps.util.ThemeUtils
import app.organicmaps.util.Utils
import kotlin.math.roundToInt

/**
 * Renders the per-leg transit route breakdown: one row per leg, showing boarding stop, line badge,
 * stop count + time, and exit stop. The intermediate stops are collapsed until the rider taps the
 * stop-count summary. Walk legs are rendered as a single row.
 *
 * A user-added intermediate point (Add Stop) is rendered as its own numbered badge row between the
 * legs, mirroring the badges on the summary strip so the strip and the expanded details match. It
 * also explains why a continuous walk is split into two rows around the dropped point.
 */
class TransitDetailsAdapter : RecyclerView.Adapter<RecyclerView.ViewHolder>() {
    private val items = mutableListOf<TransitStepInfo>()
    private val expanded = mutableSetOf<Int>()

    // Where the route ends. Null while unknown, and then no closing row is drawn.
    private var destination: String? = null

    /** The place the route ends at, drawn as the timeline's final row. */
    fun setDestination(destination: String?) {
        this.destination = destination
        notifyDataSetChanged()
    }

    private fun hasDestinationRow() = !destination.isNullOrEmpty()

    fun setItems(steps: List<TransitStepInfo>) {
        items.clear()
        expanded.clear()
        // The detail view shows walks, transit rides, and the user's intermediate points (Add Stop).
        // Ruler segments are not meaningful at this level of detail.
        steps.filterTo(items) { it.type != TransitStepType.RULER }
        notifyDataSetChanged()
    }

    override fun getItemViewType(position: Int): Int {
        if (position == items.size) return TYPE_DESTINATION
        return when (items[position].type) {
            TransitStepType.PEDESTRIAN -> TYPE_WALK
            TransitStepType.INTERMEDIATE_POINT -> TYPE_INTERMEDIATE
            else -> TYPE_RIDE
        }
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): RecyclerView.ViewHolder {
        val inflater = LayoutInflater.from(parent.context)
        return when (viewType) {
            TYPE_DESTINATION ->
                DestinationViewHolder(inflater.inflate(R.layout.item_transit_details_destination, parent, false))

            TYPE_WALK -> WalkViewHolder(inflater.inflate(R.layout.item_transit_details_walk, parent, false))

            TYPE_INTERMEDIATE ->
                IntermediateViewHolder(inflater.inflate(R.layout.item_transit_details_intermediate, parent, false))

            else -> createRideViewHolder(inflater, parent)
        }
    }

    private fun createRideViewHolder(inflater: LayoutInflater, parent: ViewGroup): RideViewHolder {
        val holder = RideViewHolder(inflater.inflate(R.layout.item_transit_details_ride, parent, false))
        holder.summaryRow.setOnClickListener {
            val position = holder.bindingAdapterPosition
            if (position == RecyclerView.NO_POSITION) return@setOnClickListener
            val expand = position !in expanded
            if (expand) expanded.add(position) else expanded.remove(position)
            // Animate this row's own height so the whole panel resizes gradually in step, instead of the
            // list snapping shut. A parent-level transition can't tween a nested RecyclerView item collapse.
            holder.setExpanded(expand, true)
        }
        return holder
    }

    override fun onBindViewHolder(holder: RecyclerView.ViewHolder, position: Int) {
        when (holder) {
            is DestinationViewHolder -> holder.bind(destination)

            is WalkViewHolder -> holder.bind(items[position])

            is IntermediateViewHolder -> holder.bind(items[position])

            is RideViewHolder -> {
                val info = items[position]
                // A shared transfer stop is drawn once, as this ride's header; the previous ride then omits its exit.
                val isTransfer = isSameStopTransfer(position)
                val nextIsTransfer = isSameStopTransfer(position + 1)
                // The incoming line's color paints the top half of the transfer stub above this leg's bar.
                val incomingColor = if (isTransfer) items[position - 1].color else info.color
                holder.bind(info, isTransfer, nextIsTransfer, expanded.contains(position), incomingColor)
            }
        }
    }

    // Rides joined by a transfer edge may use differently named stops; those keep both the exit and the boarding stop.
    @VisibleForTesting
    fun isSameStopTransfer(position: Int): Boolean = isRide(position - 1) && isRide(position) &&
        items[position - 1].endStopName == items[position].startStopName

    // A transit ride is any leg that is neither a walk nor a user-added intermediate point.
    @VisibleForTesting
    fun isRide(position: Int): Boolean {
        val type = items.getOrNull(position)?.type ?: return false
        return type != TransitStepType.PEDESTRIAN && type != TransitStepType.INTERMEDIATE_POINT
    }

    override fun getItemCount() = items.size + if (hasDestinationRow()) 1 else 0

    private class DestinationViewHolder(itemView: View) : RecyclerView.ViewHolder(itemView) {
        private val name: TextView = itemView.findViewById(R.id.destination_name)

        fun bind(destination: String?) {
            name.text = destination
        }
    }

    private class WalkViewHolder(itemView: View) : RecyclerView.ViewHolder(itemView) {
        private val text: TextView = itemView.findViewById(R.id.walk_text)
        private val subtitle: TextView = itemView.findViewById(R.id.walk_subtitle)

        fun bind(info: TransitStepInfo) {
            // "Walk" label on top; the duration (and distance if known) in secondary grey underneath, mirroring
            // the transit legs.
            val ctx = itemView.context
            text.text = ctx.getString(R.string.transit_walk_label)
            // Distance first, then duration, so the duration is last as on the transit legs ("N stops · M min").
            val time = Utils.formatRoutingTime(ctx, info.timeInSec, R.dimen.text_size_body_3)
            val builder = SpannableStringBuilder()
            if (!info.distance.isNullOrEmpty()) {
                builder.append(info.distance).append(' ').append(info.distanceUnits).append(" · ")
            }
            builder.append(time)
            subtitle.text = builder
        }
    }

    private class IntermediateViewHolder(itemView: View) : RecyclerView.ViewHolder(itemView) {
        private val badge: TransitStepView = itemView.findViewById(R.id.intermediate_badge)
        private val label: TextView = itemView.findViewById(R.id.intermediate_label)

        fun bind(info: TransitStepInfo) {
            // The badge shows the waypoint number (index + 1), matching the summary strip.
            badge.setTransitStepInfo(info)
            label.text = itemView.context.getString(R.string.transit_waypoint_label, info.intermediateIndex + 1)
        }
    }

    private class RideViewHolder(itemView: View) : RecyclerView.ViewHolder(itemView) {
        private val boardAt: TextView = itemView.findViewById(R.id.board_at)
        private val exitAt: TextView = itemView.findViewById(R.id.exit_at)
        private val subtitle: TextView = itemView.findViewById(R.id.ride_subtitle)

        // The disclosed intermediate-stops text (one stop per line); its height is animated open/closed.
        private val intermediates: TextView = itemView.findViewById(R.id.intermediate_stops)
        private val badge: TransitStepView = itemView.findViewById(R.id.line_badge)
        val summaryRow: View = itemView.findViewById(R.id.ride_summary_row)
        private val lineBar: View = itemView.findViewById(R.id.line_bar)
        private val transferHeaderRow: View = itemView.findViewById(R.id.transfer_header_row)
        private val transferHeader: TextView = itemView.findViewById(R.id.transfer_header)
        private val transferStubTop: View = itemView.findViewById(R.id.transfer_stub_top)
        private val transferStubBottom: View = itemView.findViewById(R.id.transfer_stub_bottom)
        private val transferGap: View = itemView.findViewById(R.id.transfer_gap)
        private var hasIntermediates = false
        private var heightAnimator: ValueAnimator? = null

        fun bind(
            info: TransitStepInfo,
            isTransfer: Boolean,
            nextIsTransfer: Boolean,
            expanded: Boolean,
            incomingColor: Int,
        ) {
            val res = itemView.resources
            // The stops below belong to the chosen line only, unlike the summary's parallel-line numbers.
            badge.setTransitStepInfo(info, info.lineNumber)
            val lineColor = info.color
            // Each colored segment is a rounded pill. The bar's top is rounded unless it continues up out of a
            // transfer stub (isTransfer); its bottom is rounded unless it continues down into the next leg's
            // stub (nextIsTransfer). Where a stub and the bar share a color they meet square, so each run reads as
            // one pill rounded only at its two ends.
            lineBar.setBackgroundResource(lineBarDrawable(!isTransfer, !nextIsTransfer))
            ViewCompat.setBackgroundTintList(lineBar, ColorStateList.valueOf(lineColor))

            // At a same-station transfer the shared stop is drawn once, centered beside a small break in the
            // colored line (incoming color above, outgoing below). The in-bar boarding heading is then hidden
            // and the bar joins straight onto the stub below the break.
            val startStopText = info.startStopName.orEmpty()
            transferHeaderRow.isVisible = isTransfer
            boardAt.isVisible = !isTransfer
            if (isTransfer) {
                // Just the stop name: the two-color line break beside it already says "transfer here".
                transferHeader.text = startStopText
                // The stub is the pill end at the break: incoming run rounded at its foot, outgoing at its head.
                transferStubTop.setBackgroundResource(R.drawable.transit_line_bar_bottom)
                transferStubBottom.setBackgroundResource(R.drawable.transit_line_bar_top)
                ViewCompat.setBackgroundTintList(transferStubTop, ColorStateList.valueOf(incomingColor))
                ViewCompat.setBackgroundTintList(transferStubBottom, ColorStateList.valueOf(lineColor))
                transferGap.updateLayoutParams { height = (0.3f * transferHeader.lineHeight).roundToInt() }
            } else {
                boardAt.text = startStopText
            }

            // On a transfer the bar continues straight from the stub, so drop its usual top offset; the badge
            // row likewise drops its top margin because the transfer header above it already spaces the rhythm.
            lineBar.updateLayoutParams<ViewGroup.MarginLayoutParams> {
                topMargin = if (isTransfer) 0 else res.getDimensionPixelSize(R.dimen.margin_half)
            }
            summaryRow.updateLayoutParams<ViewGroup.MarginLayoutParams> {
                topMargin = if (isTransfer) 0 else res.getDimensionPixelSize(R.dimen.transit_details_badge_top_margin)
            }

            // At a same-stop transfer the next ride's header already names this exit.
            exitAt.isVisible = !nextIsTransfer
            if (!nextIsTransfer) exitAt.text = info.endStopName.orEmpty()

            val time = Utils.formatRoutingTime(itemView.context, info.timeInSec, R.dimen.text_size_body_3)
            val stopCount = info.stopCount
            val builder = SpannableStringBuilder()
            if (stopCount > 0) {
                builder.append(res.getQuantityString(R.plurals.transit_stops_count, stopCount, stopCount))
                builder.append(" · ")
            }
            builder.append(time)
            subtitle.text = builder

            val stops = info.intermediateStopNames
            hasIntermediates = !stops.isNullOrEmpty()
            // Only a leg with intermediate stops is an expandable disclosure; others get no chevron or ripple.
            summaryRow.isClickable = hasIntermediates
            // Text is always set (even while hidden) so the collapse/expand animation can measure it. One stop
            // per line, read as a column between board and exit.
            if (!stops.isNullOrEmpty()) intermediates.text = stops.joinToString("\n")
            setExpanded(expanded, false)
        }

        // Reflects the expanded state on the chevron and the intermediate stops. When animate is true the
        // stops slide open/closed by tweening their own height, which drives the whole panel to resize with
        // them; otherwise (a fresh bind) the state is applied instantly.
        fun setExpanded(expanded: Boolean, animate: Boolean) {
            val chevron = when {
                !hasIntermediates -> 0
                expanded -> R.drawable.ic_expand_less
                else -> R.drawable.ic_expand_more
            }
            subtitle.setCompoundDrawablesRelativeWithIntrinsicBounds(chevron, 0, 0, 0)
            TextViewCompat.setCompoundDrawableTintList(
                subtitle,
                ColorStateList.valueOf(ThemeUtils.getColor(itemView.context, R.attr.secondary)),
            )

            heightAnimator?.cancel()
            heightAnimator = null

            if (!hasIntermediates || !animate) {
                intermediates.isVisible = hasIntermediates && expanded
                setHeight(ViewGroup.LayoutParams.WRAP_CONTENT)
                return
            }

            intermediates.isVisible = true
            val from: Int
            val to: Int
            if (expanded) {
                val width = (intermediates.parent as View).width
                intermediates.measure(
                    View.MeasureSpec.makeMeasureSpec(width, View.MeasureSpec.EXACTLY),
                    View.MeasureSpec.makeMeasureSpec(0, View.MeasureSpec.UNSPECIFIED),
                )
                from = 0
                to = intermediates.measuredHeight
            } else {
                from = intermediates.height
                to = 0
            }

            val animator = ValueAnimator.ofInt(from, to)
            animator.duration = EXPAND_DURATION_MS
            animator.interpolator = AccelerateDecelerateInterpolator()
            animator.addUpdateListener { setHeight(it.animatedValue as Int) }
            animator.addListener(object : AnimatorListenerAdapter() {
                override fun onAnimationEnd(animation: Animator) {
                    if (!expanded) intermediates.isVisible = false
                    setHeight(ViewGroup.LayoutParams.WRAP_CONTENT)
                    heightAnimator = null
                }
            })
            heightAnimator = animator
            animator.start()
        }

        private fun setHeight(height: Int) {
            intermediates.updateLayoutParams { this.height = height }
        }
    }

    companion object {
        @VisibleForTesting
        const val TYPE_WALK = 0

        @VisibleForTesting
        const val TYPE_RIDE = 1

        @VisibleForTesting
        const val TYPE_INTERMEDIATE = 2

        // The route's destination, appended after the legs so the timeline ends somewhere rather than on a walk.
        @VisibleForTesting
        const val TYPE_DESTINATION = 3

        private const val EXPAND_DURATION_MS = 150L

        @DrawableRes
        private fun lineBarDrawable(roundTop: Boolean, roundBottom: Boolean) = when {
            roundTop && roundBottom -> R.drawable.transit_line_bar_round
            roundTop -> R.drawable.transit_line_bar_top
            roundBottom -> R.drawable.transit_line_bar_bottom
            else -> R.drawable.transit_line_bar
        }
    }
}
