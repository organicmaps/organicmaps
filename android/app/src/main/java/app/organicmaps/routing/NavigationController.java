package app.organicmaps.routing;

import static app.organicmaps.sdk.util.Utils.dimen;

import android.content.res.Configuration;
import android.location.Location;
import android.text.TextUtils;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;
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
import app.organicmaps.sdk.widget.roadshield.RoadShieldUtils;
import app.organicmaps.sdk.widgets.lanes.LanesView;
import app.organicmaps.sdk.widgets.speedlimit.SpeedLimitView;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.Utils;
import app.organicmaps.util.WindowInsetUtils;
import app.organicmaps.widget.menu.NavMenu;
import com.google.android.material.bottomsheet.BottomSheetBehavior;

public class NavigationController implements TrafficManager.TrafficCallback, NavMenu.NavMenuListener
{
  private final View mFrame;

  private final ImageView mNextTurnImage;
  private final TextView mNextTurnDistance;

  private final View mNextNextTurnFrame;
  private final ImageView mNextNextTurnImage;

  private final View mStreetFrame;
  private final TextView mNextStreet;

  @NonNull
  private final LanesView mLanesView;
  @NonNull
  private final SpeedLimitView mSpeedLimit;

  private final MapButtonsViewModel mMapButtonsViewModel;
  private final View mTopFrame;
  private final View mNextTurnContainer;

  private final NavMenu mNavMenu;
  View.OnClickListener mOnSettingsClickListener;

  private void addWindowsInsets(@NonNull View topFrame)
  {
    ViewCompat.setOnApplyWindowInsetsListener(
        topFrame.findViewById(R.id.nav_next_turn_container), (view, windowInsets) -> {
          view.setPadding(windowInsets.getInsets(WindowInsetsCompat.Type.systemBars()).left, view.getPaddingTop(),
                          view.getPaddingEnd(), view.getPaddingBottom());
          return windowInsets;
        });
  }

  public NavigationController(AppCompatActivity activity, View.OnClickListener onSettingsClickListener,
                              NavMenu.OnMenuSizeChangedListener onMenuSizeChangedListener)
  {
    mMapButtonsViewModel = new ViewModelProvider(activity).get(MapButtonsViewModel.class);

    mFrame = activity.findViewById(R.id.navigation_frame);
    mNavMenu = new NavMenu(activity, this, onMenuSizeChangedListener);
    mOnSettingsClickListener = onSettingsClickListener;

    // Top frame
    mTopFrame = mFrame.findViewById(R.id.nav_top_frame);
    mTopFrame.addOnLayoutChangeListener(
        (v, l, t, r, b, ol, ot, or, ob) -> mMapButtonsViewModel.setTopHeaderHeight(computeNavContentHeight()));
    View turnFrame = mTopFrame.findViewById(R.id.nav_next_turn_frame);
    mNextTurnImage = turnFrame.findViewById(R.id.turn);
    mNextTurnDistance = turnFrame.findViewById(R.id.distance);

    addWindowsInsets(mTopFrame);

    mNextNextTurnFrame = mTopFrame.findViewById(R.id.nav_next_next_turn_frame);
    mNextNextTurnImage = mNextNextTurnFrame.findViewById(R.id.turn);

    mStreetFrame = mTopFrame.findViewById(R.id.street_frame);
    mNextStreet = mStreetFrame.findViewById(R.id.street);

    mLanesView = mTopFrame.findViewById(R.id.lanes);

    mSpeedLimit = mTopFrame.findViewById(R.id.nav_speed_limit);

    // Show a blank view below the navbar to hide the menu content
    final View navigationBarBackground = mFrame.findViewById(R.id.nav_bottom_sheet_nav_bar);
    mNextTurnContainer = mFrame.findViewById(R.id.nav_next_turn_container);
    ViewCompat.setOnApplyWindowInsetsListener(mStreetFrame, (v, windowInsets) -> {
      UiUtils.setViewInsetsPaddingNoBottom(v, windowInsets);

      final Insets safeDrawingInsets = windowInsets.getInsets(WindowInsetUtils.TYPE_SAFE_DRAWING);
      mNextTurnContainer.setPadding(safeDrawingInsets.left, mNextTurnContainer.getPaddingTop(),
                                    mNextTurnContainer.getPaddingEnd(), mNextTurnContainer.getPaddingBottom());
      navigationBarBackground.getLayoutParams().height = safeDrawingInsets.bottom;
      // The gesture navigation bar stays at the bottom in landscape
      // We need to add a background only above the nav menu
      navigationBarBackground.getLayoutParams().width = mFrame.findViewById(R.id.nav_bottom_sheet).getWidth();
      return windowInsets;
    });
  }

