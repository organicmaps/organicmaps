package app.organicmaps.widget.placepage;

import android.animation.ValueAnimator;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.res.Resources;
import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.core.widget.NestedScrollViewClickFixed;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.interpolator.view.animation.FastOutSlowInInterpolator;
import androidx.lifecycle.Lifecycle;
import androidx.lifecycle.Observer;
import androidx.lifecycle.ViewModelProvider;
import app.organicmaps.Framework;
import app.organicmaps.MwmActivity;
import app.organicmaps.R;
import app.organicmaps.api.ParsedMwmRequest;
import app.organicmaps.base.Initializable;
import app.organicmaps.base.Savable;
import app.organicmaps.bookmarks.data.MapObject;
import app.organicmaps.bookmarks.data.RoadWarningMarkType;
import app.organicmaps.routing.RoutingController;
import app.organicmaps.settings.RoadType;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.bottomsheet.MenuBottomSheetFragment;
import app.organicmaps.util.bottomsheet.MenuBottomSheetItem;
import app.organicmaps.util.log.Logger;

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
  private final PlacePageViewModel mViewModel;
  private int mPreviewHeight;
  private int mFrameHeight;
  private boolean mDeactivateMapSelection = true;
  @Nullable
  private MapObject mMapObject;
  private WindowInsetsCompat mCurrentWindowInsets;

  private boolean mShouldCollapse;
  private int mDistanceToTop;

  private ValueAnimator mCustomPeekHeightAnimator;

  class DefaultBottomSheetCallback extends BottomSheetBehavior.BottomSheetCallback
  {
    @Override
    public void onStateChanged(@NonNull View bottomSheet, int newState)
    {
      final Lifecycle.State state = mMwmActivity.getLifecycle().getCurrentState();
      if (!state.isAtLeast(Lifecycle.State.RESUMED))
      {
        Logger.e(TAG, "Called in the wrong activity state=" + state);
        return;
      }

      Logger.d(TAG, "State change, new = " + PlacePageUtils.toString(newState));
      if (PlacePageUtils.isSettlingState(newState) || PlacePageUtils.isDraggingState(newState))
        return;

      if (PlacePageUtils.isHiddenState(newState))
        onHiddenInternal();
    }

    @Override
    public void onSlide(@NonNull View bottomSheet, float slideOffset)
    {
      final Lifecycle.State state = mMwmActivity.getLifecycle().getCurrentState();
      if (!state.isAtLeast(Lifecycle.State.RESUMED))
      {
        Logger.e(TAG, "Called in the wrong activity state=" + state);
        return;
      }
      stopCustomPeekHeightAnimation();
      mDistanceToTop = bottomSheet.getTop();
      mSlideListener.onPlacePageSlide(mDistanceToTop);
    }
  }

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

    mPlacePageBehavior.addBottomSheetCallback(new DefaultBottomSheetCallback());
    mPlacePageBehavior.setHideable(true);
    mPlacePageBehavior.setState(BottomSheetBehavior.STATE_HIDDEN);
    mPlacePageBehavior.setFitToContents(true);
    mPlacePageBehavior.setSkipCollapsed(true);

    UiUtils.bringViewToFrontOf(activity.findViewById(R.id.pp_buttons_fragment), mPlacePage);

    mPlacePage.requestApplyInsets();

    mButtonsHeight = (int) mMwmActivity.getResources()
                                       .getDimension(R.dimen.place_page_buttons_height);
    mMaxButtons = mMwmActivity.getResources().getInteger(R.integer.pp_buttons_max);
    mViewModel = new ViewModelProvider(mMwmActivity).get(PlacePageViewModel.class);

    ViewCompat.setOnApplyWindowInsetsListener(mPlacePage, (view, windowInsets) -> {
      mCurrentWindowInsets = windowInsets;
      return windowInsets;
    });
  }

  @NonNull
  private static PlacePageButtons.ButtonType toPlacePageButton(@NonNull RoadWarningMarkType type)
  {
    switch (type)
    {
      case DIRTY:
        return PlacePageButtons.ButtonType.ROUTE_AVOID_UNPAVED;
      case FERRY:
        return PlacePageButtons.ButtonType.ROUTE_AVOID_FERRY;
      case TOLL:
        return PlacePageButtons.ButtonType.ROUTE_AVOID_TOLL;
      default:
        throw new AssertionError("Unsupported road warning type: " + type);
    }
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
    final List<PlacePageButtons.ButtonType> currentItems = mViewModel.getCurrentButtons().getValue();
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
    final MapObject previousMapObject = mViewModel.getMapObject().getValue();
    // Only collapse the place page if the data is different from the one already available
    mShouldCollapse = PlacePageUtils.isHiddenState(mPlacePageBehavior.getState()) || !MapObject.same(previousMapObject, mapObject);
    mViewModel.setMapObject(mapObject);
  }

  private void resetPlacePageHeightBounds()
  {
    mFrameHeight = 0;
    mPlacePageContainer.setMinimumHeight(0);
    final int parentHeight = ((View) mPlacePage.getParent()).getHeight();
    mPlacePageBehavior.setMaxHeight(parentHeight);
  }

  /**
   * Set the min and max height of the place page to prevent jumps when switching from one map object
   * to the other.
   */
  private void setPlacePageHeightBounds()
  {
    final int peekHeight = calculatePeekHeight();
    // Make sure the place page can reach the peek height
    final int height = Math.max(peekHeight, mFrameHeight);
    // Set the minimum height of the place page to prevent jumps when new data results in SMALLER content
    // This cannot be set on the place page itself as it has the fitToContent property set
    mPlacePageContainer.setMinimumHeight(height);
    // Set the maximum height of the place page to prevent jumps when new data results in BIGGER content
    // It does not take into account the navigation bar height so we need to add it manually
    mPlacePageBehavior.setMaxHeight(height + mCurrentWindowInsets.getInsets(WindowInsetsCompat.Type.systemBars()).bottom);
  }

  /**
   * Make sure the place page can reach the peek height
   */
  private void preparePlacePageMinHeight(int peekHeight)
  {
    final int currentHeight = mPlacePageContainer.getHeight();
    if (currentHeight < peekHeight)
      mPlacePageContainer.setMinimumHeight(peekHeight);
  }

  private void setPeekHeight()
  {
    final int peekHeight = calculatePeekHeight();
    preparePlacePageMinHeight(peekHeight);

    final int state = mPlacePageBehavior.getState();
    // Do not animate the peek height if the place page should not be collapsed (eg: when returning from editor)
    final boolean shouldAnimate = !(PlacePageUtils.isExpandedState(state) && !mShouldCollapse) && !PlacePageUtils.isHiddenState(state);
    if (shouldAnimate)
      animatePeekHeight(peekHeight);
    else
    {
      mPlacePageBehavior.setPeekHeight(peekHeight);
      setPlacePageHeightBounds();
    }
  }

  /**
   * Using the animate param in setPeekHeight does not work when adding removing fragments
   * from inside the place page so we manually animate the peek height with ValueAnimator
   */
  private void animatePeekHeight(int peekHeight)
  {
    final int bottomInsets = mCurrentWindowInsets.getInsets(WindowInsetsCompat.Type.systemBars()).bottom;
    // Make sure to start from the current height of the place page
    final int parentHeight = ((View) mPlacePage.getParent()).getHeight();
    // Make sure to remove the navbar height because the peek height already takes it into account
    int initialHeight = parentHeight - mDistanceToTop - bottomInsets;

    if (mCustomPeekHeightAnimator != null)
      mCustomPeekHeightAnimator.cancel();
    mCustomPeekHeightAnimator = ValueAnimator.ofInt(initialHeight, peekHeight);
    mCustomPeekHeightAnimator.setInterpolator(new FastOutSlowInInterpolator());
    mCustomPeekHeightAnimator.addUpdateListener(valueAnimator -> {
      int value = (Integer) valueAnimator.getAnimatedValue();
      // Make sure the place page can reach the animated peek height to prevent jumps
      // maxHeight does not take the navbar height into account so we manually add it
      mPlacePageBehavior.setMaxHeight(value + bottomInsets);
      mPlacePageBehavior.setPeekHeight(value);
      // The place page is not firing the slide callbacks when using this animation, so we must call them manually
      mDistanceToTop = parentHeight - value - bottomInsets;
      mSlideListener.onPlacePageSlide(mDistanceToTop);
      if (value == peekHeight)
      {
        setPlacePageHeightBounds();
      }
    });
    mCustomPeekHeightAnimator.setDuration(200);
    mCustomPeekHeightAnimator.start();
  }

  private int calculatePeekHeight()
  {
    if (mMapObject != null && mMapObject.getOpeningMode() == MapObject.OPENING_MODE_PREVIEW_PLUS)
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
    // Make sure to update the peek height on the UI thread to prevent weird animation jumps
    mPlacePage.post(() -> {
      setPeekHeight();
      if (mShouldCollapse && !PlacePageUtils.isCollapsedState(mPlacePageBehavior.getState()))
        mPlacePageBehavior.setState(BottomSheetBehavior.STATE_COLLAPSED);
      mShouldCollapse = false;
    });
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
    final FragmentManager fm = mMwmActivity.getSupportFragmentManager();
    final Fragment placePageButtonsFragment = fm.findFragmentByTag(PLACE_PAGE_BUTTONS_FRAGMENT_TAG);
    final Fragment placePageFragment = fm.findFragmentByTag(PLACE_PAGE_FRAGMENT_TAG);

    if (placePageButtonsFragment != null)
    {
      fm.beginTransaction()
        .setReorderingAllowed(true)
        .remove(placePageButtonsFragment)
        .commitNow();
    }
    if (placePageFragment != null)
    {
      // Make sure to synchronously remove the fragment so setting the map object to null
      // won't impact the fragment
      fm.beginTransaction()
        .setReorderingAllowed(true)
        .remove(placePageFragment)
        .commitNow();
    }
    mViewModel.setMapObject(null);
  }

  private void createPlacePageFragments()
  {
    final FragmentManager fm = mMwmActivity.getSupportFragmentManager();
    if (fm.findFragmentByTag(PLACE_PAGE_FRAGMENT_TAG) == null)
    {
      fm.beginTransaction()
        .setReorderingAllowed(true)
        .add(R.id.placepage_fragment, PlacePageView.class, null, PLACE_PAGE_FRAGMENT_TAG)
        .commit();
    }
    if (fm.findFragmentByTag(PLACE_PAGE_BUTTONS_FRAGMENT_TAG) == null)
    {
      fm.beginTransaction()
        .setReorderingAllowed(true)
        .add(R.id.pp_buttons_fragment, PlacePageButtons.class, null, PLACE_PAGE_BUTTONS_FRAGMENT_TAG)
        .commit();
    }
  }

  private void updateButtons(MapObject mapObject, boolean showBackButton, boolean showRoutingButton)
  {
    List<PlacePageButtons.ButtonType> buttons = new ArrayList<>();
    if (mapObject.getRoadWarningMarkType() != RoadWarningMarkType.UNKNOWN)
    {
      RoadWarningMarkType markType = mapObject.getRoadWarningMarkType();
      PlacePageButtons.ButtonType roadType = toPlacePageButton(markType);
      buttons.add(roadType);
    }
    else if (RoutingController.get().isRoutePoint(mapObject))
    {
      buttons.add(PlacePageButtons.ButtonType.ROUTE_REMOVE);
    }
    else
    {
      final ParsedMwmRequest request = ParsedMwmRequest.getCurrentRequest();
      if (showBackButton || (request != null && request.isPickPointMode()))
        buttons.add(PlacePageButtons.ButtonType.BACK);

      boolean needToShowRoutingButtons = RoutingController.get().isPlanning() || showRoutingButton;

      if (needToShowRoutingButtons)
        buttons.add(PlacePageButtons.ButtonType.ROUTE_FROM);

      // If we can show the add route button, put it in the place of the bookmark button
      // And move the bookmark button at the end
      if (needToShowRoutingButtons && RoutingController.get().isStopPointAllowed())
        buttons.add(PlacePageButtons.ButtonType.ROUTE_ADD);
      else
        buttons.add(mapObject.getMapObjectType() == MapObject.BOOKMARK
                    ? PlacePageButtons.ButtonType.BOOKMARK_DELETE
                    : PlacePageButtons.ButtonType.BOOKMARK_SAVE);

      if (needToShowRoutingButtons)
      {
        buttons.add(PlacePageButtons.ButtonType.ROUTE_TO);
        if (RoutingController.get().isStopPointAllowed())
          buttons.add(mapObject.getMapObjectType() == MapObject.BOOKMARK
                      ? PlacePageButtons.ButtonType.BOOKMARK_DELETE
                      : PlacePageButtons.ButtonType.BOOKMARK_SAVE);
      }
      buttons.add(PlacePageButtons.ButtonType.SHARE);
    }
    mViewModel.setCurrentButtons(buttons);
  }

  @Override
  public void onChanged(@Nullable MapObject mapObject)
  {
    mMapObject = mapObject;
    if (mapObject != null)
    {
      createPlacePageFragments();
      updateButtons(
          mapObject,
          MapObject.isOfType(MapObject.API_POINT, mMapObject),
          !MapObject.isOfType(MapObject.MY_POSITION, mMapObject));
    }
  }

  @Override
  public void initialize(@Nullable Activity activity)
  {
    Objects.requireNonNull(activity);
    mViewModel.getMapObject().observe((MwmActivity) activity, this);
  }

  @Override
  public void destroy()
  {
    mViewModel.getMapObject().removeObserver(this);
  }

  public interface SlideListener
  {
    void onPlacePageSlide(int top);
  }
}
