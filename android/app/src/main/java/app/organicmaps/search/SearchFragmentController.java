package app.organicmaps.search;

import android.annotation.SuppressLint;
import android.content.res.ColorStateList;
import android.content.res.Configuration;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewConfiguration;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.fragment.app.Fragment;
import androidx.lifecycle.Observer;
import androidx.lifecycle.ViewModelProvider;
import app.organicmaps.R;
import app.organicmaps.maplayer.MapButtonsViewModel;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.util.InputUtils;
import app.organicmaps.util.ThemeUtils;
import app.organicmaps.widget.placepage.PlacePageUtils;
import app.organicmaps.widget.placepage.PlacePageViewModel;
import com.google.android.material.bottomsheet.BottomSheetBehavior;
import com.google.android.material.color.MaterialColors;
import com.google.android.material.shape.CornerFamily;
import com.google.android.material.shape.MaterialShapeDrawable;
import com.google.android.material.shape.ShapeAppearanceModel;

public class SearchFragmentController extends Fragment implements SearchFragment.SearchFragmentListener
{
  private BottomSheetBehavior<FrameLayout> mBottomSheetBehavior;
  private final Observer<MapObject> mPlacePageMapObjectObserver = new Observer<>() {
    @Override
    public void onChanged(MapObject mapObject)
    {
      if (mapObject != null)
      {
        // Hide search when place page is opened
        if (mBottomSheetBehavior.getState() != BottomSheetBehavior.STATE_HIDDEN)
        {
          mViewModel.setHiddenByPlacePage(true);
          mViewModel.setSearchPageLastState(mBottomSheetBehavior.getState());
          mBottomSheetBehavior.setState(BottomSheetBehavior.STATE_HIDDEN);
        }
      }
      else
      {
        // Only restore search page if search is actually enabled
        Boolean searchEnabled = mViewModel.getSearchEnabled().getValue();
        Integer lastState = mViewModel.getSearchPageLastState().getValue();
        if (searchEnabled != null && searchEnabled && lastState != null && lastState != BottomSheetBehavior.STATE_HIDDEN
            && mViewModel.isHiddenByPlacePage())
        {
          mBottomSheetBehavior.setState(lastState);
          mViewModel.setHiddenByPlacePage(false);
        }
      }
    }
  };
  private FrameLayout mSearchPageContainer;
  private int mDistanceToTop;
  private int mViewportMinHeight;
  private SearchPageViewModel mViewModel;
  private final Observer<Boolean> mSearchPageEnabledObserver = new Observer<>() {
    @Override
    public void onChanged(Boolean enabled)
    {
      if (enabled == null)
        return;
      if (enabled)
      {
        if (mPlacePageViewModel.getMapObject().getValue() != null && !mViewModel.isHiddenByPlacePage())
          mPlacePageViewModel.setMapObject(null);
        SearchRequest request = mViewModel.getPendingRequest();
        if (request != null && request.mode == SearchRequest.Mode.MAP_ONLY)
          return; // map-only search: don't open or restore the sheet
        if (mViewModel.isHiddenByPlacePage())
          return; // hidden behind the place page (e.g. after recreation); restored when it closes
        Integer lastState = mViewModel.getSearchPageLastState().getValue();
        if (lastState != null && lastState != BottomSheetBehavior.STATE_HIDDEN)
        {
          mBottomSheetBehavior.setState(lastState);
        }
        else if (mBottomSheetBehavior.getState() == BottomSheetBehavior.STATE_HIDDEN)
        {
          mBottomSheetBehavior.setState(BottomSheetBehavior.STATE_EXPANDED);
          mViewModel.setKeyboardVisible(true);
        }
      }
      else
      {
        mBottomSheetBehavior.setState(BottomSheetBehavior.STATE_HIDDEN);
      }
    }
  };
  private PlacePageViewModel mPlacePageViewModel;
  private MapButtonsViewModel mMapButtonsViewModel;
  private ViewGroup mCoordinator;
  private WindowInsetsCompat mCurrentWindowInsets;
  private int mTopHeaderHeight = 0;
  private View mMapView;
  private final BottomSheetBehavior.BottomSheetCallback mDefaultBottomSheetCallback =
      new BottomSheetBehavior.BottomSheetCallback() {
        @Override
        public void onStateChanged(@NonNull View bottomSheet, int newState)
        {
          if (PlacePageUtils.isSettlingState(newState))
            return;
          if (PlacePageUtils.isDraggingState(newState))
          {
            InputUtils.hideKeyboard(bottomSheet);
            return;
          }

          if (!RoutingController.get().isNavigating() && !RoutingController.get().isPlanning())
            PlacePageUtils.updateMapViewport(mCoordinator, mDistanceToTop, mViewportMinHeight);

          if (PlacePageUtils.isHiddenState(newState))
          {
            if (!mViewModel.isHiddenByPlacePage() && mViewModel.getSearchEnabled().getValue() != null
                && mViewModel.getSearchEnabled().getValue())
            {
              mViewModel.setSearchEnabled(false, null);
            }
          }
          // we do not save the state if search page is hiding
          if (newState == BottomSheetBehavior.STATE_EXPANDED || newState == BottomSheetBehavior.STATE_HALF_EXPANDED
              || newState == BottomSheetBehavior.STATE_COLLAPSED)
          {
            mViewModel.setSearchPageLastState(newState);
          }
        }

        @Override
        public void onSlide(@NonNull View bottomSheet, float slideOffset)
        {
          mDistanceToTop = bottomSheet.getTop();
          mViewModel.setSearchPageDistanceToTop(mDistanceToTop);
        }
      };
  // These variables are used to determine if the touch event is a tap or a drag
  private float mInitialX = 0f;
  private float mInitialY = 0f;
  private int mTouchSlop = 0;

