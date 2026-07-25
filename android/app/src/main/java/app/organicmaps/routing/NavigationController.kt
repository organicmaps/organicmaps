package app.organicmaps.routing

import android.view.View
import android.view.ViewGroup
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.view.ViewCompat
import androidx.core.view.WindowInsetsCompat
import androidx.lifecycle.ViewModelProvider
import app.organicmaps.MwmApplication
import app.organicmaps.R
import app.organicmaps.maplayer.MapButtonsViewModel
import app.organicmaps.sdk.Router
import app.organicmaps.sdk.routing.RoutingController
import app.organicmaps.sdk.routing.RoutingInfo
import app.organicmaps.sdk.util.StringUtils
import app.organicmaps.sdk.util.Utils.dimen
import app.organicmaps.sdk.widgets.speedlimit.SpeedLimitView
import app.organicmaps.util.UiUtils
import app.organicmaps.util.WindowInsetUtils
import app.organicmaps.widget.menu.NavMenu
import com.google.android.material.bottomsheet.BottomSheetBehavior

class NavigationController(
    activity: AppCompatActivity,
    private val onSettingsClick: () -> Unit,
    private val onVoiceSettingsClick: () -> Unit,
    onMenuSizeChangedListener: NavMenu.OnMenuSizeChangedListener,
) : NavMenu.NavMenuListener {

    private val frame: View = ActivityCompat.requireViewById(activity, R.id.navigation_frame)
    private val topFrame: ViewGroup = ViewCompat.requireViewById(frame, R.id.nav_top_frame)
    private val maneuverView: ManeuverView = ViewCompat.requireViewById(topFrame, R.id.maneuver_view)
    private val speedLimit: SpeedLimitView = ViewCompat.requireViewById(topFrame, R.id.nav_speed_limit)
    private val bottomSheet: View = ViewCompat.requireViewById(frame, R.id.nav_bottom_sheet)
    private val navigationBarBackground: View = ViewCompat.requireViewById(frame, R.id.nav_bottom_sheet_nav_bar)
    private val minStartMargin = dimen(activity, R.dimen.nav_side_margin_min)
    private val mapButtonsViewModel = ViewModelProvider(activity)[MapButtonsViewModel::class.java]
    private val navMenu = NavMenu(activity, this, onMenuSizeChangedListener)

    // True only for the edge-to-edge portrait phone layout (full-width card + full-width sheet);
    // landscape and tablets use a fixed-width floating card. Single source for every width/inset
    // branch below; MwmActivity reads the same R.bool for the status-bar icon color.
    private val isFullWidthCard = activity.resources.getBoolean(R.bool.nav_full_width_card)

    init {
        ViewCompat.setOnApplyWindowInsetsListener(frame) { _, insets -> applyNavInsets(insets) }

        // Push the FAB below the nav header; only the full-width card overlaps it, so only there
        // does the FAB track the card height.
        val navFramePadding = dimen(frame.context, R.dimen.nav_frame_padding)
        mapButtonsViewModel.setTopButtonsMarginTop(navFramePadding)
        if (isFullWidthCard) {
            maneuverView.addOnLayoutChangeListener { v, _, _, _, _, _, _, _, _ ->
                mapButtonsViewModel.setTopButtonsMarginTop(v.height + navFramePadding)
            }
        }
    }

    // Single inset pass over the nav header: pad nav_top_frame (side cutout as start, status bar as
    // top) so the card and speed limit shift together, then delegate the sheet and its strip below.
    private fun applyNavInsets(windowInsets: WindowInsetsCompat): WindowInsetsCompat {
        val insets = windowInsets.getInsets(WindowInsetUtils.TYPE_SAFE_DRAWING)
        // On RTL a start-aligned view sits on the right, so its cutout edge is insets.right.
        val isRtl = topFrame.layoutDirection == View.LAYOUT_DIRECTION_RTL
        val sideInset = if (isRtl) insets.right else insets.left
        val startMargin = if (isFullWidthCard) sideInset else maxOf(minStartMargin, sideInset)

        // Full-width portrait card fills behind the status bar: no frame top padding; the card
        // takes the status-bar inset as internal padding (background up, content below).
        val framePaddingTop = if (isFullWidthCard) 0 else insets.top
        if (topFrame.paddingStart != startMargin || topFrame.paddingTop != framePaddingTop) {
            topFrame.setPaddingRelative(startMargin, framePaddingTop, 0, 0)
        }
        // Clear only the status bar (matches the scrim), not the full safe inset — a tall cutout gaps.
        val statusBarTop = windowInsets.getInsets(WindowInsetsCompat.Type.statusBars()).top
        maneuverView.setFillToTop(isFullWidthCard, statusBarTop)

        applySheetTranslation(isRtl, startMargin)
        applyNavBarStrip(isRtl, startMargin, windowInsets)
        return windowInsets
    }

    // Landscape/tablet: the fixed-width sheet ignores horizontal margins, so shift it with the
    // card via translationX. The full-width portrait sheet needs no offset.
    private fun applySheetTranslation(isRtl: Boolean, startMargin: Int) {
        if (isFullWidthCard) {
            return
        }
        val translationX = (if (isRtl) -startMargin else startMargin).toFloat()
        if (bottomSheet.translationX != translationX) {
            bottomSheet.translationX = translationX
        }
    }

    // Mirror the nav-bar strip below the sheet. Height from systemBars (not TYPE_SAFE_DRAWING) so
    // the strip doesn't grow to IME height; set the raw edge margin too — setMarginStart alone can
    // go stale on re-dispatch.
    private fun applyNavBarStrip(isRtl: Boolean, startMargin: Int, windowInsets: WindowInsetsCompat) {
        val navBarLp = navigationBarBackground.layoutParams as ViewGroup.MarginLayoutParams
        // Full-width sheet → strip spans the screen; fixed-width sheet → strip mirrors its width.
        val navBarWidth =
            if (isFullWidthCard) ViewGroup.LayoutParams.MATCH_PARENT else bottomSheet.layoutParams.width
        val navBarHeight = windowInsets.getInsets(WindowInsetsCompat.Type.systemBars()).bottom
        val navBarEdgeMargin = if (isRtl) navBarLp.rightMargin else navBarLp.leftMargin
        if (navBarLp.width != navBarWidth || navBarLp.height != navBarHeight || navBarEdgeMargin != startMargin) {
            navBarLp.width = navBarWidth
            navBarLp.height = navBarHeight
            navBarLp.marginStart = startMargin
            if (isRtl) navBarLp.rightMargin = startMargin else navBarLp.leftMargin = startMargin
            navigationBarBackground.requestLayout()
        }
    }

    fun update(info: RoutingInfo?) {
        info ?: return

        if (Router.get() == Router.Pedestrian) {
            maneuverView.updatePedestrian(info)
        } else {
            maneuverView.updateVehicle(info)
            updateSpeedLimit(info)
        }
        navMenu.update(info)
    }

    fun show(visible: Boolean) {
        if (visible && !UiUtils.isVisible(frame)) {
            collapseNavMenu()
            // Seed from the built route so the panel isn't empty before the first GPS fix.
            update(RoutingController.get().cachedRoutingInfo)
        }
        UiUtils.showIf(visible, frame)
        if (!visible) {
            mapButtonsViewModel.setTopHeaderHeight(0)
        }
    }

    fun isNavMenuCollapsed(): Boolean = navMenu.bottomSheetState == BottomSheetBehavior.STATE_COLLAPSED

    fun isNavMenuHidden(): Boolean = navMenu.bottomSheetState == BottomSheetBehavior.STATE_HIDDEN

    fun collapseNavMenu() = navMenu.collapseNavBottomSheet()

    fun refresh() = navMenu.refreshTts()

    override fun onSettingsClicked() = onSettingsClick()

    override fun onTtsVoiceSettingsClicked() = onVoiceSettingsClick()

    override fun onStopClicked() {
        RoutingController.get().cancel()
    }

    private fun updateSpeedLimit(info: RoutingInfo) {
        val location = MwmApplication.from(frame.context).locationHelper.savedLocation
        val speedLimitExceeded = location != null && info.speedLimitMps > 0 && info.speedLimitMps < location.speed
        speedLimit.setSpeedLimit(StringUtils.nativeFormatSpeed(info.speedLimitMps), speedLimitExceeded)
    }
}
