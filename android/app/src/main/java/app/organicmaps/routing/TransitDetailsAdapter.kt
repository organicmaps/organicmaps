package app.organicmaps.routing

import android.animation.Animator
import android.animation.AnimatorListenerAdapter
import android.animation.ValueAnimator
import android.content.res.ColorStateList
import android.graphics.drawable.GradientDrawable
import android.text.SpannableStringBuilder
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.view.animation.AccelerateDecelerateInterpolator
import android.widget.TextView
import androidx.annotation.VisibleForTesting
import androidx.core.view.isVisible
import androidx.core.view.updateLayoutParams
import androidx.core.widget.TextViewCompat
import androidx.recyclerview.widget.RecyclerView
import app.organicmaps.R
import app.organicmaps.sdk.routing.TransitStepInfo
import app.organicmaps.sdk.routing.TransitStepType
import app.organicmaps.sdk.util.StringUtils
import app.organicmaps.util.ThemeUtils
import kotlin.math.roundToInt

/**
 * Renders the per-leg transit route breakdown: one row per leg, showing boarding stop, line badge,
 * stop count + time, and exit stop. The intermediate stops are collapsed until the rider taps the
 * stop-count summary. Walk legs are rendered as a single row.
 *
 * A user-added intermediate point (Add Stop) is rendered as its own numbered badge row between the
 * legs, mirroring the badges on the summary strip so the strip and this screen match. It also
 * explains why a continuous walk is split into two rows around that point.
 *
 * The adapter is a snapshot of one built route: [steps] are the route's walks, rides and added stops.
 */
