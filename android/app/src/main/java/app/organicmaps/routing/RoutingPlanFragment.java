package app.organicmaps.routing;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CompoundButton;
import android.widget.RadioGroup;
import android.widget.TextView;
import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.DrawableRes;
import androidx.annotation.IdRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.fragment.app.Fragment;
import androidx.lifecycle.MediatorLiveData;
import androidx.lifecycle.Observer;
import androidx.lifecycle.ViewModelProvider;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.maplayer.MapButtonsController;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.Router;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.routing.RouteMarkType;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.routing.RoutingInfo;
import app.organicmaps.sdk.routing.TransitRouteInfo;
import app.organicmaps.sdk.util.SharedPropertiesUtils;
import app.organicmaps.search.SearchActivity;
import app.organicmaps.settings.DrivingOptionsActivity;
import app.organicmaps.settings.DrivingOptionsFragment;
import app.organicmaps.util.UiUtils;
import app.organicmaps.widget.RoutingToolbarButton;
import app.organicmaps.widget.WheelProgressView;
import com.google.android.material.bottomsheet.BottomSheetBehavior;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;

public class RoutingPlanFragment extends Fragment implements View.OnLayoutChangeListener, RoutingBottomMenuListener
{
  public static final String TAG = RoutingPlanFragment.class.getSimpleName();

  private RoutingPlanViewModel mViewModel;
  private View mRoutingDetails;
  private View mDrivingOptionsBtn;
  private View mFrame;
  private View mRoutingRoot;
  private RadioGroup mRouterTypes;
  private View mProgressFrame;
  private WheelProgressView mProgressVehicle;
  private WheelProgressView mProgressPedestrian;
  private WheelProgressView mProgressTransit;
  private WheelProgressView mProgressBicycle;
  private WheelProgressView mProgressRuler;
  private RoutingBottomMenuController mRoutingBottomMenuController;
  private TextView mDrivingOptionsBadge;
  private View mSearchBtn;
  private View mBookmarkBtn;
  private View mActionFrame;
  private View mRoutingContainer;
  private View mRoutingTypesContainer;
  private MapButtonsController.MapButtonClickListener mMapButtonClickListener;
  private RoutingPlanController mRoutingPlanController;
  private BottomSheetBehavior<View> mSheetBehavior;
  private int mBottomButtonsMaxHeight;
  private int mPeekHeightMargins;
  private View mButtonsLayout;
  private int mTopInset;

  private final ActivityResultLauncher<Intent> startDrivingOptionsForResult =
      registerForActivityResult(new ActivityResultContracts.StartActivityForResult(), activityResult -> {
        if (activityResult.getResultCode() == android.app.Activity.RESULT_OK)
        {
          RoutingController.get().rebuildLastRoute();
          Intent data = activityResult.getData();
          if (data == null)
            return;
          int count = data.getIntExtra(DrivingOptionsFragment.DRIVING_OPTIONS_COUNT, 0);
          mViewModel.setDrivingOptionsCount(count);
        }
      });

  // Single source of truth for the sheet's visibility: planning wants it AND no place page is covering it.
  private final MediatorLiveData<Boolean> mSheetVisible = new MediatorLiveData<>();
  private final Observer<Integer> mMenuUpdateObserver = trigger -> updateMenuInternal();
  private final Observer<int[]> mBuildProgressObserver = progress ->
  {
    if (progress != null)
      updateBuildProgress(progress[0], Router.values()[progress[1]]);
  };
  private final Observer<Integer> mDrivingOptionsCountObserver = this::updateBadgeCount;
  private final Observer<Boolean> mDrivingOptionsErrorObserver = error ->
  {
    if (error != null && error)
      onDrivingOptionsBuildError();
  };

