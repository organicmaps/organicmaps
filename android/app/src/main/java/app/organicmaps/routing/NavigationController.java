package app.organicmaps.routing;

import static app.organicmaps.sdk.util.Utils.dimen;

import android.location.Location;
import android.view.View;
import android.view.ViewGroup;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.lifecycle.ViewModelProvider;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.maplayer.MapButtonsViewModel;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.Router;
import app.organicmaps.sdk.maplayer.traffic.TrafficManager;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.routing.RoutingInfo;
import app.organicmaps.sdk.util.StringUtils;
import app.organicmaps.sdk.widgets.speedlimit.SpeedLimitView;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.WindowInsetUtils;
import app.organicmaps.widget.menu.NavMenu;
import com.google.android.material.bottomsheet.BottomSheetBehavior;

public class NavigationController implements TrafficManager.TrafficCallback, NavMenu.NavMenuListener
{
  private final View mFrame;

  @NonNull
  private final ManeuverView mManeuverView;
  @NonNull
  private final SpeedLimitView mSpeedLimit;

  private final MapButtonsViewModel mMapButtonsViewModel;

  private final NavMenu mNavMenu;
  View.OnClickListener mOnSettingsClickListener;
  View.OnClickListener mOnVoiceSettingsClickListener;

  public NavigationController(AppCompatActivity activity, View.OnClickListener onSettingsClickListener,
                              View.OnClickListener onVoiceSettingsClickListener,
                              NavMenu.OnMenuSizeChangedListener onMenuSizeChangedListener)
  {
    mMapButtonsViewModel = new ViewModelProvider(activity).get(MapButtonsViewModel.class);

    mFrame = activity.findViewById(R.id.navigation_frame);
    mNavMenu = new NavMenu(activity, this, onMenuSizeChangedListener);
    mOnSettingsClickListener = onSettingsClickListener;
    mOnVoiceSettingsClickListener = onVoiceSettingsClickListener;

    final ViewGroup topFrame = mFrame.findViewById(R.id.nav_top_frame);
    mManeuverView = ViewCompat.requireViewById(topFrame, R.id.maneuver_view);
    mSpeedLimit = ViewCompat.requireViewById(topFrame, R.id.nav_speed_limit);

    // Window-inset handling. Each listener below owns exactly one view. The layout family
    // (portrait phone / landscape phone / sw600dp tablet, both orientations) is mirrored by
    // bool resources declared in the same qualifier buckets as layout_nav.xml, so the code
    // branches cannot diverge from whichever layout was inflated. All writes are guarded by
    // equality checks: TYPE_SAFE_DRAWING includes system bars and IME, so inset dispatches
    // arrive on every frame of an inset animation.
    final View navigationBarBackground = mFrame.findViewById(R.id.nav_bottom_sheet_nav_bar);
    final View bottomSheet = mFrame.findViewById(R.id.nav_bottom_sheet);
    final int minStartMargin = dimen(activity, R.dimen.nav_side_margin_min);

    // Maneuver card: the top inset becomes padding inside the card, and the start edge keeps
    // at least nav_side_margin_min (the same value as the XML margins) while clearing a side
    // display cutout in any orientation — portrait side insets are nonzero on
    // waterfall/curved-edge displays.
    ViewCompat.setOnApplyWindowInsetsListener(mManeuverView, (v, windowInsets) -> {
      final Insets insets = windowInsets.getInsets(WindowInsetUtils.TYPE_SAFE_DRAWING);
      if (v.getPaddingTop() != insets.top)
        v.setPaddingRelative(v.getPaddingStart(), insets.top, v.getPaddingEnd(), v.getPaddingBottom());
      setStartMarginIfChanged(v, startMarginFor(v, insets, minStartMargin));
      return windowInsets;
    });

    // Speed limit: only when it is laid out beside the card (phone landscape) it is
    // top-aligned with the card and needs the same top offset the card gets via padding.
    // When it is constrained below the card, the card's padding already shifts it.
    if (activity.getResources().getBoolean(R.bool.nav_speed_limit_beside_card))
    {
      ViewCompat.setOnApplyWindowInsetsListener(mSpeedLimit, (v, windowInsets) -> {
        final Insets insets = windowInsets.getInsets(WindowInsetUtils.TYPE_SAFE_DRAWING);
        final ViewGroup.MarginLayoutParams lp = (ViewGroup.MarginLayoutParams) v.getLayoutParams();
        if (lp.topMargin != insets.top)
        {
          lp.topMargin = insets.top;
          v.requestLayout();
        }
        return windowInsets;
      });
    }

    // Bottom sheet: a fixed-width sheet (landscape, tablets) is start-aligned like the card
    // and clears a side cutout the same way; a match_parent sheet (phone portrait) must keep
    // its full width, so no listener is attached. LayoutParams.width is an XML constant,
    // valid before the first layout pass.
    if (bottomSheet.getLayoutParams().width > 0)
    {
      ViewCompat.setOnApplyWindowInsetsListener(bottomSheet, (v, windowInsets) -> {
        final Insets insets = windowInsets.getInsets(WindowInsetUtils.TYPE_SAFE_DRAWING);
        setStartMarginIfChanged(v, startMarginFor(v, insets, minStartMargin));
        return windowInsets;
      });
    }

    // Navigation-bar background: fills the gesture-area gap below the menu, outside the
    // sheet. Mirror the sheet's width and start margin so both stay aligned; sizing from the
    // sheet's constant LayoutParams.width works on the very first inset dispatch, before any
    // layout pass has run.
    ViewCompat.setOnApplyWindowInsetsListener(navigationBarBackground, (v, windowInsets) -> {
      final Insets insets = windowInsets.getInsets(WindowInsetUtils.TYPE_SAFE_DRAWING);
      final ViewGroup.MarginLayoutParams lp = (ViewGroup.MarginLayoutParams) v.getLayoutParams();
      final int sheetWidth = bottomSheet.getLayoutParams().width;
      final int width = sheetWidth > 0 ? sheetWidth : ViewGroup.LayoutParams.MATCH_PARENT;
      final int startMargin = sheetWidth > 0 ? startMarginFor(v, insets, minStartMargin) : 0;
      if (lp.width != width || lp.height != insets.bottom || lp.getMarginStart() != startMargin)
      {
        lp.width = width;
        lp.height = insets.bottom;
        lp.setMarginStart(startMargin);
        v.requestLayout();
      }
      return windowInsets;
    });

    // topButtonsMarginTop pushes the track-recording status FAB (pinned to the top-end
    // corner of the map-buttons overlay) down so it clears the nav header. Only the
    // portrait-phone card spans the full width and covers that corner, so only there the
    // FAB must clear the actual card height; in landscape and on tablets the card is a
    // start-aligned fixed-width column that never overlaps the top-end corner.
    final int navFramePadding = dimen(activity, R.dimen.nav_frame_padding);
    mMapButtonsViewModel.setTopButtonsMarginTop(navFramePadding);
    if (activity.getResources().getBoolean(R.bool.nav_maneuver_card_full_width))
      mManeuverView.addOnLayoutChangeListener(
          (v, left, top, right, bottom, oldLeft, oldTop, oldRight,
           oldBottom) -> mMapButtonsViewModel.setTopButtonsMarginTop(v.getHeight() + navFramePadding));

    // The expanded search sheet must stop below the nav header instead of covering it:
    // publish the bottom of the lowest visible header child. SearchFragmentController adds
    // the top window inset itself (expandedOffset = topInset + topHeaderHeight), and the
    // card already contains that inset as padding, so subtract it here to avoid doubling.
    topFrame.addOnLayoutChangeListener((v, l, t, r, b, ol, ot, or, ob) -> {
      int maxBottom = 0;
      for (int i = 0; i < topFrame.getChildCount(); ++i)
      {
        final View child = topFrame.getChildAt(i);
        if (UiUtils.isVisible(child))
          maxBottom = Math.max(maxBottom, child.getBottom());
      }
      final WindowInsetsCompat windowInsets = ViewCompat.getRootWindowInsets(v);
      final int topInset = windowInsets == null
                             ? 0
                             : Math.max(windowInsets.getInsets(WindowInsetsCompat.Type.systemBars()).top,
                                        windowInsets.getInsets(WindowInsetsCompat.Type.displayCutout()).top);
      mMapButtonsViewModel.setTopHeaderHeight(Math.max(0, maxBottom - topInset));
    });
  }