class TransitDetailsAdapter(
    private val steps: List<TransitStepInfo>,
    // Where the route ends. Null or empty draws no closing row.
    private val destination: String?,
    // Names of the user's added stops by intermediate index; an unnamed stop falls back to its number.
    private val waypointNames: Map<Int, String>,
    // Formats a leg's duration; replaceable so rows can be bound without the native core.
    private val formatDuration: (Int) -> String = StringUtils::nativeFormatRoutingDuration,
) : RecyclerView.Adapter<RecyclerView.ViewHolder>() {
    private val expanded = mutableSetOf<Int>()

    private fun hasDestinationRow() = !destination.isNullOrEmpty()

    override fun getItemViewType(position: Int): Int {
        if (position == steps.size) return TYPE_DESTINATION
        return when (steps[position].type) {
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

            TYPE_WALK -> WalkViewHolder(
                inflater.inflate(R.layout.item_transit_details_walk, parent, false),
                formatDuration,
            )

            TYPE_INTERMEDIATE ->
                IntermediateViewHolder(inflater.inflate(R.layout.item_transit_details_intermediate, parent, false))

            else -> createRideViewHolder(inflater, parent)
        }
    }

    private fun createRideViewHolder(inflater: LayoutInflater, parent: ViewGroup): RideViewHolder {
        val holder = RideViewHolder(inflater.inflate(R.layout.item_transit_details_ride, parent, false), formatDuration)
        holder.summaryRow.setOnClickListener {
            val position = holder.bindingAdapterPosition
            if (position == RecyclerView.NO_POSITION) return@setOnClickListener
            val expand = position !in expanded
            if (expand) expanded.add(position) else expanded.remove(position)
            // Tween this row's height: only it grows or shrinks, and the rows below it follow.
            holder.setExpanded(expand, true)
        }
        return holder
    }

    override fun onBindViewHolder(holder: RecyclerView.ViewHolder, position: Int) {
        when (holder) {
            is DestinationViewHolder -> holder.bind(destination)

            is WalkViewHolder -> holder.bind(steps[position])

            is IntermediateViewHolder -> {
                val info = steps[position]
                holder.bind(info, waypointNames[info.intermediateIndex])
            }

            is RideViewHolder -> {
                val info = steps[position]
                // A shared transfer stop is drawn once, as this ride's header; the previous ride then omits its exit.
                val isTransfer = isSameStopTransfer(position)
                val nextIsTransfer = isSameStopTransfer(position + 1)
                // The incoming line's color paints the top half of the transfer stub above this leg's bar.
                val incomingColor = if (isTransfer) steps[position - 1].color else info.color
                holder.bind(
                    info,
                    isTransfer,
                    nextIsTransfer,
                    expanded.contains(position),
                    incomingColor,
                )
            }
        }
    }

    // Rides joined by a transfer edge may use differently named stops; those keep both the exit and the boarding stop.
    // Unnamed stops can't be matched, so they never join.
    @VisibleForTesting
    fun isSameStopTransfer(position: Int): Boolean {
        if (!isRide(position - 1) || !isRide(position)) return false
        val stop = steps[position].startStopName
        return !stop.isNullOrEmpty() && steps[position - 1].endStopName == stop
    }

    // A transit ride is any leg that is neither a walk nor a user-added intermediate point.
    private fun isRide(position: Int): Boolean {
        val type = steps.getOrNull(position)?.type ?: return false
        return type != TransitStepType.PEDESTRIAN && type != TransitStepType.INTERMEDIATE_POINT
    }

    override fun getItemCount() = steps.size + if (hasDestinationRow()) 1 else 0

    private class DestinationViewHolder(itemView: View) : RecyclerView.ViewHolder(itemView) {
        private val name: TextView = itemView.findViewById(R.id.destination_name)

        fun bind(destination: String?) {
            name.text = destination
        }
    }

    private class WalkViewHolder(itemView: View, private val formatDuration: (Int) -> String) :
        RecyclerView.ViewHolder(itemView) {
        private val subtitle: TextView = itemView.findViewById(R.id.walk_subtitle)

        fun bind(info: TransitStepInfo) {
            // Distance first, then duration, so the duration is last as on the transit legs ("N stops · M min").
            val builder = SpannableStringBuilder()
            if (!info.distance.isNullOrEmpty()) {
                builder.append(info.distance).append(' ').append(info.distanceUnits).append(" · ")
            }
            builder.append(formatDuration(info.timeInSec))
            subtitle.text = builder
        }
    }

    private class IntermediateViewHolder(itemView: View) : RecyclerView.ViewHolder(itemView) {
        private val badge: TransitStepView = itemView.findViewById(R.id.intermediate_badge)
        private val label: TextView = itemView.findViewById(R.id.intermediate_label)

        fun bind(info: TransitStepInfo, name: String?) {
            // The badge shows the waypoint number (index + 1), matching the summary strip.
            badge.setTransitStepInfo(info)
            label.text = name ?: itemView.context.getString(R.string.transit_waypoint_label, info.intermediateIndex + 1)
        }
    }

    private class RideViewHolder(itemView: View, private val formatDuration: (Int) -> String) :
        RecyclerView.ViewHolder(itemView) {
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
        private val barRadius = itemView.resources.getDimension(R.dimen.routing_transit_step_corner_radius)
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
            lineBar.background = lineBar(lineColor, !isTransfer, !nextIsTransfer)

            // At a same-station transfer the shared stop is drawn once, centered beside a small break in the
            // colored line (incoming color above, outgoing below). The in-bar boarding heading is then hidden
            // and the bar joins straight onto the stub below the break.
            val startStop = info.startStopName.orEmpty()
            transferHeaderRow.isVisible = isTransfer
            boardAt.isVisible = !isTransfer && startStop.isNotEmpty()
            if (isTransfer) {
                // Just the stop name: the two-color line break beside it already says "transfer here".
                transferHeader.text = startStop
                // The stub is the pill end at the break: incoming run rounded at its foot, outgoing at its head.
                transferStubTop.background = lineBar(incomingColor, roundTop = false, roundBottom = true)
                transferStubBottom.background = lineBar(lineColor, roundTop = true, roundBottom = false)
                transferGap.updateLayoutParams { height = (0.3f * transferHeader.lineHeight).roundToInt() }
            } else {
                boardAt.text = startStop
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
            val endStop = info.endStopName.orEmpty()
            exitAt.isVisible = !nextIsTransfer && endStop.isNotEmpty()
            exitAt.text = endStop

            val stopCount = info.stopCount
            val builder = SpannableStringBuilder()
            if (stopCount > 0) {
                builder.append(res.getQuantityString(R.plurals.transit_stops_count, stopCount, stopCount))
                builder.append(" · ")
            }
            builder.append(formatDuration(info.timeInSec))
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

        private fun lineBar(color: Int, roundTop: Boolean, roundBottom: Boolean) = GradientDrawable().apply {
            // Colored directly rather than tinted: API 21 doesn't tint shape backgrounds.
            setColor(color)
            val top = if (roundTop) barRadius else 0f
            val bottom = if (roundBottom) barRadius else 0f
            cornerRadii = floatArrayOf(top, top, top, top, bottom, bottom, bottom, bottom)
        }

        // Reflects the expanded state on the chevron and the intermediate stops. When animate is true the
        // stops open/close by tweening their height, pushing the rows below; otherwise (a fresh bind) the
        // state is applied instantly.
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

            // A quick second tap or a rebind of this recycled holder must not let a stale tween resize it.
            // Its end handling is skipped on cancel, so a new tween continues from the current height.
            heightAnimator?.cancel()
            heightAnimator = null

            if (!hasIntermediates || !animate) {
                intermediates.isVisible = hasIntermediates && expanded
                setHeight(ViewGroup.LayoutParams.WRAP_CONTENT)
                return
            }

            // A collapse that was interrupted mid-way leaves its animated height; otherwise this is 0 or wrap.
            val current = if (intermediates.isVisible) intermediates.height else 0
            val from = if (intermediates.layoutParams.height >= 0) intermediates.layoutParams.height else current
            intermediates.isVisible = true
            val to = if (expanded) measuredStopsHeight() else 0

            val animator = ValueAnimator.ofInt(from, to)
            animator.duration = EXPAND_DURATION_MS
            animator.interpolator = AccelerateDecelerateInterpolator()
            animator.addUpdateListener { setHeight(it.animatedValue as Int) }
            animator.addListener(object : AnimatorListenerAdapter() {
                private var cancelled = false

                override fun onAnimationCancel(animation: Animator) {
                    cancelled = true
                }

                override fun onAnimationEnd(animation: Animator) {
                    if (cancelled) return
                    if (!expanded) intermediates.isVisible = false
                    setHeight(ViewGroup.LayoutParams.WRAP_CONTENT)
                    heightAnimator = null
                }
            })
            heightAnimator = animator
            animator.start()
        }

        private fun measuredStopsHeight(): Int {
            val width = (intermediates.parent as View).width
            intermediates.measure(
                View.MeasureSpec.makeMeasureSpec(width, View.MeasureSpec.EXACTLY),
                View.MeasureSpec.makeMeasureSpec(0, View.MeasureSpec.UNSPECIFIED),
            )
            return intermediates.measuredHeight
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
    }
}
