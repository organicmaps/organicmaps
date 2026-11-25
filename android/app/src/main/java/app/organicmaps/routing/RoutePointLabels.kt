@file:JvmName("RoutePointLabels")

package app.organicmaps.routing

import androidx.annotation.StringRes
import app.organicmaps.R
import app.organicmaps.sdk.routing.FollowedTrackInfo
import app.organicmaps.sdk.routing.RouteMarkType

@StringRes
fun pickTitle(pointType: RouteMarkType, isReplace: Boolean): Int = when (pointType) {
    RouteMarkType.Start -> if (isReplace) R.string.change_start_location else R.string.choose_start_location
    RouteMarkType.Intermediate -> if (isReplace) R.string.change_stop_along_route else R.string.route_add_destination
    RouteMarkType.Finish -> if (isReplace) R.string.change_destination else R.string.choose_destination
}

@StringRes
fun trackDestination(track: FollowedTrackInfo): Int = if (track.endsAtGap()) {
    if (track.isReversed) R.string.navigate_track_section_start else R.string.navigate_track_section_finish
} else {
    if (track.isReversed) R.string.navigate_track_start else R.string.navigate_track_finish
}