  private int mMinCollapsedPeekHeight = 0;
  private final Observer<Integer> mTopHeaderHeightObserver = height ->
  {
    mTopHeaderHeight = height != null ? height : 0;
    updateExpandedOffset();
  };
  private final Observer<Integer> mToolbarHeightObserver = new Observer<>() {
    @Override
    public void onChanged(Integer height)
    {
      if (height != null && height > 0)
        mBottomSheetBehavior.setPeekHeight(Math.max(height, mMinCollapsedPeekHeight));
    }
  };
  private final View.OnTouchListener mMapTouchListener = new View.OnTouchListener() {
    @SuppressLint("ClickableViewAccessibility")
    @Override
    public boolean onTouch(View v, MotionEvent event)
    {
      if (event.getAction() == MotionEvent.ACTION_DOWN)
        InputUtils.hideKeyboard(v);
      boolean drag = isDrag(event);
      if (event.getAction() == MotionEvent.ACTION_UP && !drag)
      {
        v.performClick();
        return false;
      }
      if (!drag)
        return false;
      if (mBottomSheetBehavior.getState() == BottomSheetBehavior.STATE_SETTLING)
        return false;
      if (mBottomSheetBehavior.getState() == BottomSheetBehavior.STATE_EXPANDED
          || mBottomSheetBehavior.getState() == BottomSheetBehavior.STATE_HALF_EXPANDED)
        mBottomSheetBehavior.setState(BottomSheetBehavior.STATE_COLLAPSED);
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
    mMapButtonsViewModel = new ViewModelProvider(requireActivity()).get(MapButtonsViewModel.class);

    mCoordinator = requireActivity().findViewById(R.id.coordinator);
    mViewportMinHeight = requireActivity().getResources().getDimensionPixelSize(R.dimen.viewport_min_height);

    mSearchPageContainer = view.findViewById(R.id.search_page_container);
    mTouchSlop = ViewConfiguration.get(requireContext()).getScaledTouchSlop();

    int actionBarSize = (int) getResources().getDimension(
        ThemeUtils.getResource(requireContext(), androidx.appcompat.R.attr.actionBarSize));
    int tabsHeight = getResources().getDimensionPixelSize(R.dimen.tabs_height);
    mMinCollapsedPeekHeight = actionBarSize + tabsHeight;

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

    mBottomSheetBehavior = BottomSheetBehavior.from(mSearchPageContainer);

    DisplayMetrics dm = getResources().getDisplayMetrics();
    if (getResources().getConfiguration().orientation == Configuration.ORIENTATION_LANDSCAPE)
      adjustSearchContainerWidthForLandscape(dm);

    mBottomSheetBehavior.setFitToContents(false);
    // Peek height will be set dynamically when toolbar height is measured
    mBottomSheetBehavior.setHalfExpandedRatio(0.5f); // mid
    mBottomSheetBehavior.setHideable(true);
    mBottomSheetBehavior.setDraggable(true);
    mBottomSheetBehavior.setState(BottomSheetBehavior.STATE_HIDDEN);

    mSearchPageContainer.post(this::updateExpandedOffset);
    mSearchPageContainer.addOnLayoutChangeListener((v, left, top, right, bottom, oldLeft, oldTop, oldRight,
                                                    oldBottom) -> mViewModel.setSearchPageWidth(right - left));

    ViewCompat.setOnApplyWindowInsetsListener(view, (v, insets) -> {
      mCurrentWindowInsets = insets;
      boolean imeVisible = insets.isVisible(WindowInsetsCompat.Type.ime());
      mViewModel.setKeyboardVisible(imeVisible);
      updateExpandedOffset();
      if (imeVisible && mViewModel.getSearchEnabled().getValue() != null && mViewModel.getSearchEnabled().getValue()
          && !mViewModel.isHiddenByPlacePage())
        mBottomSheetBehavior.setState(BottomSheetBehavior.STATE_EXPANDED);
      // Explicitly dispatch insets into the bottom sheet's content tree so that
      // child views (e.g. tab RecyclerViews created lazily by ViewPager) receive them.
      ViewCompat.dispatchApplyWindowInsets(mSearchPageContainer, insets);
      return insets;
    });

    // Set touch listener on map view to handle drag events
    mMapView = requireActivity().findViewById(R.id.map);
    if (mMapView != null)
    {
      mMapView.setOnTouchListener(mMapTouchListener);
      mMapView.setClickable(true);
    }
  }

  @SuppressLint("ClickableViewAccessibility")
  @Override
  public void onDestroyView()
  {
    super.onDestroyView();
    if (mMapView != null)
    {
      mMapView.setOnTouchListener(null);
      mMapView.setClickable(false);
      mMapView = null;
    }
  }

  @Override
  public void onStart()
  {
    super.onStart();
    mPlacePageViewModel.getMapObject().observe(getViewLifecycleOwner(), mPlacePageMapObjectObserver);
    mBottomSheetBehavior.addBottomSheetCallback(mDefaultBottomSheetCallback);
    mViewModel.getSearchEnabled().observe(getViewLifecycleOwner(), mSearchPageEnabledObserver);
    mViewModel.getToolbarHeight().observe(getViewLifecycleOwner(), mToolbarHeightObserver);
    mMapButtonsViewModel.getTopHeaderHeight().observe(getViewLifecycleOwner(), mTopHeaderHeightObserver);
  }

  @Override
  public void onStop()
  {
    super.onStop();
    mBottomSheetBehavior.removeBottomSheetCallback(mDefaultBottomSheetCallback);
  }

  private void updateExpandedOffset()
  {
    if (mBottomSheetBehavior == null || mSearchPageContainer == null)
      return;
    int topInset =
        mCurrentWindowInsets != null ? mCurrentWindowInsets.getInsets(WindowInsetsCompat.Type.systemBars()).top : 0;
    mBottomSheetBehavior.setExpandedOffset(topInset + mTopHeaderHeight);
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
        mInitialX = event.getX();
        mInitialY = event.getY();
        yield false;
      }
      case MotionEvent.ACTION_UP -> false;
      case MotionEvent.ACTION_MOVE ->
      {
        float dx = Math.abs(event.getX() - mInitialX);
        float dy = Math.abs(event.getY() - mInitialY);
        // Consider it a drag if movement exceeds the platform-scaled touch slop in either axis.
        yield dx >= mTouchSlop || dy >= mTouchSlop;
      }
      default -> false;
    };
  }

  public boolean onBackPressed()
  {
    Fragment fragment = getChildFragmentManager().findFragmentById(R.id.search_fragment);
    if (fragment instanceof SearchFragment searchFragment && searchFragment.onBackPressed())
      return true;

    if (mBottomSheetBehavior.getState() != BottomSheetBehavior.STATE_HIDDEN)
    {
      mBottomSheetBehavior.setState(BottomSheetBehavior.STATE_HIDDEN);
      return true;
    }
    return false;
  }

  @Override
  public void onSearchClicked()
  {
    mBottomSheetBehavior.setState(BottomSheetBehavior.STATE_HALF_EXPANDED);
  }

  @Override
  public void closeSearch()
  {
    mBottomSheetBehavior.setState(BottomSheetBehavior.STATE_HIDDEN);
  }
}
