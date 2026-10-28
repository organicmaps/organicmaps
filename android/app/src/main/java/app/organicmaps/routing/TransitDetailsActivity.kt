package app.organicmaps.routing

import android.app.Activity
import android.content.Intent
import androidx.fragment.app.Fragment
import app.organicmaps.base.BaseMwmFragmentActivity

/**
 * Full-screen host for the transit route details ([TransitDetailsFragment]), opened from the routing panel
 * when the transit summary tile is tapped.
 */
class TransitDetailsActivity : BaseMwmFragmentActivity() {
    override fun getFragmentClass(): Class<out Fragment> = TransitDetailsFragment::class.java

    companion object {
        @JvmStatic
        fun start(activity: Activity) {
            activity.startActivity(Intent(activity, TransitDetailsActivity::class.java))
        }
    }
}
