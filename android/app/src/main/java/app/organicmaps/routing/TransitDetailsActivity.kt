package app.organicmaps.routing

import android.app.Activity
import android.content.Intent
import android.os.Build
import androidx.fragment.app.Fragment
import app.organicmaps.R
import app.organicmaps.base.BaseMwmFragmentActivity

/**
 * Full-screen host for the transit route details ([TransitDetailsFragment]). Pushed on top of the
 * routing panel with a slide-in-from-the-right transition when the transit summary row is tapped.
 */
class TransitDetailsActivity : BaseMwmFragmentActivity() {
    override fun getFragmentClass(): Class<out Fragment> = TransitDetailsFragment::class.java

    override fun finish() {
        super.finish()
        if (useCustomTransitions()) {
            @Suppress("DEPRECATION")
            overridePendingTransition(R.anim.slide_in_left, R.anim.slide_out_right)
        }
    }

    companion object {
        @JvmStatic
        fun start(activity: Activity) {
            activity.startActivity(Intent(activity, TransitDetailsActivity::class.java))
            if (useCustomTransitions()) {
                @Suppress("DEPRECATION")
                activity.overridePendingTransition(R.anim.slide_in_right, R.anim.slide_out_left)
            }
        }

        // API 34+ animates the predictive back gesture itself; a custom slide on top of it would fight it.
        private fun useCustomTransitions() = Build.VERSION.SDK_INT < Build.VERSION_CODES.UPSIDE_DOWN_CAKE
    }
}
