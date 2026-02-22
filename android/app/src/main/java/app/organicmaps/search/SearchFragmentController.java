package app.organicmaps.search;

import android.annotation.SuppressLint;
import android.content.res.ColorStateList;
import android.content.res.Configuration;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.lifecycle.Observer;
import androidx.lifecycle.ViewModelProvider;
import app.organicmaps.R;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.util.ThemeUtils;
import app.organicmaps.widget.placepage.PlacePageUtils;
import app.organicmaps.widget.placepage.PlacePageViewModel;
import com.google.android.material.bottomsheet.BottomSheetBehavior;
import com.google.android.material.color.MaterialColors;
import com.google.android.material.shape.CornerFamily;
import com.google.android.material.shape.MaterialShapeDrawable;
import com.google.android.material.shape.ShapeAppearanceModel;
import java.util.concurrent.atomic.AtomicReference;

public class SearchFragmentController extends Fragment implements SearchFragment.SearchFragmentListener
{
  BottomSheetBehavior<FrameLayout> mFrameLayoutBottomSheetBehavior;
  private ViewGroup mCoordinator;
  private WindowInsetsCompat mCurrentWindowInsets;
  @Nullable
  private View mRoutingPlanFrame;
  FrameLayout mSearchPageContainer;
  int mDistanceToTop;
  int mViewportMinHeight;
  SearchPageViewModel mViewModel;
  PlacePageViewModel mPlacePageViewModel;
  // These variables are used to determine if the touch event is a tap or a drag
  static AtomicReference<Float> mInitialX = new AtomicReference<>((float) 0);
  static AtomicReference<Float> mInitialY = new AtomicReference<>((float) 0);
  private boolean mHiddenByPlacePage = false;

  private final BottomSheetBehavior.BottomSheetCallback mDefaultBottomSheetCallback =
      new BottomSheetBehavior.BottomSheetCallback() {
        @Override
        public void onStateChanged(@NonNull View bottomSheet, int newState)
        {
          if (PlacePageUtils.isSettlingState(newState) || PlacePageUtils.isDraggingState(newState))
            return;

          PlacePageUtils.updateMapViewport(mCoordinator, mDistanceToTop, mViewportMinHeight);

          if (PlacePageUtils.isHiddenState(newState))
          {
            if (mHiddenByPlacePage)
            {
              mHiddenByPlacePage = false;
            }
            else if (mPlacePageViewModel.getMapObject().getValue() == null)
            {
              mViewModel.setSearchEnabled(false, null);
            }
          }
          // we do not save the state if search page is hiding
          if (newState == BottomSheetBehavior.STATE_EXPANDED || newState == BottomSheetBehavior.STATE_HALF_EXPANDED
              || newState == BottomSheetBehavior.STATE_COLLAPSED)
            mViewModel.setSearchPageLastState(newState);
          else if (newState != BottomSheetBehavior.STATE_HIDDEN)
          {
            mViewModel.setSearchPageLastState(BottomSheetBehavior.STATE_HALF_EXPANDED);
          }
        }

        @Override
        public void onSlide(@NonNull View bottomSheet, float slideOffset)
        {
          //          stopCustomPeekHeightAnimation();
          mDistanceToTop = bottomSheet.getTop();
          mViewModel.setSearchPageDistanceToTop(mDistanceToTop);
        }
      };

  private final Observer<Boolean> mSearchPageEnabledObserver = new Observer<>() {
    @Override
    public void onChanged(Boolean enabled)
    {
      if (enabled != null && enabled)
      {
        // Don't show search if place page is currently visible
        if (mPlacePageViewModel.getMapObject().getValue() != null)
          return;
        if (mFrameLayoutBottomSheetBehavior.getState() == BottomSheetBehavior.STATE_HIDDEN)
        {
          mFrameLayoutBottomSheetBehavior.setState(BottomSheetBehavior.STATE_EXPANDED);
          mSearchPageContainer.post(SearchFragmentController.this::activateSearchToolbar);
        }
      }
    }
  };

  private final Observer<MapObject> mPlacePageMapObjectObserver = new Observer<>() {
    @Override
    public void onChanged(MapObject mapObject)
    {
      if (mapObject != null)
      {
        // Hide search when place page is opened
        if (mFrameLayoutBottomSheetBehavior.getState() != BottomSheetBehavior.STATE_HIDDEN)
        {
          mHiddenByPlacePage = true;
          mViewModel.setSearchPageLastState(mFrameLayoutBottomSheetBehavior.getState());
          mFrameLayoutBottomSheetBehavior.setState(BottomSheetBehavior.STATE_HIDDEN);
        }
      }
      else
      {
        // Only restore search page if search is actually enabled
        Boolean searchEnabled = mViewModel.getSearchEnabled().getValue();
        Integer lastState = mViewModel.getSearchPageLastState().getValue();
        if (searchEnabled != null && searchEnabled && lastState != null
            && lastState != BottomSheetBehavior.STATE_HIDDEN)
          mFrameLayoutBottomSheetBehavior.setState(lastState);
      }
    }
  };