  private int computeNavContentHeight()
  {
    int height = mStreetFrame.getHeight();
    int a = 0, lanesHeight = 0;
    if (mFrame.getResources().getConfiguration().orientation == Configuration.ORIENTATION_PORTRAIT)
    {
      if (UiUtils.isVisible(mNextTurnContainer))
        a += mNextTurnContainer.getHeight();
      if (UiUtils.isVisible(mSpeedLimit))
        a += mSpeedLimit.getHeight();
    }
    if (UiUtils.isVisible(mLanesView))
      lanesHeight += mLanesView.getHeight();
    return height + Math.max(a, lanesHeight);
  }

  private void updateVehicle(@NonNull RoutingInfo info)
  {
    mNextTurnDistance.setText(Utils.formatDistance(mFrame.getContext(), info.distToTurn));
    mNextTurnImage.setImageResource(info.carDirection.getTurnRes(info.exitNum));

    final boolean showNextNextTurn = info.hasNextNextTurn();
    UiUtils.showIf(showNextNextTurn, mNextNextTurnFrame);
    if (showNextNextTurn)
      mNextNextTurnImage.setImageResource(info.nextCarDirection.getTurnRes());

    mLanesView.setLanes(info.lanes);

    updateSpeedLimit(info);
  }

  private void updatePedestrian(@NonNull RoutingInfo info)
  {
    mNextTurnDistance.setText(Utils.formatDistance(mFrame.getContext(), info.distToTurn));
    mNextTurnImage.setImageResource(info.pedestrianDirection.getTurnRes());
  }

  public void updateNorth()
  {
    if (!RoutingController.get().isNavigating())
      return;

    update(Framework.nativeGetRouteFollowingInfo());
  }

  public void update(@Nullable RoutingInfo info)
  {
    if (info == null)
      return;

    if (Router.get() == Router.Pedestrian)
      updatePedestrian(info);
    else
      updateVehicle(info);

    updateStreetView(info);
    mNavMenu.update(info);
  }

  private void updateStreetView(@NonNull RoutingInfo info)
  {
    boolean hasStreet = !TextUtils.isEmpty(info.nextStreet);
    // Sic: don't use UiUtils.showIf() here because View.GONE breaks layout
    // https://github.com/organicmaps/organicmaps/issues/3732
    UiUtils.visibleIf(hasStreet, mStreetFrame);
    if (!TextUtils.isEmpty(info.nextStreet))
      mNextStreet.setText(RoadShieldUtils.createStreetTextWithShields(info.nextStreet, info.nextStreetRoadShields,
                                                                      mNextStreet.getTextSize()));
    int margin = dimen(mFrame.getContext(), R.dimen.nav_frame_padding);
    if (hasStreet)
      margin += mStreetFrame.getHeight();
    mMapButtonsViewModel.setTopButtonsMarginTop(margin);
  }

  public void show(boolean show)
  {
    if (show && !UiUtils.isVisible(mFrame))
      collapseNavMenu();
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
  {
    // mNavMenu.refreshTraffic();
  }

  @Override
  public void onDisabled()
  {
    // mNavMenu.refreshTraffic();
  }

  @Override
  public void onWaitingData()
  {
    // no op
  }

  @Override
  public void onOutdated()
  {
    // no op
  }

  @Override
  public void onNoData()
  {
    // no op
  }

  @Override
  public void onNetworkError()
  {
    // no op
  }

  @Override
  public void onExpiredData()
  {
    // no op
  }

  @Override
  public void onExpiredApp()
  {
    // no op
  }

  @Override
  public void onSettingsClicked()
  {
    mOnSettingsClickListener.onClick(null);
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
