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
import app.organicmaps.sdk.routing.RoutingOptions;
import app.organicmaps.sdk.routing.TransitRouteInfo;
import app.organicmaps.settings.DrivingOptionsActivity;
import app.organicmaps.util.UiUtils;
import app.organicmaps.widget.RoutingToolbarButton;
import app.organicmaps.widget.WheelProgressView;
import com.google.android.material.bottomsheet.BottomSheetBehavior;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;

public class RoutingPlanFragment extends Fragment implements View.OnLayoutChangeListener, RoutingBottomMenuListener
{
  public static final String TAG = RoutingPlanFragment.class.getSimpleName();

  private RoutingPlanViewModel mViewModel;
  private View mChartPanel;
  private ChartHeaderAdapter mChartHeaderAdapter;
  private View mDrivingOptionsBtn;
  private View mFrame;
  private View mRoutingRoot;
  private View mRoutingBottomContainer;
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
          mViewModel.setDrivingOptionsCount(RoutingOptions.getActiveRoadTypes().size());
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
  private final Observer<Void> mDrivingOptionsErrorObserver = e -> onDrivingOptionsBuildError();

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
    mChartPanel = inflater.inflate(R.layout.altitude_chart_panel, container, false);
    return inflater.inflate(R.layout.routing_bottom_sheet, container, false);
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    mViewModel = new ViewModelProvider(requireActivity()).get(RoutingPlanViewModel.class);
    mFrame = view.findViewById(R.id.routing_sheet_frame);
    mRoutingTypesContainer = mFrame.findViewById(R.id.routing_types_frame);
    mRouterTypes = mFrame.findViewById(R.id.route_type);
    mPeekHeightMargins = getResources().getDimensionPixelSize(R.dimen.routing_margin_peek_height);
    mBottomButtonsMaxHeight = getResources().getDimensionPixelSize(R.dimen.routing_bottom_buttons_max_height);

    setupRouterButtons();

    mProgressFrame = mFrame.findViewById(R.id.progress_frame);
    mProgressVehicle = mProgressFrame.findViewById(R.id.progress_vehicle);
    mProgressPedestrian = mProgressFrame.findViewById(R.id.progress_pedestrian);
    mProgressTransit = mProgressFrame.findViewById(R.id.progress_transit);
    mProgressBicycle = mProgressFrame.findViewById(R.id.progress_bicycle);
    mProgressRuler = mProgressFrame.findViewById(R.id.progress_ruler);

    mChartHeaderAdapter = new ChartHeaderAdapter(mChartPanel);
    mActionFrame = view.findViewById(R.id.routing_action_frame);
    mRoutingContainer = requireActivity().findViewById(R.id.routing_container);
    mButtonsLayout = view.findViewById(R.id.routing_bottom_buttons);

    mRoutingBottomMenuController =
        RoutingBottomMenuController.newInstance(requireActivity(), mFrame, mChartPanel, mChartHeaderAdapter, this);
    mRoutingBottomMenuController.setVisibilityChangedCallback(this::updateMapButtonsOffset);
    mRoutingRoot = view.findViewById(R.id.routing_root);
    mRoutingBottomContainer = view.findViewById(R.id.routing_bottom_container);

    mDrivingOptionsBadge = mChartPanel.findViewById(R.id.driving_options_badge);
    mDrivingOptionsBtn = mChartPanel.findViewById(R.id.driving_options_btn_img);
    mDrivingOptionsBtn.setOnClickListener(
        v -> DrivingOptionsActivity.start(requireActivity(), startDrivingOptionsForResult));

    mSearchBtn = mRoutingRoot.findViewById(R.id.routing_btn_search);
    mBookmarkBtn = mButtonsLayout.findViewById(R.id.routing_btn_bookmarks);
    mSearchBtn.setOnClickListener(
        v -> mMapButtonClickListener.onMapButtonClick(MapButtonsController.MapButtons.search));
    mBookmarkBtn.setOnClickListener(
        v -> mMapButtonClickListener.onMapButtonClick(MapButtonsController.MapButtons.bookmarks));