  /**
   * Start-edge inset for a start-aligned view: on RTL devices it sits on the right, so the
   * relevant cutout edge is insets.right. Never less than {@code minStartMargin}.
   */
  private static int startMarginFor(@NonNull View v, @NonNull Insets insets, int minStartMargin)
  {
    final int sideInset = v.getLayoutDirection() == View.LAYOUT_DIRECTION_RTL ? insets.right : insets.left;
    return Math.max(minStartMargin, sideInset);
  }

  private static void setStartMarginIfChanged(@NonNull View v, int startMargin)
  {
    final ViewGroup.MarginLayoutParams lp = (ViewGroup.MarginLayoutParams) v.getLayoutParams();
    if (lp.getMarginStart() != startMargin)
    {
      lp.setMarginStart(startMargin);
      v.requestLayout();
    }
  }

  public void update(@Nullable RoutingInfo info)
  {
    if (info == null)
      return;

    if (Router.get() == Router.Pedestrian)
      mManeuverView.updatePedestrian(info);
    else
    {
      mManeuverView.updateVehicle(info);
      updateSpeedLimit(info);
    }
    mNavMenu.update(info);
  }

  public void updateNorth()
  {
    if (!RoutingController.get().isNavigating())
      return;

    update(Framework.nativeGetRouteFollowingInfo());
  }

