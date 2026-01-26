package app.organicmaps.search;

import android.os.Bundle;
import android.util.DisplayMetrics;
import android.view.LayoutInflater;
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
import app.organicmaps.sdk.util.log.Logger;
import app.organicmaps.widget.placepage.PlacePageUtils;
import app.organicmaps.widget.placepage.PlacePageViewModel;
import com.google.android.material.bottomsheet.BottomSheetBehavior;

public class SearchFragmentController extends Fragment implements SearchFragment.SearchFragmentListener
{
  BottomSheetBehavior<FrameLayout> mFrameLayoutBottomSheetBehavior;
  private ViewGroup mCoordinator;
  private WindowInsetsCompat mCurrentWindowInsets;
  FrameLayout mSearchPageContainer;
  int mDistanceToTop;
  int mViewportMinHeight;
  SearchPageViewModel mViewModel;
  PlacePageViewModel mPlacePageViewModel;

  private final BottomSheetBehavior.BottomSheetCallback mDefaultBottomSheetCallback =
      new BottomSheetBehavior.BottomSheetCallback() {
        @Override
        public void onStateChanged(@NonNull View bottomSheet, int newState)
        {
          //          Logger.d(TAG, "State change, new = " + PlacePageUtils.toString(newState));
          if (PlacePageUtils.isSettlingState(newState) || PlacePageUtils.isDraggingState(newState))
            return;

          PlacePageUtils.updateMapViewport(mCoordinator, mDistanceToTop, mViewportMinHeight);

          if (PlacePageUtils.isHiddenState(newState) && mPlacePageViewModel.getMapObject() == null)
            closeSearch();
          // we do not save the state if search page is hiding
          if (!PlacePageUtils.isHiddenState(newState))
            mViewModel.setSearchPageLastState(newState);
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
        mViewModel.setSearchPageLastState(mFrameLayoutBottomSheetBehavior.getState());
        mFrameLayoutBottomSheetBehavior.setState(BottomSheetBehavior.STATE_HIDDEN);
      }
      else if (mViewModel.getSearchEnabled().getValue() != null && mViewModel.getSearchEnabled().getValue() != false)
        mFrameLayoutBottomSheetBehavior.setState(mViewModel.getSearchPageLastState().getValue());
    }
  };

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.search_fragment_container, container, false);
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    mViewModel = new ViewModelProvider(requireActivity()).get(SearchPageViewModel.class);
    mPlacePageViewModel = new ViewModelProvider(requireActivity()).get(PlacePageViewModel.class);

    mCoordinator = requireActivity().findViewById(R.id.coordinator);
    mViewportMinHeight = requireActivity().getResources().getDimensionPixelSize(R.dimen.viewport_min_height);

    mSearchPageContainer = view.findViewById(R.id.search_page_container);
    mFrameLayoutBottomSheetBehavior = BottomSheetBehavior.from(mSearchPageContainer);

    DisplayMetrics dm = getResources().getDisplayMetrics();
    int h = dm.heightPixels;

    mFrameLayoutBottomSheetBehavior.setFitToContents(false);
    mFrameLayoutBottomSheetBehavior.setPeekHeight((int) (0.2f * h)); // collapsed
    mFrameLayoutBottomSheetBehavior.setHalfExpandedRatio(0.5f); // mid
    mFrameLayoutBottomSheetBehavior.setExpandedOffset((int) (0.1f * h)); // full
    mFrameLayoutBottomSheetBehavior.setHideable(true);
    mFrameLayoutBottomSheetBehavior.setDraggable(true);
    mFrameLayoutBottomSheetBehavior.setState(BottomSheetBehavior.STATE_HIDDEN);

    mViewModel.getSearchEnabled().observe(requireActivity(), mSearchPageEnabledObserver);

    ViewCompat.setOnApplyWindowInsetsListener(view, (v, insets) -> {
      boolean imeVisible = insets.isVisible(WindowInsetsCompat.Type.ime());
      if (imeVisible && mFrameLayoutBottomSheetBehavior.getState() != BottomSheetBehavior.STATE_EXPANDED)
        mFrameLayoutBottomSheetBehavior.setState(BottomSheetBehavior.STATE_EXPANDED);
      return insets;
    });

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
  }

  @Override
  public void onStop()
  {
    super.onStop();
    mPlacePageViewModel.getMapObject().removeObserver(mPlacePageMapObjectObserver);
    mFrameLayoutBottomSheetBehavior.removeBottomSheetCallback(mDefaultBottomSheetCallback);
  }

  private void closeSearch()
  {
    FragmentManager childFm = getChildFragmentManager();
    childFm.popBackStackImmediate(null, FragmentManager.POP_BACK_STACK_INCLUSIVE);
    Fragment search = childFm.findFragmentByTag("SearchPageFragment");
    if (search != null)
      childFm.beginTransaction().remove(search).commitNowAllowingStateLoss();

    getParentFragmentManager().beginTransaction().remove(this).commitNowAllowingStateLoss();
  }

  @Override
  public boolean getBackPressedCallback()
  {
    if (mFrameLayoutBottomSheetBehavior.getState() == BottomSheetBehavior.STATE_EXPANDED
        || mFrameLayoutBottomSheetBehavior.getState() == BottomSheetBehavior.STATE_HALF_EXPANDED)
    {
      mFrameLayoutBottomSheetBehavior.setState(BottomSheetBehavior.STATE_COLLAPSED);
      return true;
    }
    mFrameLayoutBottomSheetBehavior.setState(BottomSheetBehavior.STATE_HIDDEN);
    return false;
  }
}