  @Override
  public void onAttach(@NonNull Context context)
  {
    super.onAttach(context);
    if (!(context instanceof MapButtonsController.MapButtonClickListener)
        || !(context instanceof RoutingPlanController))
      throw new IllegalStateException("Host activity must implement the routing plan callbacks");
    mMapButtonClickListener = (MapButtonsController.MapButtonClickListener) context;
    mRoutingPlanController = (RoutingPlanController) context;
  }

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.routing_bottom_sheet, container, false);
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    mViewModel = new ViewModelProvider(requireActivity()).get(RoutingPlanViewModel.class);
    mFrame = view.findViewById(R.id.routing_sheet_frame);
    mRoutingTypesContainer = mFrame.findViewById(R.id.routing_types_frame);
    mRouterTypes = mFrame.findViewById(R.id.route_type2);
    mPeekHeightMargins = getResources().getDimensionPixelSize(R.dimen.routing_margin_peek_height);
    mBottomButtonsMaxHeight = getResources().getDimensionPixelSize(R.dimen.routing_bottom_buttons_max_height);

    setupRouterButtons();

    mProgressFrame = mFrame.findViewById(R.id.progress_frame2);
    mProgressVehicle = mProgressFrame.findViewById(R.id.progress_vehicle);
    mProgressPedestrian = mProgressFrame.findViewById(R.id.progress_pedestrian);
    mProgressTransit = mProgressFrame.findViewById(R.id.progress_transit);
    mProgressBicycle = mProgressFrame.findViewById(R.id.progress_bicycle);
    mProgressRuler = mProgressFrame.findViewById(R.id.progress_ruler);

    mRoutingDetails = mFrame.findViewById(R.id.routing_details);
    mActionFrame = view.findViewById(R.id.routing_action_frame);
    mRoutingContainer = requireActivity().findViewById(R.id.routing_container);
    mButtonsLayout = view.findViewById(R.id.routing_bottom_buttons);

    mRoutingBottomMenuController = RoutingBottomMenuController.newInstance(requireActivity(), mFrame, this);
    mRoutingBottomMenuController.setVisibilityChangedCallback(this::updateMapButtonsOffset);
    mRoutingRoot = view.findViewById(R.id.routing_root);

    mDrivingOptionsBadge = mFrame.findViewById(R.id.driving_options_badge);
    mDrivingOptionsBtn = mRoutingDetails.findViewById(R.id.driving_options_btn_img);
    mDrivingOptionsBtn.setOnClickListener(
        v -> DrivingOptionsActivity.start(requireActivity(), startDrivingOptionsForResult));

    mSearchBtn = mRoutingRoot.findViewById(R.id.routing_btn_search);
    mBookmarkBtn = mButtonsLayout.findViewById(R.id.routing_btn_bookmarks);
    mSearchBtn.setOnClickListener(
        v -> mMapButtonClickListener.onMapButtonClick(MapButtonsController.MapButtons.search));
    mBookmarkBtn.setOnClickListener(
        v -> mMapButtonClickListener.onMapButtonClick(MapButtonsController.MapButtons.bookmarks));

    final View closeButton = mRoutingTypesContainer.findViewById(R.id.back);
    closeButton.setOnClickListener(v -> requireActivity().getOnBackPressedDispatcher().onBackPressed());

    setInsets();
    setupBottomSheetBehavior();

    mSheetVisible.addSource(mViewModel.getShowRoutingBottomSheet(), show -> updateSheetVisible());
    mSheetVisible.addSource(mViewModel.getIsPlacePageActive(), active -> updateSheetVisible());
    mSheetVisible.observe(getViewLifecycleOwner(), this::showSheet);
    mViewModel.getMenuUpdateTrigger().observe(getViewLifecycleOwner(), mMenuUpdateObserver);
    mViewModel.getBuildProgress().observe(getViewLifecycleOwner(), mBuildProgressObserver);
    mViewModel.getDrivingOptionsCount().observe(getViewLifecycleOwner(), mDrivingOptionsCountObserver);
    mViewModel.getDrivingOptionsErrorTrigger().observe(getViewLifecycleOwner(), mDrivingOptionsErrorObserver);

    if (savedInstanceState != null)
      restoreRoutingPanelState(savedInstanceState);

    updateBadgeCount(SharedPropertiesUtils.getDrivingOptionsCount());
    mRoutingContainer.addOnLayoutChangeListener(this);
  }

  private void setInsets()
  {
    ViewCompat.setOnApplyWindowInsetsListener(mRoutingRoot, (v, insets) -> {
      final Insets mCurrentWindowInsets =
          insets.getInsets(WindowInsetsCompat.Type.systemBars() | WindowInsetsCompat.Type.displayCutout());
      final int leftInset = mCurrentWindowInsets.left;
      final int rightInset = mCurrentWindowInsets.right;
      final int bottomInset = mCurrentWindowInsets.bottom;
      mTopInset = mCurrentWindowInsets.top;
      mRoutingRoot.setPadding(leftInset, mTopInset, rightInset, 0);
      mActionFrame.setPadding(0, 0, rightInset, bottomInset);
      mButtonsLayout.setPadding(0, 0, 0, bottomInset);
      return ViewCompat.onApplyWindowInsets(v, insets);
    });
  }

  private void setupBottomSheetBehavior()
  {
    mSheetBehavior = BottomSheetBehavior.from(mFrame);
    mSheetBehavior.setHideable(true);
    mSheetBehavior.setFitToContents(true);
    mSheetBehavior.addBottomSheetCallback(new BottomSheetBehavior.BottomSheetCallback() {
      @Override
      public void onStateChanged(@NonNull View bottomSheet, int newState)
      {
        if (newState != BottomSheetBehavior.STATE_SETTLING && newState != BottomSheetBehavior.STATE_DRAGGING
            && newState != BottomSheetBehavior.STATE_HIDDEN)
          mViewModel.setBottomSheetState(newState);
      }

      @Override
      public void onSlide(@NonNull View bottomSheet, float slideOffset)
      {
        mViewModel.setRoutingBottomDistanceToTop(bottomSheet.getTop() + mTopInset);
      }
    });
  }

  @Override
  public void onDestroyView()
  {
    super.onDestroyView();
    mRoutingContainer.removeOnLayoutChangeListener(this);
  }

  private void updateMenuInternal()
  {
    final RoutingController controller = RoutingController.get();

    if (controller.isPlanning())
      setRoutingContentActive(false);

    if (controller.isBuilt())
      setRoutingContentActive(true);

    if (controller.isPlanning())
    {
      boolean actionFrameShown = showAddStartOrFinishFrame(controller);
      if (!actionFrameShown)
        hideActionFrame();
      mViewModel.setShowRoutingBottomSheet(!actionFrameShown);
    }
  }

  private boolean showAddStartOrFinishFrame(@NonNull RoutingController controller)
  {
    MapObject myPosition = MwmApplication.from(requireContext()).getLocationHelper().getMyPosition();

    if (myPosition != null && controller.getEndPoint() == null)
    {
      showAddFinishFrame();
      return true;
    }
    if (controller.getStartPoint() == null)
    {
      showAddStartFrame();
      return true;
    }
    if (controller.getEndPoint() == null)
    {
      showAddFinishFrame();
      return true;
    }
    return false;
  }

  private void updateSheetVisible()
  {
    final boolean show = Boolean.TRUE.equals(mViewModel.getShowRoutingBottomSheet().getValue());
    final boolean placePageActive = Boolean.TRUE.equals(mViewModel.getIsPlacePageActive().getValue());
    mSheetVisible.setValue(show && !placePageActive);
  }

  private void showSheet(boolean show)
  {
    if (show)
    {
      UiUtils.show(mButtonsLayout, mFrame);
      mFrame.post(() -> {
        // The view may be destroyed before this runs; bail out instead of touching a detached sheet.
        if (getView() == null)
          return;
        mSheetBehavior.setHideable(false);
        final int state = mViewModel.getBottomSheetState();
        mSheetBehavior.setState(state == BottomSheetBehavior.STATE_HIDDEN ? BottomSheetBehavior.STATE_COLLAPSED
                                                                          : state);
      });
    }
    else
    {
      mSheetBehavior.setHideable(true);
      mSheetBehavior.setState(BottomSheetBehavior.STATE_HIDDEN);
      UiUtils.hide(mButtonsLayout);
    }
  }

  private void setRoutingContentActive(boolean active)
  {
    if (active)
    {
      mDrivingOptionsBtn.setEnabled(true);
      mRoutingDetails.setAlpha(1.0f);
    }
    else
    {
      mDrivingOptionsBtn.setEnabled(false);
      mRoutingDetails.setAlpha(0.2f);
    }
  }

  public void updateMapButtonsOffset()
  {
    if (mFrame.getTop() == 0)
      return;
    final int newPeekHeight =
        mRoutingTypesContainer.getHeight() + mRoutingDetails.getHeight() + mBottomButtonsMaxHeight + mPeekHeightMargins;
    mSheetBehavior.setPeekHeight(newPeekHeight);
    final int newDistanceTop = mFrame.getTop() + mTopInset;
    mViewModel.setRoutingBottomDistanceToTop(newDistanceTop);
  }

  private void setupRouterButton(@IdRes int buttonId, final @DrawableRes int iconRes,
                                 View.OnClickListener clickListener)
  {
    CompoundButton.OnCheckedChangeListener listener = (buttonView, isChecked) ->
    {
      RoutingToolbarButton button = (RoutingToolbarButton) buttonView;
      button.setIcon(iconRes);
      if (isChecked)
        button.activate();
      else
        button.deactivate();
    };

    RoutingToolbarButton rb = mRouterTypes.findViewById(buttonId);
    listener.onCheckedChanged(rb, false);
    rb.setOnCheckedChangeListener(listener);
    rb.setOnClickListener(clickListener);
  }

  private void setupRouterButtons()
  {
    setupRouterButton(R.id.vehicle2, R.drawable.ic_car, this::onVehicleModeSelected);
    setupRouterButton(R.id.pedestrian2, R.drawable.ic_pedestrian, this::onPedestrianModeSelected);
    setupRouterButton(R.id.transit2, R.drawable.ic_transit, this::onTransitModeSelected);
    setupRouterButton(R.id.bicycle2, R.drawable.ic_bike, this::onBicycleModeSelected);
    setupRouterButton(R.id.ruler2, app.organicmaps.sdk.R.drawable.ic_ruler_route, this::onRulerModeSelected);
  }

  private void onTransitModeSelected(@NonNull View v)
  {
    RoutingController.get().setRouterType(Router.Transit);
  }

  private void onBicycleModeSelected(@NonNull View v)
  {
    RoutingController.get().setRouterType(Router.Bicycle);
  }

  private void onRulerModeSelected(@NonNull View v)
  {
    RoutingController.get().setRouterType(Router.Ruler);
  }

  private void onPedestrianModeSelected(@NonNull View v)
  {
    RoutingController.get().setRouterType(Router.Pedestrian);
  }

  private void onVehicleModeSelected(@NonNull View v)
  {
    RoutingController.get().setRouterType(Router.Vehicle);
  }

  private void updateProgressLabels()
  {
    RoutingController.BuildState buildState = RoutingController.get().getBuildState();
    final boolean ready = (buildState == RoutingController.BuildState.BUILT);
    if (!ready)
      return;

    if (isTransitType())
    {
      TransitRouteInfo info = RoutingController.get().getCachedTransitInfo();
      if (info != null)
        mRoutingBottomMenuController.showTransitInfo(info);
      return;
    }

    if (isRulerType())
    {
      RoutingInfo routingInfo = RoutingController.get().getCachedRoutingInfo();
      if (routingInfo != null)
        mRoutingBottomMenuController.showRulerInfo(Framework.nativeGetRoutePoints(), routingInfo.distToTarget);
      return;
    }

    // Ruler routes returned early above, so the start button is always shown at this point.
    mRoutingBottomMenuController.setStartButton(true);
    mRoutingBottomMenuController.showAltitudeChartAndRoutingDetails();
  }

  private void updateBadgeCount(int count)
  {
    if (count > 0)
    {
      UiUtils.show(mDrivingOptionsBadge);
      mDrivingOptionsBadge.setText(String.valueOf(count));
    }
    else
    {
      UiUtils.hide(mDrivingOptionsBadge);
    }
  }

  private void updateBuildProgress(int progress, @NonNull Router router)
  {
    if (getView() == null)
      return;

    UiUtils.invisible(mProgressVehicle, mProgressPedestrian, mProgressTransit, mProgressBicycle, mProgressRuler);
    WheelProgressView progressView;
    switch (router)
    {
    case Vehicle:
      mRouterTypes.check(R.id.vehicle2);
      progressView = mProgressVehicle;
      break;
    case Pedestrian:
      mRouterTypes.check(R.id.pedestrian2);
      progressView = mProgressPedestrian;
      break;
    case Transit:
      mRouterTypes.check(R.id.transit2);
      progressView = mProgressTransit;
      break;
    case Bicycle:
      mRouterTypes.check(R.id.bicycle2);
      progressView = mProgressBicycle;
      break;
    case Ruler:
      mRouterTypes.check(R.id.ruler2);
      progressView = mProgressRuler;
      break;
    default: throw new IllegalArgumentException("unknown router: " + router);
    }

    int bt = mRouterTypes.getCheckedRadioButtonId();
    RoutingToolbarButton button = mRouterTypes.findViewById(bt);
    if (button == null)
      return;

    button.progress();
    updateProgressLabels();

    if (!RoutingController.get().isBuilding())
    {
      button.complete();
      return;
    }

    UiUtils.show(progressView);
    progressView.setPending(progress == 0);
    if (progress != 0)
      progressView.setProgress(progress);
  }

  private boolean isTransitType()
  {
    return RoutingController.get().isTransitType();
  }

  private boolean isRulerType()
  {
    return RoutingController.get().isRulerRouterType();
  }

  @Override
  public void onSaveInstanceState(@NonNull Bundle outState)
  {
    super.onSaveInstanceState(outState);
    outState.putInt(TAG + "_bottom_sheet_state", mViewModel.getBottomSheetState());
    if (mRoutingBottomMenuController != null)
      mRoutingBottomMenuController.saveRoutingPanelState(outState);
  }

  private void restoreRoutingPanelState(@NonNull Bundle state)
  {
    mViewModel.setBottomSheetState(state.getInt(TAG + "_bottom_sheet_state", BottomSheetBehavior.STATE_COLLAPSED));
    if (mRoutingBottomMenuController != null)
      mRoutingBottomMenuController.restoreRoutingPanelState(state);
    updateBadgeCount(SharedPropertiesUtils.getDrivingOptionsCount());
  }

  private void showAddStartFrame()
  {
    if (mRoutingBottomMenuController != null)
      mRoutingBottomMenuController.showAddStartFrame();
  }

  private void showAddFinishFrame()
  {
    if (mRoutingBottomMenuController != null)
      mRoutingBottomMenuController.showAddFinishFrame();
  }

  private void hideActionFrame()
  {
    if (mRoutingBottomMenuController != null)
      mRoutingBottomMenuController.hideActionFrame();
  }

  @Override
  public void onLayoutChange(View v, int left, int top, int right, int bottom, int oldLeft, int oldTop, int oldRight,
                             int oldBottom)
  {
    updateMapButtonsOffset();
  }

  @Override
  public void onUseMyPositionAsStart()
  {
    RoutingController.get().setStartPoint(MwmApplication.from(requireContext()).getLocationHelper().getMyPosition());
  }

  @Override
  public void onSearchRoutePoint(@NonNull RouteMarkType pointType)
  {
    RoutingController.get().waitForPoiPick(pointType);
    SearchActivity.start(requireActivity(), "");
  }

  @Override
  public void onRoutingStart()
  {
    if (!mRoutingPlanController.showStartPointNotice())
    {
      mRoutingPlanController.setFullscreen(false);
      return;
    }

    if (!mRoutingPlanController.showRoutingDisclaimer())
      return;

    mRoutingPlanController.closeFloatingPanels();
    mRoutingPlanController.setFullscreen(false);
    RoutingController.get().start();
  }

  private void onDrivingOptionsBuildError()
  {
    new MaterialAlertDialogBuilder(requireContext(), R.style.MwmTheme_AlertDialog)
        .setTitle(R.string.unable_to_calc_alert_title)
        .setMessage(R.string.unable_to_calc_alert_subtitle)
        .setPositiveButton(
            R.string.settings,
            (dialog, which) -> DrivingOptionsActivity.start(requireActivity(), startDrivingOptionsForResult))
        .setNegativeButton(R.string.cancel, null)
        .show();
  }
}
