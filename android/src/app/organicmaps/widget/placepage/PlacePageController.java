package app.organicmaps.widget.placepage;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.res.Resources;
import android.location.Location;
import android.os.Bundle;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.view.GestureDetectorCompat;
import androidx.fragment.app.Fragment;
import androidx.lifecycle.ViewModelProvider;
import app.organicmaps.Framework;
import app.organicmaps.R;
import app.organicmaps.base.Initializable;
import app.organicmaps.base.Savable;
import app.organicmaps.bookmarks.data.MapObject;
import app.organicmaps.location.LocationHelper;
import app.organicmaps.location.LocationListener;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.Utils;
import app.organicmaps.util.bottomsheet.MenuBottomSheetFragment;
import app.organicmaps.util.bottomsheet.MenuBottomSheetItem;
import app.organicmaps.util.log.Logger;
import com.google.android.material.bottomsheet.BottomSheetBehavior;

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

public class PlacePageController implements Initializable<Activity>,
                                            Savable<Bundle>,
                                            LocationListener,
                                            PlacePageView.OnPlacePageRequestCloseListener,
                                            PlacePageButtons.PlacePageButtonClickListener,
                                            MenuBottomSheetFragment.MenuBottomSheetInterface
{
  private static final String TAG = PlacePageController.class.getSimpleName();

  private static final float PREVIEW_PLUS_RATIO = 0.45f;
  @NonNull
  private final SlideListener mSlideListener;
  @Nullable
  private final RoutingModeListener mRoutingModeListener;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private BottomSheetBehavior<View> mPlacePageBehavior;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private PlacePageView mPlacePage;
  private int mViewportMinHeight;
  private boolean mDeactivateMapSelection = true;
  private AppCompatActivity mMwmActivity;
  private float mButtonsHeight;
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
      mPlacePage.resetScroll();
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
  @NonNull
  private final BottomSheetBehavior.BottomSheetCallback mSheetCallback
      = new DefaultBottomSheetCallback(mBottomSheetChangedListener);
  private int mMaxButtons;
  private PlacePageButtonsViewModel viewModel;

  public PlacePageController(@NonNull SlideListener listener,
                             @Nullable RoutingModeListener routingModeListener)
  {
    mSlideListener = listener;
    mRoutingModeListener = routingModeListener;
  }

  private void onHiddenInternal()
  {
    if (mDeactivateMapSelection)
      Framework.nativeDeactivatePopup();
    mDeactivateMapSelection = true;
    PlacePageUtils.moveViewportUp(mPlacePage, mViewportMinHeight);

    Fragment f = mMwmActivity.getSupportFragmentManager().findFragmentByTag("PLACE_PAGE_BUTTONS");
    if (f != null)
    {
      mMwmActivity.getSupportFragmentManager().beginTransaction()
                  .remove(f)
                  .commit();
    }
  }

  @SuppressLint("ClickableViewAccessibility")
  @Override
  public void initialize(@Nullable Activity activity)
  {
    Objects.requireNonNull(activity);
    mMwmActivity = (AppCompatActivity) activity;
    Resources res = activity.getResources();
    mViewportMinHeight = res.getDimensionPixelSize(R.dimen.viewport_min_height);
    mPlacePage = activity.findViewById(R.id.placepage);
    mPlacePageBehavior = BottomSheetBehavior.from(mPlacePage);
    mPlacePageBehavior.addBottomSheetCallback(mSheetCallback);
    mPlacePageBehavior.setHideable(true);
    mPlacePageBehavior.setState(BottomSheetBehavior.STATE_HIDDEN);
    PlacePageGestureListener ppGestureListener = new PlacePageGestureListener(mPlacePageBehavior);
    GestureDetectorCompat gestureDetector = new GestureDetectorCompat(activity, ppGestureListener);
    mPlacePage.addPlacePageGestureListener(ppGestureListener);
    mPlacePage.setOnTouchListener((v, event) -> gestureDetector.onTouchEvent(event));
    mPlacePage.setOnPlacePageRequestCloseListener(this);
    mPlacePage.setRoutingModeListener(mRoutingModeListener);
    mPlacePage.setOnPlacePageContentChangeListener(this::setPeekHeight);

    UiUtils.bringViewToFrontOf(activity.findViewById(R.id.pp_buttons_fragment), mPlacePage);

    LocationHelper.INSTANCE.addListener(this);

    mPlacePage.requestApplyInsets();

    mButtonsHeight = mMwmActivity.getResources().getDimension(R.dimen.place_page_buttons_height);
    mMaxButtons = mMwmActivity.getResources().getInteger(R.integer.pp_buttons_max);
    viewModel = new ViewModelProvider(mMwmActivity).get(PlacePageButtonsViewModel.class);
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

  @Override
  public void destroy()
  {
    LocationHelper.INSTANCE.removeListener(this);
  }

  public void openFor(@NonNull PlacePageData data)
  {
    mDeactivateMapSelection = true;
    MapObject object = (MapObject) data;
    mPlacePage.setMapObject(object, (isSameObject) -> {
      @BottomSheetBehavior.State
      int state = mPlacePageBehavior.getState();
      if (isSameObject && !PlacePageUtils.isHiddenState(state))
        return;

      mPlacePage.resetScroll();

      if (object.getOpeningMode() == MapObject.OPENING_MODE_DETAILS)
      {
        mPlacePageBehavior.setState(BottomSheetBehavior.STATE_EXPANDED);
        return;
      }
      if (mMwmActivity.getSupportFragmentManager().findFragmentByTag("PLACE_PAGE_BUTTONS") == null)
      {
        mMwmActivity.getSupportFragmentManager().beginTransaction()
                    .add(R.id.pp_buttons_fragment,
                         PlacePageButtons.class, null, "PLACE_PAGE_BUTTONS")
                    .commit();
      }

      mPlacePage.post(() -> {
        setPeekHeight();
        mPlacePageBehavior.setState(BottomSheetBehavior.STATE_COLLAPSED);
      });
    });
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
    // Buttons layout padding is the navigation bar height.
    // Bottom sheets are displayed above it so we need to remove it from the computed size
    final int organicPeekHeight = (int) (mPlacePage.getPreviewHeight() + mButtonsHeight);
    final MapObject object = mPlacePage.getMapObject();
    if (object != null)
    {
      @MapObject.OpeningMode
      int mode = object.getOpeningMode();
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
    mPlacePage.reset();
  }

  public boolean isClosed()
  {
    return PlacePageUtils.isHiddenState(mPlacePageBehavior.getState());
  }

  @Override
  public void onLocationUpdated(@NonNull Location location)
  {
    mPlacePage.refreshLocation(location);
  }

  @Override
  public void onCompassUpdated(double north)
  {
    @BottomSheetBehavior.State
    int currentState = mPlacePageBehavior.getState();
    if (PlacePageUtils.isHiddenState(currentState))
      return;

    mPlacePage.refreshAzimuth(north);
  }

  @Override
  public void onSave(@NonNull Bundle outState)
  {
    outState.putParcelable(PlacePageUtils.EXTRA_PLACE_PAGE_DATA, mPlacePage.getMapObject());
  }

  @Override
  public void onRestore(@NonNull Bundle inState)
  {
    if (mPlacePageBehavior.getState() == BottomSheetBehavior.STATE_HIDDEN)
      return;

    if (!Framework.nativeHasPlacePageInfo())
    {
      close(false);
      return;
    }

    MapObject object = Utils.getParcelable(inState, PlacePageUtils.EXTRA_PLACE_PAGE_DATA, MapObject.class);
    if (object == null)
      return;

    @BottomSheetBehavior.State
    int state = mPlacePageBehavior.getState();
    mPlacePage.setMapObject(object, (isSameObject) -> restorePlacePageState(state));
  }

  private void restorePlacePageState(@BottomSheetBehavior.State int state)
  {
    mPlacePage.post(() -> {
      mPlacePageBehavior.setState(state);
      setPeekHeight();
    });
  }

  @Override
  public void onPlacePageRequestClose()
  {
    close(true);
  }

  @Override
  public void onPlacePageButtonClick(PlacePageButtons.ButtonType item)
  {
    mPlacePage.onPlacePageButtonClick(item);
  }

  public interface SlideListener
  {
    void onPlacePageSlide(int top);
  }
}
