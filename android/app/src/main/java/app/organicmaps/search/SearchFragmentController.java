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
import app.organicmaps.sdk.util.log.Logger;
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

  private final BottomSheetBehavior.BottomSheetCallback mDefaultBottomSheetCallback =
      new BottomSheetBehavior.BottomSheetCallback() {
        @Override
        public void onStateChanged(@NonNull View bottomSheet, int newState)
        {
          if (PlacePageUtils.isSettlingState(newState) || PlacePageUtils.isDraggingState(newState))
            return;

          PlacePageUtils.updateMapViewport(mCoordinator, mDistanceToTop, mViewportMinHeight);

          if (PlacePageUtils.isHiddenState(newState) && mPlacePageViewModel.getMapObject().getValue() == null)
            mViewModel.setSearchEnabled(false, null);
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
        if (mFrameLayoutBottomSheetBehavior.getState() == BottomSheetBehavior.STATE_HIDDEN)
          mFrameLayoutBottomSheetBehavior.setState(BottomSheetBehavior.STATE_EXPANDED);
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
          mViewModel.setSearchPageLastState(mFrameLayoutBottomSheetBehavior.getState());
          mCoordinator.post(() -> mFrameLayoutBottomSheetBehavior.setState(BottomSheetBehavior.STATE_HIDDEN));
        }
      }
      else if (mViewModel.getSearchEnabled().getValue() != null && mViewModel.getSearchEnabled().getValue() != false)
        mFrameLayoutBottomSheetBehavior.setState(mViewModel.getSearchPageLastState().getValue());
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
    int h = dm.heightPixels;

    mFrameLayoutBottomSheetBehavior.setFitToContents(false);
    mFrameLayoutBottomSheetBehavior.setPeekHeight((int) (0.2f * h)); // collapsed
    mFrameLayoutBottomSheetBehavior.setHalfExpandedRatio(0.5f); // mid
    mFrameLayoutBottomSheetBehavior.setHideable(true);
    mFrameLayoutBottomSheetBehavior.setDraggable(true);
    mFrameLayoutBottomSheetBehavior.setState(BottomSheetBehavior.STATE_HIDDEN);
    mSearchPageContainer.addOnLayoutChangeListener((v, l, t, r, b, ol, ot, or, ob) -> updateExpandedOffset());
    mSearchPageContainer.post(this::updateExpandedOffset);
    if (mRoutingPlanFrame != null)
      mRoutingPlanFrame.addOnLayoutChangeListener((v, l, t, r, b, ol, ot, or, ob) -> updateExpandedOffset());

    ViewCompat.setOnApplyWindowInsetsListener(view, (v, insets) -> {
      mCurrentWindowInsets = insets;
      boolean imeVisible = insets.isVisible(WindowInsetsCompat.Type.ime());
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
    mPlacePageViewModel.getMapObject().observe(this, mPlacePageMapObjectObserver);
    mFrameLayoutBottomSheetBehavior.addBottomSheetCallback(mDefaultBottomSheetCallback);
    mViewModel.getSearchEnabled().observe(getViewLifecycleOwner(), mSearchPageEnabledObserver);
  }

  @Override
  public void onStop()
  {
    super.onStop();
    mPlacePageViewModel.getMapObject().removeObserver(mPlacePageMapObjectObserver);
    mFrameLayoutBottomSheetBehavior.removeBottomSheetCallback(mDefaultBottomSheetCallback);
    mViewModel.getSearchEnabled().removeObserver(mSearchPageEnabledObserver);
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

    View parent = mCoordinator != null ? mCoordinator : (View) mSearchPageContainer.getParent();
    if (parent == null)
      return;

    int parentHeight = parent.getHeight();
    int contentHeight = mSearchPageContainer.getHeight();
    if (parentHeight == 0 || contentHeight == 0)
      return;

    int topInset =
        mCurrentWindowInsets != null ? mCurrentWindowInsets.getInsets(WindowInsetsCompat.Type.systemBars()).top : 0;
    int routingHeaderHeight = 0;
    if (mRoutingPlanFrame != null && mRoutingPlanFrame.getVisibility() == View.VISIBLE)
      routingHeaderHeight = mRoutingPlanFrame.getHeight();
    if (routingHeaderHeight == 0 && RoutingController.get().isPlanning())
      routingHeaderHeight = (int) getResources().getDimension(
          ThemeUtils.getResource(requireContext(), androidx.appcompat.R.attr.actionBarSize));

    int expandedOffset = Math.max(parentHeight - contentHeight, topInset + routingHeaderHeight);
    mFrameLayoutBottomSheetBehavior.setExpandedOffset(Math.max(expandedOffset, 0));
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
    mFrameLayoutBottomSheetBehavior.setState(BottomSheetBehavior.STATE_HIDDEN);
    return false;
  }

  @Override
  public void onSearchClicked()
  {
    mFrameLayoutBottomSheetBehavior.setState(BottomSheetBehavior.STATE_HALF_EXPANDED);
  }
}
