package app.organicmaps.syria

import android.content.Context
import app.organicmaps.sdk.routing.RoutingOptions
import app.organicmaps.sdk.util.Config

/**
 * Defaults of the Syria edition that differ from Organic Maps. Each is applied once (with its own flag,
 * so an update also brings defaults added later); a user change in the settings sticks afterwards.
 */
object SyriaDefaults {
    /** Needs the native core: call once it is initialized. */
    @JvmStatic
    fun applyOnce(context: Context) {
        if (!SyriaPrefs.isRouteOptimizationDefaultApplied(context)) {
            RoutingOptions.setRouteOptimizationEnabled(true)
            SyriaPrefs.markRouteOptimizationDefaultApplied(context)
        }
        // Larger map labels (Settings → "Increase font size on the map"); applies to the map right away.
        if (!SyriaPrefs.isLargeFontsDefaultApplied(context)) {
            Config.setLargeFontsSize(true)
            SyriaPrefs.markLargeFontsDefaultApplied(context)
        }
        // Settings → "Keep screen on" while the map is shown; MwmActivity applies it in onResume().
        if (!SyriaPrefs.isKeepScreenOnDefaultApplied(context)) {
            Config.setKeepScreenOnEnabled(true)
            SyriaPrefs.markKeepScreenOnDefaultApplied(context)
        }
    }
}