  private final Observer<Integer> mToolbarHeightObserver = new Observer<>() {
    @Override
    public void onChanged(Integer height)
    {
      if (height != null && height > 0)
        mFrameLayoutBottomSheetBehavior.setPeekHeight(height);
    }
  };

  private final View.OnTouchListener mMapTouchListener = new View.OnTouchListener() {
    @SuppressLint("ClickableViewAccessibility")
    @Override
    public boolean onTouch(View v, MotionEvent event)
    {
      if (!isDrag(event))
        return false;
      if (mFrameLayoutBottomSheetBehavior.getState() == BottomSheetBehavior.STATE_SETTLING)
        return false;
      if (mFrameLayoutBottomSheetBehavior.getState() == BottomSheetBehavior.STATE_EXPANDED
          || mFrameLayoutBottomSheetBehavior.getState() == BottomSheetBehavior.STATE_HALF_EXPANDED)
        mFrameLayoutBottomSheetBehavior.setState(BottomSheetBehavior.STATE_COLLAPSED);
      return false;
    }
  };

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.search_fragment_container, container, false);
  }

  @SuppressLint("ClickableViewAccessibility")
  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    mViewModel = new ViewModelProvider(requireActivity()).get(SearchPageViewModel.class);
    mPlacePageViewModel = new ViewModelProvider(requireActivity()).get(PlacePageViewModel.class);

    mCoordinator = requireActivity().findViewById(R.id.coordinator);
    mRoutingPlanFrame = requireActivity().findViewById(R.id.routing_plan_frame);
    mViewportMinHeight = requireActivity().getResources().getDimensionPixelSize(R.dimen.viewport_min_height);

    mSearchPageContainer = view.findViewById(R.id.search_page_container);

    float topRadius = TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 30, getResources().getDisplayMetrics());
    ShapeAppearanceModel shape = ShapeAppearanceModel.builder()
                                     .setTopLeftCorner(CornerFamily.ROUNDED, topRadius)
                                     .setTopRightCorner(CornerFamily.ROUNDED, topRadius)
                                     .setBottomLeftCorner(CornerFamily.ROUNDED, 0f)
                                     .setBottomRightCorner(CornerFamily.ROUNDED, 0f)
                                     .build();
    MaterialShapeDrawable background = new MaterialShapeDrawable(shape);
    int surface = MaterialColors.getColor(mSearchPageContainer, com.google.android.material.R.attr.colorSurface);
    background.setFillColor(ColorStateList.valueOf(surface));
    mSearchPageContainer.setBackground(background);
    mSearchPageContainer.setClipToOutline(true);

    mFrameLayoutBottomSheetBehavior = BottomSheetBehavior.from(mSearchPageContainer);

    DisplayMetrics dm = getResources().getDisplayMetrics();
    if (getResources().getConfiguration().orientation == Configuration.ORIENTATION_LANDSCAPE)
      adjustSearchContainerWidthForLandscape(dm);

    mFrameLayoutBottomSheetBehavior.setFitToContents(false);
    // Peek height will be set dynamically when toolbar height is measured
    mFrameLayoutBottomSheetBehavior.setHalfExpandedRatio(0.5f); // mid
    mFrameLayoutBottomSheetBehavior.setHideable(true);
    mFrameLayoutBottomSheetBehavior.setDraggable(true);

    // Restore search page state after configuration change (e.g., rotation)
    // Only restore if search is enabled AND place page is not visible
    if (savedInstanceState != null)
    {
      Boolean searchEnabled = mViewModel.getSearchEnabled().getValue();
      MapObject mapObject = mPlacePageViewModel.getMapObject().getValue();
      Integer lastState = mViewModel.getSearchPageLastState().getValue();
      if (searchEnabled != null && searchEnabled && mapObject == null && lastState != null
          && lastState != BottomSheetBehavior.STATE_HIDDEN)
      {
        mFrameLayoutBottomSheetBehavior.setState(lastState);
        // Only activate the search toolbar if keyboard was visible before rotation
        if (mViewModel.isKeyboardVisible())
          mSearchPageContainer.post(this::activateSearchToolbar);
      }
      else
        mFrameLayoutBottomSheetBehavior.setState(BottomSheetBehavior.STATE_HIDDEN);
    }
    else
      mFrameLayoutBottomSheetBehavior.setState(BottomSheetBehavior.STATE_HIDDEN);

    mSearchPageContainer.addOnLayoutChangeListener((v, l, t, r, b, ol, ot, or, ob) -> updateExpandedOffset());
    mSearchPageContainer.post(this::updateExpandedOffset);
    if (mRoutingPlanFrame != null)
      mRoutingPlanFrame.addOnLayoutChangeListener((v, l, t, r, b, ol, ot, or, ob) -> updateExpandedOffset());

    ViewCompat.setOnApplyWindowInsetsListener(view, (v, insets) -> {
      mCurrentWindowInsets = insets;
      boolean imeVisible = insets.isVisible(WindowInsetsCompat.Type.ime());
      // Track keyboard visibility in ViewModel for persistence across rotation
      mViewModel.setKeyboardVisible(imeVisible);
      if (imeVisible && mFrameLayoutBottomSheetBehavior.getState() != BottomSheetBehavior.STATE_EXPANDED)
        mFrameLayoutBottomSheetBehavior.setState(BottomSheetBehavior.STATE_EXPANDED);
      updateExpandedOffset();
      return insets;
    });

    // Set touch listener on map view to handle drag events
    SurfaceView mapView = requireActivity().findViewById(R.id.map);
    mapView.setOnTouchListener(mMapTouchListener);

    final FragmentManager fm = getChildFragmentManager();
    if (savedInstanceState == null && fm.findFragmentByTag("SearchPageFragment") == null)
    {
      fm.beginTransaction()
          .setReorderingAllowed(true)
          .addToBackStack(null)
          .replace(R.id.search_fragment, SearchFragment.class, null, "SearchPageFragment")
          .commit();
    }
  }

  @Override
  public void onStart()
  {
    super.onStart();
    mPlacePageViewModel.getMapObject().observe(getViewLifecycleOwner(), mPlacePageMapObjectObserver);
    mFrameLayoutBottomSheetBehavior.addBottomSheetCallback(mDefaultBottomSheetCallback);
    mViewModel.getSearchEnabled().observe(getViewLifecycleOwner(), mSearchPageEnabledObserver);
    mViewModel.getToolbarHeight().observe(getViewLifecycleOwner(), mToolbarHeightObserver);
  }

  @Override
  public void onStop()
  {
    super.onStop();
    mFrameLayoutBottomSheetBehavior.removeBottomSheetCallback(mDefaultBottomSheetCallback);
  }

  @Override
  public void onDestroy()
  {
    super.onDestroy();
  }

  private void updateExpandedOffset()
  {
    if (mFrameLayoutBottomSheetBehavior == null || mSearchPageContainer == null)
      return;

    int topInset =
        mCurrentWindowInsets != null ? mCurrentWindowInsets.getInsets(WindowInsetsCompat.Type.systemBars()).top : 0;
    int routingHeaderHeight = 0;
    if (mRoutingPlanFrame != null && mRoutingPlanFrame.getVisibility() == View.VISIBLE)
      routingHeaderHeight = mRoutingPlanFrame.getHeight();
    if (routingHeaderHeight == 0 && RoutingController.get().isPlanning())
      routingHeaderHeight = (int) getResources().getDimension(
          ThemeUtils.getResource(requireContext(), androidx.appcompat.R.attr.actionBarSize));

    int expandedOffset = topInset + routingHeaderHeight;
    mFrameLayoutBottomSheetBehavior.setExpandedOffset(Math.max(expandedOffset, 0));
  }

  /**
   * Adjusts the search container width for landscape orientation.
   * On screens with sufficient width, sets the width to 60% of screen width.
   * On smaller screens, uses full width to ensure content fits properly.
   */
  private void adjustSearchContainerWidthForLandscape(DisplayMetrics dm)
  {
    float screenWidthDp = dm.widthPixels / dm.density;

    // Apply 60% width restriction only on screens wide enough (600dp+)
    // This ensures content doesn't get clipped on smaller devices in landscape
    if (screenWidthDp >= 600)
    {
      ViewGroup.LayoutParams lp = mSearchPageContainer.getLayoutParams();
      lp.width = (int) (dm.widthPixels * 0.6f);
      mSearchPageContainer.setLayoutParams(lp);
    }
  }

  boolean isDrag(MotionEvent event)
  {
    return switch (event.getAction())
    {
      case MotionEvent.ACTION_DOWN ->
      {
        mInitialX.set(event.getX());
        mInitialY.set(event.getY());
        yield false;
      }
      case MotionEvent.ACTION_UP -> false;
      case MotionEvent.ACTION_MOVE ->
      {
        float dx = Math.abs(event.getX() - mInitialX.get());
        float dy = Math.abs(event.getY() - mInitialY.get());
        yield !(dx < 50) || !(dy < 50);
      }
      default -> false;
    };
  }

  @Override
  public boolean getBackPressedCallback()
  {
    // If search page is visible, hide it and consume the back press
    if (mFrameLayoutBottomSheetBehavior.getState() != BottomSheetBehavior.STATE_HIDDEN)
    {
      mFrameLayoutBottomSheetBehavior.setState(BottomSheetBehavior.STATE_HIDDEN);
      return true;
    }
    // Search page is already hidden, let activity handle the back press
    return false;
  }

  @Override
  public void onSearchClicked()
  {
    mFrameLayoutBottomSheetBehavior.setState(BottomSheetBehavior.STATE_HALF_EXPANDED);
  }

  /**
   * Activates the search toolbar to request focus and show the keyboard.
   * This is called after the search page is shown or restored after rotation.
   */
  private void activateSearchToolbar()
  {
    Fragment fragment = getChildFragmentManager().findFragmentByTag("SearchPageFragment");
    if (fragment instanceof SearchFragment searchFragment)
    {
      // Use postDelayed to ensure the fragment and its views are fully ready
      mSearchPageContainer.postDelayed(searchFragment::activateToolbar, 100);
    }
  }
}