    final View closeButton = mRoutingTypesContainer.findViewById(R.id.back);
    closeButton.setOnClickListener(v -> mRoutingPlanController.handleBackPress());

    setInsets();
    setupBottomSheetBehavior();

    mSheetVisible.addSource(mViewModel.getShowRoutingBottomSheet(), show -> updateSheetVisible());
    mSheetVisible.addSource(mViewModel.getIsPlacePageActive(), active -> updateSheetVisible());
    mSheetVisible.addSource(mViewModel.getIsSearchActive(), active -> updateSheetVisible());
    mSheetVisible.observe(getViewLifecycleOwner(), this::showSheet);
    mViewModel.getIsSearchActive().observe(getViewLifecycleOwner(), active -> reflectActionFrame());
    mViewModel.getIsPlacePageActive().observe(getViewLifecycleOwner(), active -> reflectActionFrame());
    mViewModel.getMenuUpdateTrigger().observe(getViewLifecycleOwner(), mMenuUpdateObserver);
    mViewModel.getBuildProgress().observe(getViewLifecycleOwner(), mBuildProgressObserver);
    mViewModel.getDrivingOptionsCount().observe(getViewLifecycleOwner(), mDrivingOptionsCountObserver);
    mViewModel.getDrivingOptionsError().observe(getViewLifecycleOwner(), mDrivingOptionsErrorObserver);

    if (savedInstanceState != null)
      restoreRoutingPanelState(savedInstanceState);