  public void show(boolean show)
  {
    if (show && !UiUtils.isVisible(mFrame))
    {
      collapseNavMenu();
      // Seed the panel from the already-built route so it isn't empty until the first GPS fix arrives.
      update(RoutingController.get().getCachedRoutingInfo());
    }
    UiUtils.showIf(show, mFrame);
    if (!show)
      mMapButtonsViewModel.setTopHeaderHeight(0);
  }

  public boolean isNavMenuCollapsed()
  {
    return mNavMenu.getBottomSheetState() == BottomSheetBehavior.STATE_COLLAPSED;
  }

  public boolean isNavMenuHidden()
  {
    return mNavMenu.getBottomSheetState() == BottomSheetBehavior.STATE_HIDDEN;
  }

  public void collapseNavMenu()
  {
    mNavMenu.collapseNavBottomSheet();
  }

  public void refresh()
  {
    mNavMenu.refreshTts();
  }

  @Override
  public void onEnabled()
  {}

  @Override
  public void onDisabled()
  {}

  @Override
  public void onWaitingData()
  {}

  @Override
  public void onOutdated()
  {}

  @Override
  public void onNoData()
  {}

  @Override
  public void onNetworkError()
  {}

  @Override
  public void onExpiredData()
  {}

  @Override
  public void onExpiredApp()
  {}

  @Override
  public void onSettingsClicked()
  {
    mOnSettingsClickListener.onClick(null);
  }

  @Override
  public void onTtsVoiceSettingsClicked()
  {
    mOnVoiceSettingsClickListener.onClick(null);
  }

  @Override
  public void onStopClicked()
  {
    RoutingController.get().cancel();
  }

  private void updateSpeedLimit(@NonNull final RoutingInfo info)
  {
    final Location location = MwmApplication.from(mFrame.getContext()).getLocationHelper().getSavedLocation();
    final boolean speedLimitExceeded = location != null && info.speedLimitMps < location.getSpeed();
    mSpeedLimit.setSpeedLimit(StringUtils.nativeFormatSpeed(info.speedLimitMps), speedLimitExceeded);
  }
}
