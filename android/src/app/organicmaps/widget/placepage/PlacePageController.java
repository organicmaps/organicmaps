package app.organicmaps.widget.placepage;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.res.Resources;
import android.os.Bundle;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.widget.NestedScrollViewClickFixed;
import androidx.fragment.app.Fragment;
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
  private final int mViewportMinHeight;
  private final AppCompatActivity mMwmActivity;
  private final float mButtonsHeight;
  private final int mMaxButtons;
  private final PlacePageViewModel viewModel;
  private int mPreviewHeight;
  private boolean mDeactivateMapSelection = true;
  @NonNull
  private final BottomSheetChangedListener mBottomSheetChangedListener = new BottomSheetChangedListener()
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
      setPeekHeight();
    }

    @Override
    public void onSheetSliding(int top)
    {
      mSlideListener.onPlacePageSlide(top);
      //  mPlacePageTracker.onMove();
    }

    @Override
    public void onSheetSlideFinish()
    {
      PlacePageUtils.moveViewportUp(mPlacePage, mViewportMinHeight);
    }
  };

  @SuppressLint("ClickableViewAccessibility")
  public PlacePageController(@Nullable Activity activity,
                             @NonNull SlideListener listener)
  {
    mSlideListener = listener;

    Objects.requireNonNull(activity);
    mMwmActivity = (AppCompatActivity) activity;
    Resources res = activity.getResources();
    mViewportMinHeight = res.getDimensionPixelSize(R.dimen.viewport_min_height);
    mPlacePage = activity.findViewById(R.id.placepage);
    mPlacePageBehavior = BottomSheetBehavior.from(mPlacePage);
    BottomSheetBehavior.BottomSheetCallback sheetCallback = new DefaultBottomSheetCallback(mBottomSheetChangedListener);
    mPlacePageBehavior.addBottomSheetCallback(sheetCallback);
    mPlacePageBehavior.setHideable(true);
    mPlacePageBehavior.setState(BottomSheetBehavior.STATE_HIDDEN);

    UiUtils.bringViewToFrontOf(activity.findViewById(R.id.pp_buttons_fragment), mPlacePage);

    mPlacePage.requestApplyInsets();

    mButtonsHeight = mMwmActivity.getResources().getDimension(R.dimen.place_page_buttons_height);
    mMaxButtons = mMwmActivity.getResources().getInteger(R.integer.pp_buttons_max);
    viewModel = new ViewModelProvider(mMwmActivity).get(PlacePageViewModel.class);
  }

  private void onHiddenInternal()
  {
    if (mDeactivateMapSelection)
      Framework.nativeDeactivatePopup();
    mDeactivateMapSelection = true;
    PlacePageUtils.moveViewportUp(mPlacePage, mViewportMinHeight);
    viewModel.setMapObject(null);
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
    viewModel.setMapObject(mapObject);
  }

  private void setPeekHeight()
  {
    final int peekHeight = calculatePeekHeight();
    if (peekHeight == mPlacePageBehavior.getPeekHeight())
      return;

    @BottomSheetBehavior.State
    int currentState = mPlacePageBehavior.getState();
    if (PlacePageUtils.isSettlingState(currentState) || PlacePageUtils.isDraggingState(currentState))
    {
      Logger.d(TAG, "Sheet state inappropriate, ignore.");
      return;
    }

    final boolean shouldAnimate = PlacePageUtils.isCollapsedState(currentState) && mPlacePageBehavior.getPeekHeight() > 0;
    mPlacePageBehavior.setPeekHeight(peekHeight, shouldAnimate);
  }

  private int calculatePeekHeight()
  {
    final int organicPeekHeight = (int) (mPreviewHeight + mButtonsHeight);
    final MapObject mapObject = viewModel.getMapObject().getValue();
    if (mapObject != null)
    {
      @MapObject.OpeningMode
      int mode = mapObject.getOpeningMode();
      if (mode == MapObject.OPENING_MODE_PREVIEW_PLUS)
      {
        View parent = (View) mPlacePage.getParent();
        int promoPeekHeight = (int) (parent.getHeight() * PREVIEW_PLUS_RATIO);
        return Math.max(promoPeekHeight, organicPeekHeight);
      }
    }

    return organicPeekHeight;
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
  public void onPlacePageHeightChange(int previewHeight)
  {
    mPreviewHeight = previewHeight;
    setPeekHeight();
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
    if (PlacePageUtils.isCollapsedState(state))
      mPlacePageBehavior.setState(BottomSheetBehavior.STATE_EXPANDED);
    else if (PlacePageUtils.isExpandedState(state))
      mPlacePageBehavior.setState(BottomSheetBehavior.STATE_COLLAPSED);
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

  private void onMapObjectChange(@Nullable MapObject mapObject)
  {
    if (mapObject == null)
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
    else
    {
      if (mapObject.getOpeningMode() == MapObject.OPENING_MODE_DETAILS)
      {
        mPlacePageBehavior.setState(BottomSheetBehavior.STATE_EXPANDED);
        return;
      }
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
      mPlacePage.post(() -> {
        setPeekHeight();
        mPlacePageBehavior.setState(BottomSheetBehavior.STATE_COLLAPSED);
      });
    }
  }

  @Override
  public void onChanged(MapObject mapObject)
  {
    onMapObjectChange(mapObject);
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
