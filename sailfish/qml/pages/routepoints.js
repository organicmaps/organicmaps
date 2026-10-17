.import app.organicmaps 1.0 as OM

// Titles for picking a route point of a Routing.PointType, which may replace a point. Not a library: it reads
// appInfo.
function pickTitle(type, replace) {
    switch (type) {
    case OM.Routing.Start: return appInfo.localized(replace ? "change_start_location" : "choose_start_location")
    case OM.Routing.Finish: return appInfo.localized(replace ? "change_destination" : "choose_destination")
    default: return appInfo.localized(replace ? "change_stop_along_route" : "placepage_add_stop")
    }
}

// The place page action that picks the place.
function pickActionTitle(type, replace) {
    switch (type) {
    case OM.Routing.Start: return appInfo.localized(replace ? "change_start_location" : "p2p_from_here")
    case OM.Routing.Finish: return appInfo.localized(replace ? "change_destination" : "p2p_to_here")
    default: return appInfo.localized(replace ? "placepage_replace_stop" : "placepage_add_stop")
    }
}
