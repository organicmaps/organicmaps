package app.organicmaps.intent.geo

object TestUriSamples {
    const val NAVIGATION_GOOGLEPLEX = "geo:0,0?q=Googleplex"
    const val NAVIGATION_ADD_STOP =
        "geo:0,0?q=1600+Amphitheatre+parkway&mode=b&intent=add_a_stop"
    const val NAVIGATION_WALKING =
        "geo:0,0?q=coffee+shop&mode=w&intent=navigation"
    const val NAVIGATION_WITH_COORDINATES =
        "geo:1.1,2.2?q=Starbucks+on+Main+Street&mode=w&intent=navigation"
    const val SEARCH_NEARBY = "geo:0,0?q=restaurants+nearby"
    const val ACTION_REPORT_CRASH = "geo.action:?act=report_crash&accident_type=major"
    const val ACTION_MUTE = "geo.action:?act=mute"
    const val ACTION_EXIT_NAVIGATION = "geo.action:?act=exit_navigation"
}
