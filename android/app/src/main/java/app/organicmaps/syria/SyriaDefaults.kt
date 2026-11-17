package app.organicmaps.syria

import android.content.Context
import app.organicmaps.sdk.routing.RoutingOptions

/** Defaults of the Syria edition that differ from Organic Maps. Applied once, so user changes stick. */
object SyriaDefaults {
    /** Needs the native core: call once it is initialized. */
    @JvmStatic
    fun applyOnce(context: Context) {
        if (SyriaPrefs.isRouteOptimizationDefaultApplied(context)) return
        RoutingOptions.setRouteOptimizationEnabled(true)
        SyriaPrefs.markRouteOptimizationDefaultApplied(context)
    }
}