    updateBadgeCount(RoutingOptions.getActiveRoadTypes().size());
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
      mRoutingRoot.setPadding(0, mTopInset, 0, 0);
      if (mRoutingBottomContainer != null)
        mRoutingBottomContainer.setPadding(leftInset, mRoutingBottomContainer.getPaddingTop(), rightInset, 0);
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
    mSheetBehavior.setShouldRemoveExpandedCorners(false);
    mSheetBehavior.addBottomSheetCallback(new BottomSheetBehavior.BottomSheetCallback() {
      @Override
      public void onStateChanged(@NonNull View bottomSheet, int newState)
      {
        if (newState != BottomSheetBehavior.STATE_SETTLING && newState != BottomSheetBehavior.STATE_DRAGGING
            && newState != BottomSheetBehavior.STATE_HIDDEN)
          mViewModel.setBottomSheetState(newState);
        if (newState == BottomSheetBehavior.STATE_HIDDEN)
          UiUtils.hide(mButtonsLayout);
      }

      @Override
      public void onSlide(@NonNull View bottomSheet, float slideOffset)
      {
        mViewModel.setRoutingBottomDistanceToTop(bottomSheet.getTop());
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
    {
      setRoutingContentActive(false);
      mRoutingBottomMenuController.refreshManageRoute();
    }

    if (controller.isBuilt())
      setRoutingContentActive(true);

    if (controller.isPlanning())
      reflectActionFrame(controller);
  }

  private void reflectActionFrame()
  {
    final RoutingController controller = RoutingController.get();
    if (controller.isPlanning())
      reflectActionFrame(controller);
  }

  private void reflectActionFrame(@NonNull RoutingController controller)
  {
    final boolean missing = controller.getStartPoint() == null || controller.getEndPoint() == null;
    if (missing && !isPlannerOverlayed())
      showAddStartOrFinishFrame(controller);
    else
      hideActionFrame();
    mViewModel.setShowRoutingBottomSheet(!missing);
  }

  private boolean isPlannerOverlayed()
  {
    return Boolean.TRUE.equals(mViewModel.getIsSearchActive().getValue())
 || Boolean.TRUE.equals(mViewModel.getIsPlacePageActive().getValue());
  }

  private void showAddStartOrFinishFrame(@NonNull RoutingController controller)
  {
    MapObject myPosition = MwmApplication.from(requireContext()).getLocationHelper().getMyPosition();

    if (myPosition != null && controller.getEndPoint() == null)
    {
      showAddFinishFrame();
      return;
    }
    if (controller.getStartPoint() == null)
    {
      showAddStartFrame();
      return;
    }
    if (controller.getEndPoint() == null)
      showAddFinishFrame();
  }

  private void updateSheetVisible()
  {
    final boolean show = Boolean.TRUE.equals(mViewModel.getShowRoutingBottomSheet().getValue());
    final boolean placePageActive = Boolean.TRUE.equals(mViewModel.getIsPlacePageActive().getValue());
    final boolean searchActive = Boolean.TRUE.equals(mViewModel.getIsSearchActive().getValue());
    mSheetVisible.setValue(show && !placePageActive && !searchActive);
  }

  private void showSheet(boolean show)
  {
    if (show)
    {
      UiUtils.show(mButtonsLayout, mFrame);
      mFrame.post(() -> {
        // The view may be destroyed, or visibility may have flipped back to hidden (e.g. a place page
        // opened), before this runs; bail out instead of reopening the sheet over whatever is on top.
        if (getView() == null || !Boolean.TRUE.equals(mSheetVisible.getValue()))
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
    }
  }

  private void setRoutingContentActive(boolean active)
  {
    if (active)
    {
      mDrivingOptionsBtn.setEnabled(true);
      mChartPanel.setAlpha(1.0f);
    }
    else
    {
      mDrivingOptionsBtn.setEnabled(false);
      mChartPanel.setAlpha(0.2f);
    }
  }

  public void updateMapButtonsOffset()
  {
    if (mFrame.getTop() == 0)
      return;
    // mChartPanel lives inside the route list (ConcatAdapter header), so it reports a real height once the
    // RecyclerView has measured its first pass. Falling back to 0 keeps peek-height sensible on early calls.
    final int chartHeight = mChartPanel.getHeight();
    final int newPeekHeight =
        mRoutingTypesContainer.getHeight() + chartHeight + mBottomButtonsMaxHeight + mPeekHeightMargins;
    mSheetBehavior.setPeekHeight(newPeekHeight);
    final int newDistanceTop = mFrame.getTop();
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
    setupRouterButton(R.id.vehicle, R.drawable.ic_car, selectRouter(Router.Vehicle));
    setupRouterButton(R.id.pedestrian, R.drawable.ic_pedestrian, selectRouter(Router.Pedestrian));
    setupRouterButton(R.id.transit, R.drawable.ic_transit, selectRouter(Router.Transit));
    setupRouterButton(R.id.bicycle, R.drawable.ic_bike, selectRouter(Router.Bicycle));
    setupRouterButton(R.id.ruler, app.organicmaps.sdk.R.drawable.ic_ruler_route, selectRouter(Router.Ruler));
  }

  @NonNull
  private static View.OnClickListener selectRouter(@NonNull Router router)
  {
    return v -> RoutingController.get().setRouterType(router);
  }

  @IdRes
  private static int routerToButtonId(@NonNull Router router)
  {
    return switch (router)
    {
      case Vehicle -> R.id.vehicle;
      case Pedestrian -> R.id.pedestrian;
      case Transit -> R.id.transit;
      case Bicycle -> R.id.bicycle;
      case Ruler -> R.id.ruler;
    };
  }

  @NonNull
  private WheelProgressView routerToProgressView(@NonNull Router router)
  {
    return switch (router)
    {
      case Vehicle -> mProgressVehicle;
      case Pedestrian -> mProgressPedestrian;
      case Transit -> mProgressTransit;
      case Bicycle -> mProgressBicycle;
      case Ruler -> mProgressRuler;
    };
  }

  private void updateProgressLabels()
  {
    RoutingController.BuildState buildState = RoutingController.get().getBuildState();
    final boolean ready = (buildState == RoutingController.BuildState.BUILT);
    if (!ready)
    {
      mRoutingBottomMenuController.hideAltitudeChartAndRoutingDetails();
      return;
    }

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
    mRouterTypes.check(routerToButtonId(router));
    final WheelProgressView progressView = routerToProgressView(router);

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
    updateBadgeCount(RoutingOptions.getActiveRoadTypes().size());
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
    mMapButtonClickListener.onMapButtonClick(MapButtonsController.MapButtons.search);
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
