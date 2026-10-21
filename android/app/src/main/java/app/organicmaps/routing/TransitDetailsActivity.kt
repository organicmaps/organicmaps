package app.organicmaps.routing

import android.app.Activity
import android.content.Intent
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
        @Suppress("DEPRECATION")
        overridePendingTransition(R.anim.slide_in_left, R.anim.slide_out_right)
    }

    companion object {
        @JvmStatic
        fun start(activity: Activity) {
            activity.startActivity(Intent(activity, TransitDetailsActivity::class.java))
            // overrideActivityTransition requires API 34; keep parity down to minSdk.
            @Suppress("DEPRECATION")
            activity.overridePendingTransition(R.anim.slide_in_right, R.anim.slide_out_left)
        }
    }
}
