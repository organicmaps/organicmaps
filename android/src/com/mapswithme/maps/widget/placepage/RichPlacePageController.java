package com.mapswithme.maps.widget.placepage;

import android.animation.Animator;
import android.animation.ObjectAnimator;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.res.Resources;
import android.location.Location;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.widget.Toolbar;
import androidx.core.view.GestureDetectorCompat;
import com.google.android.material.appbar.AppBarLayout;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.maps.ads.CompoundNativeAdLoader;
import com.mapswithme.maps.ads.DefaultAdTracker;
import com.mapswithme.maps.ads.Factory;
import com.mapswithme.maps.ads.MwmNativeAd;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.bookmarks.data.RoadWarningMarkType;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.location.LocationListener;
import com.mapswithme.maps.promo.Promo;
import com.mapswithme.maps.purchase.AdsRemovalPurchaseControllerProvider;
import com.mapswithme.util.NetworkPolicy;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import com.mapswithme.util.statistics.PlacePageTracker;
import com.trafi.anchorbottomsheetbehavior.AnchorBottomSheetBehavior;

import java.util.Objects;

public class RichPlacePageController implements PlacePageController, LocationListener,
                                                View.OnLayoutChangeListener,
                                                BannerController.BannerStateRequester,
                                                BannerController.BannerStateListener,
                                                Closable
{
  private static final float ANCHOR_RATIO = 0.3f;
  private static final float PREVIEW_PLUS_RATIO = 0.45f;
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = RichPlacePageController.class.getSimpleName();
  private static final int ANIM_BANNER_APPEARING_MS = 300;
  private static final int ANIM_CHANGE_PEEK_HEIGHT_MS = 100;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private AnchorBottomSheetBehavior<View> mPlacePageBehavior;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private View mButtonsLayout;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private PlacePageView mPlacePage;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private PlacePageTracker mPlacePageTracker;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private Toolbar mToolbar;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private AppBarLayout mToolbarLayout;
  private int mViewportMinHeight;
  private int mCurrentTop;
  private boolean mPeekHeightAnimating;
  private int mOpenBannerTouchSlop;
  /**
   * Represents a value that describes how much banner details are opened.
   * Must be in the range [0;1]. 0 means that the banner details are completely closed,
   * 1 - the details are completely opened.
   */
  private float mBannerRatio;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private BannerController mBannerController;
  @NonNull
  private final AdsRemovalPurchaseControllerProvider mPurchaseControllerProvider;
  @NonNull
  private final SlideListener mSlideListener;
  @Nullable
  private final RoutingModeListener mRoutingModeListener;
  @NonNull
  private final BottomSheetChangedListener mBottomSheetChangedListener = new BottomSheetChangedListener()
  {
    @Override
    public void onSheetHidden()
    {
      onHiddenInternal();
    }

    @Override
    public void onSheetDirectionIconChange()
    {
      PlacePageUtils.setPullDrawable(mPlacePageBehavior, mPlacePage, R.id.pull_icon);
    }

    @Override
    public void onSheetDetailsOpened()
    {
      mBannerController.onPlacePageDetails();
      mPlacePageTracker.onDetails();
      UiUtils.show(mToolbarLayout);
    }

    @Override
    public void onSheetCollapsed()
    {
      mPlacePage.resetScroll();
      mBannerController.onPlacePagePreview();
      setPeekHeight();
      UiUtils.show(mToolbarLayout);
    }

    @Override
    public void onSheetSliding(int top)
    {
      mSlideListener.onPlacePageSlide(top);
      mPlacePageTracker.onMove();
    }

    @Override
    public void onSheetSlideFinish()
    {
      PlacePageUtils.moveViewportUp(mPlacePage, mViewportMinHeight);
      resizeBanner();
    }
  };

  @NonNull
  private final AnchorBottomSheetBehavior.BottomSheetCallback mSheetCallback
      = new DefaultBottomSheetCallback(mBottomSheetChangedListener);

  private boolean mDeactivateMapSelection = true;

  private void onHiddenInternal()
  {
    if (mDeactivateMapSelection)
      Framework.nativeDeactivatePopup();
    mDeactivateMapSelection = true;
    PlacePageUtils.moveViewportUp(mPlacePage, mViewportMinHeight);
    UiUtils.invisible(mButtonsLayout);
    mPlacePageTracker.onHidden();
    UiUtils.hide(mToolbarLayout);
  }

  private void resizeBanner()
  {
    int lastTop = mCurrentTop;
    mCurrentTop = mPlacePage.getTop();

    if (!mBannerController.hasAd())
      return;

    int bannerMaxY = calculateBannerMaxY();
    int bannerMinY = calculateBannerMinY();
    int maxDistance = Math.abs(bannerMaxY - bannerMinY);
    int yDistance = Math.abs(mCurrentTop - bannerMinY);
    float ratio = (float) yDistance / maxDistance;
    mBannerRatio = ratio;

    if (ratio >= 1)
    {
      mBannerController.zoomOut(1);
      mBannerController.open();
      return;
    }

    if (ratio == 0)
    {
      mBannerController.zoomIn(ratio);
      mBannerController.close();
      return;
    }

    if (mCurrentTop < lastTop)
      mBannerController.zoomOut(ratio);
    else
      mBannerController.zoomIn(ratio);
  }

  private int calculateBannerMaxY()
  {
    View coordinatorLayout = (ViewGroup) mPlacePage.getParent();
    int height = coordinatorLayout.getHeight();
    int maxY = mPlacePage.getHeight() > height * (1 - ANCHOR_RATIO)
               ? (int) (height * ANCHOR_RATIO) : height - mPlacePage.getHeight();
    return maxY + mOpenBannerTouchSlop;
  }

  private int calculateBannerMinY()
  {
    View coordinatorLayout = (ViewGroup) mPlacePage.getParent();
    int height = coordinatorLayout.getHeight();
    return height - mPlacePageBehavior.getPeekHeight();
  }

  RichPlacePageController(@NonNull AdsRemovalPurchaseControllerProvider provider,
                          @NonNull SlideListener listener,
                          @Nullable RoutingModeListener routingModeListener)
  {
    mPurchaseControllerProvider = provider;
    mSlideListener = listener;
    mRoutingModeListener = routingModeListener;
  }

  @SuppressLint("ClickableViewAccessibility")
  @Override
  public void initialize(@Nullable Activity activity)
  {
    Objects.requireNonNull(activity);
    Resources res = activity.getResources();
    mViewportMinHeight = res.getDimensionPixelSize(R.dimen.viewport_min_height);
    mOpenBannerTouchSlop = res.getDimensionPixelSize(R.dimen.placepage_banner_open_touch_slop);
    mToolbar = activity.findViewById(R.id.pp_toolbar);
    mToolbarLayout = activity.findViewById(R.id.app_bar);
    UiUtils.extendViewWithStatusBar(mToolbar);
    UiUtils.showHomeUpButton(mToolbar);
    mToolbar.setNavigationOnClickListener(v -> close(true));
    mPlacePage = activity.findViewById(R.id.placepage);
    mPlacePageBehavior = AnchorBottomSheetBehavior.from(mPlacePage);
    mPlacePageBehavior.addBottomSheetCallback(mSheetCallback);
    GestureDetectorCompat gestureDetector
        = new GestureDetectorCompat(activity, new PlacePageGestureListener(mPlacePageBehavior));
    mPlacePage.setOnTouchListener((v, event) -> gestureDetector.onTouchEvent(event));
    mPlacePage.addOnLayoutChangeListener(this);
    mPlacePage.addClosable(this);
    mPlacePage.setRoutingModeListener(mRoutingModeListener);
    ViewGroup bannerContainer = mPlacePage.findViewById(R.id.banner_container);
    DefaultAdTracker tracker = new DefaultAdTracker();
    CompoundNativeAdLoader loader = Factory.createCompoundLoader(tracker,
                                                                 tracker);
    mBannerController = new BannerController(bannerContainer, loader, tracker,
                                             mPurchaseControllerProvider, this, this);

    mButtonsLayout = activity.findViewById(R.id.pp_buttons_layout);
    ViewGroup buttons = mButtonsLayout.findViewById(R.id.container);
    mPlacePage.initButtons(buttons);
    UiUtils.bringViewToFrontOf(mButtonsLayout, mPlacePage);
    UiUtils.bringViewToFrontOf(activity.findViewById(R.id.app_bar), mPlacePage);
    mPlacePageTracker = new PlacePageTracker(mPlacePage, mButtonsLayout);
    LocationHelper.INSTANCE.addListener(this);
  }

  @Override
  public void destroy()
  {
    LocationHelper.INSTANCE.removeListener(this);
  }

  @Override
  public void openFor(@NonNull PlacePageData data)
  {
    mDeactivateMapSelection = true;
    MapObject object = (MapObject) data;
    mPlacePage.setMapObject(object, (policy, isSameObject) -> {
      @AnchorBottomSheetBehavior.State
      int state = mPlacePageBehavior.getState();
      if (isSameObject && !PlacePageUtils.isHiddenState(state))
        return;

      mBannerRatio = 0;
      mPlacePage.resetScroll();

      if (object.getOpeningMode() == MapObject.OPENING_MODE_DETAILS)
      {
        mPlacePageBehavior.setState(AnchorBottomSheetBehavior.STATE_ANCHORED);
        return;
      }

      UiUtils.show(mButtonsLayout);
      openPlacePage();
      showBanner(object, policy);
    });

    mToolbar.setTitle(object.getTitle());
    mPlacePageTracker.setMapObject(object);
    Framework.logLocalAdsEvent(Framework.LocalAdsEventType.LOCAL_ADS_EVENT_OPEN_INFO, object);
  }

  private void showBanner(@NonNull MapObject object, NetworkPolicy policy)
  {
    boolean canShowBanner = object.getMapObjectType() != MapObject.MY_POSITION
                            && policy.canUseNetwork()
                            && object.getRoadWarningMarkType() == RoadWarningMarkType.UNKNOWN;
    mBannerController.updateData(canShowBanner ? object.getBanners() : null);
  }

  private void openPlacePage()
  {
    mPlacePage.post(() -> {
      setPeekHeight();
      mPlacePageBehavior.setState(AnchorBottomSheetBehavior.STATE_COLLAPSED);
      setPlacePageAnchor();
    });
  }

  private void setPeekHeight()
  {
    if (mPeekHeightAnimating)
    {
      Log.d(TAG, "Peek animation in progress, ignore.");
      return;
    }

    // If banner details are little bit or completely opened we haven't to change the peek height,
    // because the peek height is reasonable only for collapsed state and banner details are always
    // closed in collapsed state.
    if (mBannerRatio > 0)
      return;

    final int peekHeight = calculatePeekHeight();
    if (peekHeight == mPlacePageBehavior.getPeekHeight())
      return;

    @AnchorBottomSheetBehavior.State
    int currentState = mPlacePageBehavior.getState();
    if (PlacePageUtils.isSettlingState(currentState) || PlacePageUtils.isDraggingState(currentState))
    {
      LOGGER.d(TAG, "Sheet state inappropriate, ignore.");
      return;
    }

    if (PlacePageUtils.isCollapsedState(currentState) && mPlacePageBehavior.getPeekHeight() > 0)
    {
      setPeekHeightAnimatedly(peekHeight);
      return;
    }

    mPlacePageBehavior.setPeekHeight(peekHeight);
  }

  private void setPeekHeightAnimatedly(int peekHeight)
  {
    int delta = peekHeight - mPlacePageBehavior.getPeekHeight();
    ObjectAnimator animator = ObjectAnimator.ofFloat(mPlacePage, "translationY", -delta);
    animator.setDuration(delta == mBannerController.getClosedHeight() ? ANIM_BANNER_APPEARING_MS
                                                                      : ANIM_CHANGE_PEEK_HEIGHT_MS);
    animator.addListener(new UiUtils.SimpleAnimatorListener()
    {
      @Override
      public void onAnimationStart(Animator animation)
      {
        mPeekHeightAnimating = true;
        mPlacePage.setScrollable(false);
        mPlacePageBehavior.setAllowUserDragging(false);
      }

      @Override
      public void onAnimationEnd(Animator animation)
      {
        mPlacePage.setTranslationY(0);
        mPeekHeightAnimating = false;
        mPlacePage.setScrollable(true);
        mPlacePageBehavior.setAllowUserDragging(true);
        mPlacePageBehavior.setPeekHeight(peekHeight);
      }
    });
    animator.addUpdateListener(animation -> onUpdateTranslation());

    animator.start();
  }

  private void onUpdateTranslation()
  {
    mSlideListener.onPlacePageSlide((int) (mPlacePage.getTop() + mPlacePage.getTranslationY()));
  }

  private void setPlacePageAnchor()
  {
    View parent = (View) mPlacePage.getParent();
    mPlacePageBehavior.setAnchorOffset((int) (parent.getHeight() * ANCHOR_RATIO));
  }

  private int calculatePeekHeight()
  {
    final int organicPeekHeight = mPlacePage.getPreviewHeight() + mButtonsLayout.getHeight();
    final MapObject object = mPlacePage.getMapObject();
    if (object != null)
    {
      @MapObject.OpeningMode
      int mode = object.getOpeningMode();
      if (mode == MapObject.OPENING_MODE_PREVIEW_PLUS)
      {
        View parent = (View) mPlacePage.getParent();
        int promoPeekHeight = (int) (parent.getHeight() * PREVIEW_PLUS_RATIO);
        return promoPeekHeight <= organicPeekHeight ? organicPeekHeight : promoPeekHeight;
      }
    }

    return organicPeekHeight;
  }

  @Override
  public void close(boolean deactivateMapSelection)
  {
    mDeactivateMapSelection = deactivateMapSelection;
    mPlacePageBehavior.setState(AnchorBottomSheetBehavior.STATE_HIDDEN);
    mPlacePage.reset();
  }

  @Override
  public boolean isClosed()
  {
    return PlacePageUtils.isHiddenState(mPlacePageBehavior.getState());
  }

  @Override
  public void onLocationUpdated(Location location)
  {
    mPlacePage.refreshLocation(location);
  }

  @Override
  public void onCompassUpdated(long time, double north)
  {
    @AnchorBottomSheetBehavior.State
    int currentState = mPlacePageBehavior.getState();
    if (PlacePageUtils.isHiddenState(currentState) || PlacePageUtils.isDraggingState(currentState)
        || PlacePageUtils.isSettlingState(currentState))
      return;

    mPlacePage.refreshAzimuth(north);
  }

  @Override
  public void onLocationError(int errorCode)
  {
    // Do nothing by default.
  }

  @Override
  public void onLayoutChange(View v, int left, int top, int right, int bottom, int oldLeft, int
      oldTop, int oldRight, int oldBottom)
  {
    if (mPlacePageBehavior.getPeekHeight() == 0)
    {
      LOGGER.d(TAG, "Layout change ignored, peek height not calculated yet");
      return;
    }

    mPlacePage.post(this::setPeekHeight);

    if (PlacePageUtils.isHiddenState(mPlacePageBehavior.getState()))
      return;

    PlacePageUtils.moveViewportUp(mPlacePage, mViewportMinHeight);
  }

  @Override
  public void onSave(@NonNull Bundle outState)
  {
    mPlacePageTracker.onSave(outState);
    outState.putParcelable(PlacePageUtils.EXTRA_PLACE_PAGE_DATA, mPlacePage.getMapObject());
  }

  @Override
  public void onRestore(@NonNull Bundle inState)
  {
    mPlacePageTracker.onRestore(inState);
    if (mPlacePageBehavior.getState() == AnchorBottomSheetBehavior.STATE_HIDDEN)
      return;

    if (!Framework.nativeHasPlacePageInfo())
    {
      close(false);
      return;
    }

    MapObject object = inState.getParcelable(PlacePageUtils.EXTRA_PLACE_PAGE_DATA);
    if (object == null)
      return;

    @AnchorBottomSheetBehavior.State
    int state = mPlacePageBehavior.getState();
    mPlacePage.setMapObject(object, (policy, isSameObject) -> {
      restorePlacePageState(object, policy, state);
    });
    mToolbar.setTitle(object.getTitle());
  }

  private void restorePlacePageState(@NonNull MapObject object, @NonNull NetworkPolicy policy,
                                     @AnchorBottomSheetBehavior.State int state)
  {
    mPlacePage.post(() -> {
      setPlacePageAnchor();
      mPlacePageBehavior.setState(state);
      UiUtils.show(mButtonsLayout);
      setPeekHeight();
      showBanner(object, policy);
      PlacePageUtils.setPullDrawable(mPlacePageBehavior, mPlacePage, R.id.pull_icon);
    });
  }

  @Override
  public void onActivityCreated(Activity activity, Bundle savedInstanceState)
  {
  }

  @Override
  public void onActivityStarted(Activity activity)
  {
    mBannerController.attach();
    mPlacePage.attach(activity);
  }

  @Override
  public void onActivityResumed(Activity activity)
  {
    mBannerController.onChangedVisibility(true);
  }

  @Override
  public void onActivityPaused(Activity activity)
  {
    mBannerController.onChangedVisibility(false);
  }

  @Override
  public void onActivityStopped(Activity activity)
  {
    mBannerController.detach();
    mPlacePage.detach();
  }

  @Override
  public void onActivitySaveInstanceState(Activity activity, Bundle outState)
  {
    // No op.
  }

  @Override
  public void onActivityDestroyed(Activity activity)
  {
    Promo.INSTANCE.setListener(null);
  }

  @Nullable
  @Override
  public BannerController.BannerState requestBannerState()
  {
    @AnchorBottomSheetBehavior.State
    int state = mPlacePageBehavior.getState();
    if (PlacePageUtils.isSettlingState(state) || PlacePageUtils.isDraggingState(state)
        || PlacePageUtils.isHiddenState(state))
      return null;

    if (PlacePageUtils.isAnchoredState(state) || PlacePageUtils.isExpandedState(state))
      return BannerController.BannerState.DETAILS;

    return BannerController.BannerState.PREVIEW;
  }

  @Override
  public void onBannerDetails(@NonNull MwmNativeAd ad)
  {
    mPlacePageTracker.onBannerDetails(ad);
  }

  @Override
  public void onBannerPreview(@NonNull MwmNativeAd ad)
  {
    mPlacePageTracker.onBannerPreview(ad);
  }

  @Override
  public void closePlacePage()
  {
    close(true);
  }

  @Override
  public boolean support(@NonNull PlacePageData data)
  {
    return data instanceof MapObject;
  }
}
