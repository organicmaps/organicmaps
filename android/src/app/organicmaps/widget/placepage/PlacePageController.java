package app.organicmaps.widget.placepage;

import android.animation.ValueAnimator;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.res.Resources;
import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowInsets;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.widget.NestedScrollViewClickFixed;
import androidx.fragment.app.Fragment;
import androidx.interpolator.view.animation.FastOutSlowInInterpolator;
import androidx.lifecycle.Observer;
import androidx.lifecycle.ViewModelProvider;
import app.organicmaps.Framework;
import app.organicmaps.MwmActivity;
import app.organicmaps.R;
import app.organicmaps.base.Initializable;
import app.organicmaps.base.Savable;
import app.organicmaps.bookmarks.data.MapObject;
import app.organicmaps.settings.RoadType;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.bottomsheet.MenuBottomSheetFragment;
import app.organicmaps.util.bottomsheet.MenuBottomSheetItem;
import com.google.android.material.bottomsheet.BottomSheetBehavior;

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

public class PlacePageController implements Initializable<Activity>,
                                            Savable<Bundle>,
                                            PlacePageView.PlacePageViewListener,
                                            PlacePageButtons.PlacePageButtonClickListener,
                                            MenuBottomSheetFragment.MenuBottomSheetInterface,
                                            Observer<MapObject>
{
  private static final String TAG = PlacePageController.class.getSimpleName();
  private static final String PLACE_PAGE_BUTTONS_FRAGMENT_TAG = "PLACE_PAGE_BUTTONS";
  private static final String PLACE_PAGE_FRAGMENT_TAG = "PLACE_PAGE";

  private static final float PREVIEW_PLUS_RATIO = 0.45f;
  @NonNull
  private final SlideListener mSlideListener;
  @NonNull
  private final BottomSheetBehavior<View> mPlacePageBehavior;
  @NonNull
  private final NestedScrollViewClickFixed mPlacePage;
  @NonNull
  private final ViewGroup mPlacePageContainer;
  private final ViewGroup mCoordinator;
  private final int mViewportMinHeight;
  private final AppCompatActivity mMwmActivity;
  private final int mButtonsHeight;
  private final int mMaxButtons;
  private final PlacePageViewModel viewModel;
  private int mPreviewHeight;
  private int mFrameHeight;
  private boolean mDeactivateMapSelection = true;
  private MapObject mMapObject;
  private WindowInsets mCurrentWindowInsets;

  private boolean mShouldCollapse;
  private int mDistanceToTop;

  private ValueAnimator mCustomPeekHeightAnimator;

  @SuppressLint("ClickableViewAccessibility")
  public PlacePageController(@Nullable Activity activity,
                             @NonNull SlideListener listener)
  {
    mSlideListener = listener;

    Objects.requireNonNull(activity);
    mMwmActivity = (AppCompatActivity) activity;
    mCoordinator = mMwmActivity.findViewById(R.id.coordinator);
    Resources res = activity.getResources();
    mViewportMinHeight = res.getDimensionPixelSize(R.dimen.viewport_min_height);
    mPlacePage = activity.findViewById(R.id.placepage);
    mPlacePageContainer = activity.findViewById(R.id.placepage_container);
    mPlacePageBehavior = BottomSheetBehavior.from(mPlacePage);

    mShouldCollapse = true;

    BottomSheetChangedListener bottomSheetChangedListener = new BottomSheetChangedListener()
    {
      @Override
      public void onSheetHidden()
      {
        onHiddenInternal();
      }

      @Override
      public void onSheetDetailsOpened()
      {
        // No op.
      }

      @Override
      public void onSheetCollapsed()
      {
        // No op.
      }

      @Override
      public void onSheetSliding(int top)
      {
        stopCustomPeekHeightAnimation();
        mDistanceToTop = top;
        mSlideListener.onPlacePageSlide(top);
      }

      @Override
      public void onSheetSlideFinish()
      {
        PlacePageUtils.moveViewportUp(mPlacePage, mViewportMinHeight);
      }
    };
    BottomSheetBehavior.BottomSheetCallback sheetCallback = new DefaultBottomSheetCallback(bottomSheetChangedListener);
    mPlacePageBehavior.addBottomSheetCallback(sheetCallback);
    mPlacePageBehavior.setHideable(true);
    mPlacePageBehavior.setState(BottomSheetBehavior.STATE_HIDDEN);
    mPlacePageBehavior.setFitToContents(true);
    mPlacePageBehavior.setSkipCollapsed(true);

    UiUtils.bringViewToFrontOf(activity.findViewById(R.id.pp_buttons_fragment), mPlacePage);

    mPlacePage.requestApplyInsets();

    mButtonsHeight = (int)  mMwmActivity.getResources().getDimension(R.dimen.place_page_buttons_height);
    mMaxButtons = mMwmActivity.getResources().getInteger(R.integer.pp_buttons_max);
    viewModel = new ViewModelProvider(mMwmActivity).get(PlacePageViewModel.class);

    mPlacePage.setOnApplyWindowInsetsListener((view, windowInsets) -> {
      mCurrentWindowInsets = windowInsets;
      return windowInsets;
    });
  }

  private void stopCustomPeekHeightAnimation()
  {
    if (mCustomPeekHeightAnimator != null && mCustomPeekHeightAnimator.isStarted())
    {
      mCustomPeekHeightAnimator.end();
      setPlacePageHeightBounds();
    }
  }

  private void onHiddenInternal()
  {
    if (mDeactivateMapSelection)
      Framework.nativeDeactivatePopup();
    mDeactivateMapSelection = true;
    PlacePageUtils.moveViewportUp(mPlacePage, mViewportMinHeight);
    resetPlacePageHeightBounds();
    removePlacePageFragments();
  }

  public int getPlacePageWidth()
  {
    return mPlacePage.getWidth();
  }

  @Nullable
  public ArrayList<MenuBottomSheetItem> getMenuBottomSheetItems(String id)
  {
    final List<PlacePageButtons.ButtonType> currentItems = viewModel.getCurrentButtons().getValue();
    if (currentItems == null || currentItems.size() <= mMaxButtons)
      return null;
    ArrayList<MenuBottomSheetItem> items = new ArrayList<>();
    for (int i = mMaxButtons - 1; i < currentItems.size(); i++)
    {
      final PlacePageButton bsItem = PlacePageButtonFactory.createButton(currentItems.get(i), mMwmActivity);
      items.add(new MenuBottomSheetItem(
          bsItem.getTitle(),
          bsItem.getIcon(),
          () -> ((PlacePageButtons.PlacePageButtonClickListener) mMwmActivity).onPlacePageButtonClick(bsItem.getType())
      ));
    }
    return items;
  }

  public void openFor(@NonNull PlacePageData data)
  {
    mDeactivateMapSelection = true;
    MapObject mapObject = (MapObject) data;
    final MapObject previousMapObject = viewModel.getMapObject().getValue();
    // Only collapse the place page if the data is different from the one already available
    mShouldCollapse = PlacePageUtils.isHiddenState(mPlacePageBehavior.getState()) || !MapObject.same(previousMapObject, mapObject);
    viewModel.setMapObject(mapObject);
  }

  private void resetPlacePageHeightBounds()
  {
    mPlacePageContainer.setMinimumHeight(0);
    final int parentHeight = ((View) mPlacePage.getParent()).getHeight();
    mPlacePageBehavior.setMaxHeight(parentHeight);
  }

  private void setPlacePageHeightBounds()
  {
    // Set the minimum height of the place page to prevent jumps when new data results in SMALLER content
    // This cannot be set on the place page itself as it has the fitToContent property set
    mPlacePageContainer.setMinimumHeight(mFrameHeight);
    // Set the maximum height of the place page to prevent jumps when new data results in BIGGER content
    // It does not take into account the navigation bar height so we need to add it manually
    mPlacePageBehavior.setMaxHeight(mFrameHeight + mCurrentWindowInsets.getSystemWindowInsetBottom());
  }

  private void setPeekHeight()
  {
    final int peekHeight = calculatePeekHeight();

    final int state = mPlacePageBehavior.getState();
    final boolean shouldAnimate = !PlacePageUtils.isHiddenState(state);
    if (shouldAnimate)
      animatePeekHeight(peekHeight);
    else
    {
      mPlacePageBehavior.setPeekHeight(peekHeight);
      setPlacePageHeightBounds();
    }
  }

  private void animatePeekHeight(int peekHeight)
  {
    // Using the animate param in setPeekHeight does not work when adding removing fragments
    // from inside the place page so we manually animate the peek height with ValueAnimator

    // Make sure to start from the current height of the place page
    final int parentHeight = ((View) mPlacePage.getParent()).getHeight();
    // Make sure to remove the navbar height because the peek height already takes it into account
    int initialHeight = parentHeight - mDistanceToTop - mCurrentWindowInsets.getSystemWindowInsetBottom();

    if (mCustomPeekHeightAnimator != null)
      mCustomPeekHeightAnimator.cancel();
    mCustomPeekHeightAnimator = ValueAnimator.ofInt(initialHeight, peekHeight);
    mCustomPeekHeightAnimator.setInterpolator(new FastOutSlowInInterpolator());
    mCustomPeekHeightAnimator.addUpdateListener(valueAnimator -> {
      int value = (Integer) valueAnimator.getAnimatedValue();
      // Make sure the place page can reach the animated peek height to prevent jumps
      // maxHeight does not take the navbar height into account so we manually add it
      mPlacePageBehavior.setMaxHeight(value + mCurrentWindowInsets.getSystemWindowInsetBottom());
      mPlacePageBehavior.setPeekHeight(value);
      // The place page is not firing the slide callbacks when using this animation, so we must call them manually
      mDistanceToTop = parentHeight - value - mCurrentWindowInsets.getSystemWindowInsetBottom();
      mSlideListener.onPlacePageSlide(mDistanceToTop);
      if (value == peekHeight)
      {
        PlacePageUtils.moveViewportUp(mPlacePage, mViewportMinHeight);
        setPlacePageHeightBounds();
      }
    });
    mCustomPeekHeightAnimator.setDuration(200);
    mCustomPeekHeightAnimator.start();
  }

  private int calculatePeekHeight()
  {
    if (mMapObject.getOpeningMode() == MapObject.OPENING_MODE_PREVIEW_PLUS)
      return (int) (mCoordinator.getHeight() * PREVIEW_PLUS_RATIO);
    return mPreviewHeight + mButtonsHeight;
  }

  public void close(boolean deactivateMapSelection)
  {
    mDeactivateMapSelection = deactivateMapSelection;
    mPlacePageBehavior.setState(BottomSheetBehavior.STATE_HIDDEN);
  }

  public boolean isClosed()
  {
    return PlacePageUtils.isHiddenState(mPlacePageBehavior.getState());
  }

  @Override
  public void onSave(@NonNull Bundle outState)
  {
    // no op
  }

  @Override
  public void onRestore(@NonNull Bundle inState)
  {
    if (mPlacePageBehavior.getState() == BottomSheetBehavior.STATE_HIDDEN)
      return;
    if (!Framework.nativeHasPlacePageInfo())
      close(false);
  }

  @Override
  public void onPlacePageContentChanged(int previewHeight, int frameHeight)
  {
    mPreviewHeight = previewHeight;
    mFrameHeight = frameHeight;
    setPeekHeight();
    if (mShouldCollapse && !PlacePageUtils.isCollapsedState(mPlacePageBehavior.getState()))
      mPlacePageBehavior.setState(BottomSheetBehavior.STATE_COLLAPSED);
    mShouldCollapse = false;
  }

  @Override
  public void onPlacePageRequestClose()
  {
    close(true);
  }

  @Override
  public void onPlacePageRequestToggleState()
  {
    @BottomSheetBehavior.State
    int state = mPlacePageBehavior.getState();
    stopCustomPeekHeightAnimation();
    if (PlacePageUtils.isExpandedState(state))
      mPlacePageBehavior.setState(BottomSheetBehavior.STATE_HIDDEN);
    else
      mPlacePageBehavior.setState(BottomSheetBehavior.STATE_EXPANDED);
  }

  @Override
  public void onPlacePageRequestToggleRouteSettings(@NonNull RoadType roadType)
  {
    // no op
  }

  @Override
  public void onPlacePageButtonClick(PlacePageButtons.ButtonType item)
  {
    @Nullable final PlacePageView placePageFragment = (PlacePageView) mMwmActivity.getSupportFragmentManager()
                                                                                  .findFragmentByTag(PLACE_PAGE_FRAGMENT_TAG);
    if (placePageFragment != null)
      placePageFragment.onPlacePageButtonClick(item);
  }

  private void removePlacePageFragments()
  {
    Fragment placePageButtonsFragment = mMwmActivity.getSupportFragmentManager()
                                                    .findFragmentByTag(PLACE_PAGE_BUTTONS_FRAGMENT_TAG);
    if (placePageButtonsFragment != null)
    {
      mMwmActivity.getSupportFragmentManager().beginTransaction()
                  .remove(placePageButtonsFragment)
                  .commit();
    }
    Fragment placePageFragment = mMwmActivity.getSupportFragmentManager()
                                             .findFragmentByTag(PLACE_PAGE_FRAGMENT_TAG);
    if (placePageFragment != null)
    {
      mMwmActivity.getSupportFragmentManager().beginTransaction()
                  .remove(placePageFragment)
                  .commit();
    }
  }

  private void createPlacePageFragments()
  {
    if (mMwmActivity.getSupportFragmentManager()
                    .findFragmentByTag(PLACE_PAGE_FRAGMENT_TAG) == null)
    {
      mMwmActivity.getSupportFragmentManager().beginTransaction()
                  .add(R.id.placepage_fragment,
                       PlacePageView.class, null, PLACE_PAGE_FRAGMENT_TAG)
                  .commit();
    }
    if (mMwmActivity.getSupportFragmentManager()
                    .findFragmentByTag(PLACE_PAGE_BUTTONS_FRAGMENT_TAG) == null)
    {
      mMwmActivity.getSupportFragmentManager().beginTransaction()
                  .add(R.id.pp_buttons_fragment,
                       PlacePageButtons.class, null, PLACE_PAGE_BUTTONS_FRAGMENT_TAG)
                  .commit();
    }
  }

  @Override
  public void onChanged(MapObject mapObject)
  {

    mMapObject = mapObject;
    createPlacePageFragments();
  }

  @Override
  public void initialize(@Nullable Activity activity)
  {
    Objects.requireNonNull(activity);
    viewModel.getMapObject().observe((MwmActivity) activity, this);
  }

  @Override
  public void destroy()
  {
    viewModel.getMapObject().removeObserver(this);
  }

  public interface SlideListener
  {
    void onPlacePageSlide(int top);
  }
}
