package app.organicmaps.widget.modalsearch;

import android.content.res.Configuration;
import android.content.res.Resources;
import android.graphics.Rect;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.coordinatorlayout.widget.CoordinatorLayout;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.core.widget.NestedScrollView;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;
import androidx.fragment.app.FragmentManager;
import androidx.lifecycle.Observer;
import androidx.lifecycle.ViewModelProvider;

import com.google.android.material.bottomsheet.BottomSheetBehavior;

import app.organicmaps.Framework;
import app.organicmaps.R;
import app.organicmaps.sdk.search.SearchEngine;
import app.organicmaps.search.SearchFragment;
import app.organicmaps.util.InputUtils;

public class ModalSearchController extends Fragment
{
  private static final String SEARCH_FRAGMENT_TAG = SearchFragment.class.getSimpleName();
  private SearchBottomSheetBehavior<NestedScrollView> mSearchBehavior;
  private NestedScrollView mModalSearch;
  private ViewGroup mCoordinator;
  private int mViewportMinHeight;
  private ModalSearchViewModel mViewModel;
  private final Observer<Boolean> mModalSearchSuspendedObserver = suspended -> {
    if (Boolean.FALSE.equals(mViewModel.getModalSearchActive().getValue()))
      return;
    if (suspended)
    {
      mModalSearch.setVisibility(View.GONE);
      InputUtils.hideKeyboard(mModalSearch);
    }
    else
      mModalSearch.setVisibility(View.VISIBLE);
  };
  private ViewGroup mModalSearchFragmentContainer;
  private WindowInsetsCompat mCurrentWindowInsets;
  private int mDistanceToTop;
  private final BottomSheetBehavior.BottomSheetCallback mDefaultBottomSheetCallback = new BottomSheetBehavior.BottomSheetCallback()
  {
    @Override
    public void onStateChanged(@NonNull View bottomSheet, int newState)
    {
      if (ModalSearchUtils.isSettlingState(newState) || ModalSearchUtils.isDraggingState(newState))
        return;

      ModalSearchUtils.updateMapViewport(mCoordinator, mDistanceToTop, mViewportMinHeight);

      if (ModalSearchUtils.isHiddenState(newState))
        onHiddenInternal();
    }

    @Override
    public void onSlide(@NonNull View bottomSheet, float slideOffset)
    {
      mDistanceToTop = bottomSheet.getTop();
      mViewModel.setModalSearchDistanceToTop(mDistanceToTop);
    }
  };
  private int mDisplayHeight;
  private CoordinatorLayout mSearchFragmentCoordinator;
  private final FragmentManager.FragmentLifecycleCallbacks mFragmentLifecycleCallbacks = new FragmentManager.FragmentLifecycleCallbacks()
  {
    @Override
    public void onFragmentStarted(@NonNull FragmentManager fm, @NonNull Fragment f)
    {
      mSearchFragmentCoordinator = mModalSearchFragmentContainer.findViewById(R.id.coordinator);
      super.onFragmentStarted(fm, f);
    }
  };
  private FrameLayout mDragIndicator;
  private final Observer<Integer> mModalSearchDistanceToTopObserver = new Observer<>()
  {
    private int mDragHandleHeight;

    @Override
    public void onChanged(Integer distanceToTop)
    {
      if (mDragHandleHeight == 0)
      {
        mDragHandleHeight = mDragIndicator.getMeasuredHeight();
      }
      int topInset = 0, bottomInset = 0;
      if (mCurrentWindowInsets != null)
      {
        Insets insets = mCurrentWindowInsets.getInsets(WindowInsetsCompat.Type.systemBars());
        topInset = insets.top;
        bottomInset = insets.bottom;
      }
      final int topInsetOverlap = Math.max(0, topInset - distanceToTop);
      mDragIndicator.setPadding(0, topInsetOverlap, 0, 0);
      if (mSearchFragmentCoordinator != null)
      {
        ViewGroup.LayoutParams params = mSearchFragmentCoordinator.getLayoutParams();
        params.height = mDisplayHeight - distanceToTop - topInsetOverlap - mDragHandleHeight - bottomInset;
        mSearchFragmentCoordinator.setLayoutParams(params);
      }
    }
  };
  private final Observer<Boolean> mModalSearchActiveObserver = active -> {
    if (active)
      startSearch(null);
    else
      closeSearch();
  };
  private final Observer<Boolean> mModalSearchCollapsedObserver = collapsed -> {
    if (collapsed)
    {
      setCollapsible(true);
      mSearchBehavior.setState(BottomSheetBehavior.STATE_COLLAPSED);
    }
    else
    {
      if (mSearchBehavior.getState() == BottomSheetBehavior.STATE_COLLAPSED)
        mSearchBehavior.setState(BottomSheetBehavior.STATE_HALF_EXPANDED);
      if (Boolean.TRUE.equals(mViewModel.getIsQueryEmpty().getValue()))
        setCollapsible(false);
    }
  };
  private final Observer<Boolean> mIsQueryEmptyObserver = isQueryEmpty -> setCollapsible(!isQueryEmpty);

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    View rootView = inflater.inflate(R.layout.modal_search_container_fragment, container, false);
    rootView.getViewTreeObserver().addOnGlobalLayoutListener(() -> {
      mDisplayHeight = rootView.getHeight();
      if (mSearchBehavior != null)
      {
        Rect r = new Rect();
        rootView.getWindowVisibleDisplayFrame(r);
        final int availableHeight = (r.bottom - r.top);
        mSearchBehavior.updateUnavailableScreenRatio((float) (mDisplayHeight - availableHeight) / mDisplayHeight);
      }
    });
    return rootView;
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    final FragmentActivity activity = requireActivity();

