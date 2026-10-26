package app.organicmaps.routing

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.core.view.ViewCompat
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import app.organicmaps.R
import app.organicmaps.base.BaseMwmToolbarFragment
import app.organicmaps.sdk.Framework
import app.organicmaps.sdk.R as SdkR
import app.organicmaps.sdk.routing.RouteMarkType
import app.organicmaps.sdk.routing.RoutingController
import app.organicmaps.util.WindowInsetUtils.BaselinePaddingInsetsListener
import app.organicmaps.widget.ToolbarController

/**
 * Full-screen per-leg transit route breakdown, reached by tapping the transit summary tile. The route
 * steps are re-read from the cached transit info rather than passed in, so no marshaling is needed.
 */
class TransitDetailsFragment : BaseMwmToolbarFragment() {
    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View {
        val root = inflater.inflate(R.layout.fragment_transit_details, container, false)

        val adapter = TransitDetailsAdapter()
        val recycler = root.findViewById<RecyclerView>(R.id.transit_details_recycler)
        recycler.layoutManager = LinearLayoutManager(requireContext())
        // A ride row animates its own intermediate-stops height when toggled; the default change animation
        // would fight that, so disable it.
        recycler.itemAnimator = null
        recycler.adapter = adapter
        // Pad the scrolling list past the nav bar (bottom) and the display cutout in landscape (left/right);
        // the toolbar above already consumes the top inset. Ctor order is (top, bottom, left, right).
        ViewCompat.setOnApplyWindowInsetsListener(recycler, BaselinePaddingInsetsListener(false, true, true, true))

        val info = RoutingController.get().cachedTransitInfo
        if (info == null) {
            // The route was cancelled/rebuilt out from under us (e.g. process restore). Nothing to show.
            requireActivity().finish()
            return root
        }
        adapter.setDestination(destinationName())
        adapter.setWaypointNames(waypointNames())
        adapter.setItems(info.transitSteps)

        return root
    }

    // The user's added stops by intermediate index, titled as the route points list titles them.
    private fun waypointNames(): Map<Int, String> {
        val names = mutableMapOf<Int, String>()
        for (point in Framework.nativeGetRoutePoints()) {
            if (point.mPointType != RouteMarkType.Intermediate) continue
            val title = if (point.mIsMyPosition) getString(SdkR.string.core_my_position) else point.mTitle
            if (!title.isNullOrEmpty()) names[point.mIntermediateIndex] = title
        }
        return names
    }

    /**
     * The route's end point, named as the routing panel names it. Null when the route has been torn down
     * or the point has no title, in which case the timeline simply ends on its last leg.
     */
    private fun destinationName(): String? = RoutingController.get().endPoint?.title?.ifEmpty { null }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        toolbarController.setTitle(R.string.transit_details_title)
    }

    // The map activity is always directly below, so the up arrow just finishes this screen (a clean
    // slide-back pop) instead of relaunching the parent via the default up-navigation.
    override fun onCreateToolbarController(root: View): ToolbarController =
        object : ToolbarController(root, requireActivity()) {
            override fun onUpClick() {
                requireActivity().finish()
            }
        }
}
