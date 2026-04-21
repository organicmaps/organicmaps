package app.organicmaps.routing;

import static app.organicmaps.sdk.util.Utils.dimen;

import android.content.res.Configuration;
import android.location.Location;
import android.view.View;
import android.view.ViewGroup;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.lifecycle.ViewModelProvider;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.maplayer.MapButtonsViewModel;
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
  private final View mTopFrame;
  private final View mNextTurnContainer;

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

    mTopFrame = mFrame.findViewById(R.id.nav_top_frame);
    mManeuverView = mTopFrame.requireViewById(R.id.maneuver_view);
    mSpeedLimit = mTopFrame.requireViewById(R.id.nav_speed_limit);
    mNextTurnContainer = mFrame.findViewById(R.id.nav_next_turn_container);

    final boolean isLandscape = activity.getResources().getConfiguration().orientation
        == Configuration.ORIENTATION_LANDSCAPE;

    // Apply status-bar + display-cutout insets so neither the maneuver card nor the
    // bottom sheet renders behind the camera.  The left cutout is applied as a start
    // margin (both views shift right), while the top status-bar inset becomes padding
    // inside the maneuver card.  Nav-bar height is forwarded to the background view
    // that fills the gesture area below the menu.
    final View navigationBarBackground = mFrame.findViewById(R.id.nav_bottom_sheet_nav_bar);
    final View bottomSheet = mFrame.findViewById(R.id.nav_bottom_sheet);
    final int minStartMargin = dimen(activity, R.dimen.nav_side_margin_min);
    ViewCompat.setOnApplyWindowInsetsListener(mManeuverView, (v, windowInsets) -> {
      final Insets insets = windowInsets.getInsets(WindowInsetUtils.TYPE_SAFE_DRAWING);
      // In landscape keep at least nav_side_margin_min breathing room even when there
      // is no display cutout (insets.left == 0).  In portrait insets.left is always 0.
      final int startMargin = isLandscape ? Math.max(minStartMargin, insets.left) : insets.left;

      // In landscape shift both cards past the camera cutout (or apply the minimum margin).
      // In portrait XML margins are left untouched.
      if (isLandscape)
      {
        ((ViewGroup.MarginLayoutParams) v.getLayoutParams()).setMarginStart(startMargin);
        v.requestLayout();

        final ViewGroup.MarginLayoutParams sheetParams = (ViewGroup.MarginLayoutParams) bottomSheet.getLayoutParams();
        sheetParams.setMarginStart(startMargin);
        bottomSheet.requestLayout();
      }

      // Status-bar inset: padding inside the maneuver card so content clears the status bar.
      // In landscape the speed limit sits top-aligned with the card, so it needs the same
      // top offset; in portrait it is below the card and already inherits the clearance.
      v.setPaddingRelative(v.getPaddingStart(), insets.top, v.getPaddingEnd(), v.getPaddingBottom());
      if (isLandscape)
      {
        ((ViewGroup.MarginLayoutParams) mSpeedLimit.getLayoutParams()).topMargin = insets.top;
        mSpeedLimit.requestLayout();
      }

      // Nav-bar background sits outside the sheet — align it with the sheet and size it to fill
      // the system nav bar gap below.
      final ViewGroup.MarginLayoutParams navBarParams = (ViewGroup.MarginLayoutParams) navigationBarBackground.getLayoutParams();
      final ViewGroup.LayoutParams sheetLayoutParams = bottomSheet.getLayoutParams();
      navBarParams.setMarginStart(startMargin);
      navBarParams.width = sheetLayoutParams.width > 0 ? sheetLayoutParams.width : bottomSheet.getWidth();
      navBarParams.height = insets.bottom;
      navigationBarBackground.requestLayout();
      return windowInsets;
    });

    // Right-side map controls (zoom, recenter) sit on the opposite side of the card;
    // only the base padding is needed.
    mMapButtonsViewModel.setTopButtonsMarginTop(dimen(activity, R.dimen.nav_frame_padding));
  }

  public void update(@Nullable RoutingInfo info)
  {
    if (info == null)
      return;

    if (Router.get() == Router.Pedestrian)
      mManeuverView.updatePedestrian(info);
    else
      mManeuverView.updateVehicle(info);

    updateSpeedLimit(info);
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
  public void onEnabled() {}

  @Override
  public void onDisabled() {}

  @Override
  public void onWaitingData() {}

  @Override
  public void onOutdated() {}

  @Override
  public void onNoData() {}

  @Override
  public void onNetworkError() {}

  @Override
  public void onExpiredData() {}

  @Override
  public void onExpiredApp() {}

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