    final Resources res = activity.getResources();
    mViewportMinHeight = res.getDimensionPixelSize(R.dimen.viewport_min_height);

    mCoordinator = activity.findViewById(R.id.coordinator);
    mModalSearch = activity.findViewById(R.id.modal_search);
    mDragIndicator = mModalSearch.findViewById(R.id.drag_indicator);
    mModalSearch.setNestedScrollingEnabled(false);
    mModalSearchFragmentContainer = activity.findViewById(R.id.modal_search_fragment);
    mSearchBehavior = SearchBottomSheetBehavior.from(
        mModalSearch,
        getLifecycle(),
        getResources().getFraction(R.fraction.modal_search_half_expanded_ratio, 1, 1)
    );

    mSearchBehavior.setHideable(true);
    mSearchBehavior.setPeekHeight(300);
    mSearchBehavior.setState(BottomSheetBehavior.STATE_HIDDEN);
    mSearchBehavior.setFitToContents(false);
    mSearchBehavior.setSkipCollapsed(true);

    mViewModel = new ViewModelProvider(requireActivity()).get(ModalSearchViewModel.class);

    ViewCompat.setOnApplyWindowInsetsListener(mModalSearch, (v, windowInsets) -> {
      mCurrentWindowInsets = windowInsets;
      return windowInsets;
    });

    ViewCompat.requestApplyInsets(mModalSearch);
  }


  @Override
  public void onConfigurationChanged(@NonNull Configuration newConfig)
  {
    super.onConfigurationChanged(newConfig);
  }

  private void onHiddenInternal()
  {
    Framework.nativeDeactivatePopup();
    ModalSearchUtils.updateMapViewport(mCoordinator, mDistanceToTop, mViewportMinHeight);
    removeModalSearchFragments();
  }

  private void startSearch(Bundle searchArguments)
  {
    createModalSearchFragments(searchArguments);
    mDragIndicator.setVisibility(View.VISIBLE);
    mModalSearch.setVisibility(View.VISIBLE);
    mSearchBehavior.setState(BottomSheetBehavior.STATE_HALF_EXPANDED);
  }

  private void closeSearch()
  {
    SearchEngine.INSTANCE.cancel();
    mSearchBehavior.setHideable(true);
    mSearchBehavior.setState(BottomSheetBehavior.STATE_HIDDEN);
    removeModalSearchFragments();
    mDragIndicator.setVisibility(View.GONE);
  }

  private void setCollapsible(boolean collapsible)
  {
    if (collapsible)
    {
      mSearchBehavior.setSkipCollapsed(false);
      mSearchBehavior.setPeekHeight(calculateCollapsedHeight());
    }
    else
      mSearchBehavior.setSkipCollapsed(true);
  }

  private int calculateCollapsedHeight()
  {
    try
    {
      return mDragIndicator.getMeasuredHeight() +
          mModalSearch.findViewById(R.id.app_bar).getMeasuredHeight();    // TODO(savsch) get feedback on whether to change this height
    } catch (NullPointerException npe)
    {
      return 0;
    }
  }

  private void removeModalSearchFragments()
  {
    final FragmentManager fm = getChildFragmentManager();
    final Fragment modalSearchFragment = fm.findFragmentByTag(SEARCH_FRAGMENT_TAG);

    if (modalSearchFragment != null)
    {
      fm.beginTransaction()
          .setReorderingAllowed(true)
          .remove(modalSearchFragment)
          .commit();
    }
  }

  private void createModalSearchFragments(Bundle searchArguments)
  {
    final FragmentManager fm = getChildFragmentManager();
    if (fm.findFragmentByTag(SEARCH_FRAGMENT_TAG) == null)
    {
      fm.beginTransaction()
          .setReorderingAllowed(true)
          .add(R.id.modal_search_fragment, SearchFragment.class, searchArguments, SEARCH_FRAGMENT_TAG)
          .commit();
    }
  }

  public void restartSearch(Bundle searchArguments)
  {
    closeSearch();
    getChildFragmentManager().executePendingTransactions();
    startSearch(searchArguments);
  }

  @Override
  public void onStart()
  {
    super.onStart();
    mSearchBehavior.addBottomSheetCallback(mDefaultBottomSheetCallback);
    getChildFragmentManager().registerFragmentLifecycleCallbacks(mFragmentLifecycleCallbacks, false);
    FragmentActivity activity = requireActivity();
    mViewModel.getModalSearchDistanceToTop().observe(activity, mModalSearchDistanceToTopObserver);
    mViewModel.getModalSearchActive().observe(activity, mModalSearchActiveObserver);
    mViewModel.getModalSearchCollapsed().observe(activity, mModalSearchCollapsedObserver);
    mViewModel.getModalSearchSuspended().observe(activity, mModalSearchSuspendedObserver);
    mViewModel.getIsQueryEmpty().observe(activity, mIsQueryEmptyObserver);
  }

  @Override
  public void onStop()
  {
    super.onStop();
    mSearchBehavior.removeBottomSheetCallback(mDefaultBottomSheetCallback);
    getChildFragmentManager().unregisterFragmentLifecycleCallbacks(mFragmentLifecycleCallbacks);
    mViewModel.getModalSearchDistanceToTop().removeObserver(mModalSearchDistanceToTopObserver);
    mViewModel.getModalSearchActive().removeObserver(mModalSearchActiveObserver);
    mViewModel.getModalSearchCollapsed().removeObserver(mModalSearchCollapsedObserver);
    mViewModel.getModalSearchSuspended().removeObserver(mModalSearchSuspendedObserver);
    mViewModel.getIsQueryEmpty().removeObserver(mIsQueryEmptyObserver);
  }
}
