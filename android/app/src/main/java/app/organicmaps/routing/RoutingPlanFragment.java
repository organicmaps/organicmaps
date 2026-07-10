package app.organicmaps.routing;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.RadioGroup;
import android.widget.TextView;
import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
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
import app.organicmaps.R;
import app.organicmaps.maplayer.MapButtonsController;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.Router;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.routing.RoutingInfo;
import app.organicmaps.sdk.routing.RoutingOptions;
import app.organicmaps.sdk.routing.TransitRouteInfo;
import app.organicmaps.settings.DrivingOptionsActivity;
import app.organicmaps.util.UiUtils;
import com.google.android.material.bottomsheet.BottomSheetBehavior;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;

public class RoutingPlanFragment extends Fragment implements View.OnLayoutChangeListener, RoutingBottomMenuListener
{
  public static final String TAG = RoutingPlanFragment.class.getSimpleName();

  private static final Router[] ROUTERS = Router.values();

  private RoutingPlanViewModel mViewModel;
  private View mChartPanel;
  private View mTransitStepsView;
  private ChartHeaderAdapter mChartHeaderAdapter;
  private View mDrivingOptionsBtn;
  private View mFrame;
  private View mRoutingRoot;
  private View mRoutingBottomContainer;
  private RadioGroup mRouterTypes;
  private RoutingBottomMenuController mRoutingBottomMenuController;
  private TextView mDrivingOptionsBadge;
  private View mSearchBtn;
  private View mBookmarkBtn;
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
      updateBuildProgress(progress[0], ROUTERS[progress[1]]);
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

    mChartHeaderAdapter = new ChartHeaderAdapter(mChartPanel);
    mRoutingContainer = requireActivity().findViewById(R.id.routing_container);
    mButtonsLayout = view.findViewById(R.id.routing_bottom_buttons);

    mRoutingBottomMenuController =
        RoutingBottomMenuController.newInstance(requireActivity(), mFrame, mChartPanel, mChartHeaderAdapter, this);
    mRoutingBottomMenuController.setVisibilityChangedCallback(this::updateSheetLayout);
    mRoutingRoot = view.findViewById(R.id.routing_root);
    mRoutingBottomContainer = view.findViewById(R.id.routing_bottom_container);

    mTransitStepsView = mChartPanel.findViewById(R.id.transit_recycler_view);
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
      mViewModel.setShowRoutingBottomSheet(true);
    }

    if (controller.isBuilt())
      setRoutingContentActive(true);
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

  public void updateSheetLayout()
  {
    if (mFrame.getTop() == 0)
      return;
    updateSheetHeights();
    final int newDistanceTop = mFrame.getTop();
    mViewModel.setRoutingBottomDistanceToTop(newDistanceTop);
  }

  private void updateSheetHeights()
  {
    final View parent = (View) mFrame.getParent();
    final int parentHeight = parent == null ? 0 : parent.getHeight();
    if (parentHeight > 0)
      mSheetBehavior.setMaxHeight(parentHeight);
    // Peek excludes the scrollable steps/list: it ends at the steps' top (or the full header when there are
    // none), while setMaxHeight caps a long list to scroll inside the sheet instead of covering the map.
    final boolean stepsVisible = mTransitStepsView.getVisibility() == View.VISIBLE;
    final int chartHeader = stepsVisible ? mTransitStepsView.getTop() : mChartPanel.getHeight();
    final int peekHeight =
        mRoutingTypesContainer.getHeight() + chartHeader + mBottomButtonsMaxHeight + mPeekHeightMargins;
    mSheetBehavior.setPeekHeight(peekHeight);
  }

  private void setupRouterButtons()
  {
    setRouterClick(R.id.vehicle, Router.Vehicle);
    setRouterClick(R.id.pedestrian, Router.Pedestrian);
    setRouterClick(R.id.transit, Router.Transit);
    setRouterClick(R.id.bicycle, Router.Bicycle);
    setRouterClick(R.id.ruler, Router.Ruler);
  }

  private void setRouterClick(@IdRes int buttonId, @NonNull Router router)
  {
    mRouterTypes.findViewById(buttonId).setOnClickListener(v -> RoutingController.get().setRouterType(router));
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

  private void updateProgressLabels()
  {
    final RoutingController controller = RoutingController.get();
    if (controller.getBuildState() != RoutingController.BuildState.BUILT)
    {
      mRoutingBottomMenuController.hideAltitudeChartAndRoutingDetails();
      return;
    }

    if (controller.isTransitType())
    {
      TransitRouteInfo info = controller.getCachedTransitInfo();
      if (info != null)
        mRoutingBottomMenuController.showTransitInfo(info);
      return;
    }

    if (controller.isRulerRouterType())
    {
      RoutingInfo routingInfo = controller.getCachedRoutingInfo();
      if (routingInfo != null)
        mRoutingBottomMenuController.showRulerInfo(Framework.nativeGetRoutePoints(), routingInfo.distToTarget);
      return;
    }

    mRoutingBottomMenuController.setStartState(RoutingBottomMenuController.StartState.ENABLED);
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

    mRouterTypes.check(routerToButtonId(router));
    updateProgressLabels();
    final RoutingController controller = RoutingController.get();
    if (controller.isBuilding())
    {
      mRoutingBottomMenuController.setStartState(RoutingBottomMenuController.StartState.BUILDING);
      mRoutingBottomMenuController.setBuildProgress(progress);
    }
    else if (!controller.isBuilt())
      // ERROR / NONE / cancelled: clear the progress fill that BUILDING left behind.
      mRoutingBottomMenuController.setStartState(RoutingBottomMenuController.StartState.DISABLED);
  }

  public void onBuildStarted()
  {
    if (mRoutingBottomMenuController != null)
      mRoutingBottomMenuController.resetBuildProgress();
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

  @Override
  public void onLayoutChange(View v, int left, int top, int right, int bottom, int oldLeft, int oldTop, int oldRight,
                             int oldBottom)
  {
    updateSheetLayout();
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
