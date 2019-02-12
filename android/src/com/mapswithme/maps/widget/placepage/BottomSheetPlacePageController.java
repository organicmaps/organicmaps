package com.mapswithme.maps.widget.placepage;

import android.animation.Animator;
import android.animation.ObjectAnimator;
import android.app.Activity;
import android.content.res.Resources;
import android.graphics.Rect;
import android.location.Location;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v7.widget.Toolbar;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.maps.ads.CompoundNativeAdLoader;
import com.mapswithme.maps.ads.DefaultAdTracker;
import com.mapswithme.maps.ads.MwmNativeAd;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.location.LocationListener;
import com.mapswithme.maps.purchase.AdsRemovalPurchaseControllerProvider;
import com.mapswithme.util.NetworkPolicy;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import com.mapswithme.util.statistics.PlacePageTracker;
import com.trafi.anchorbottomsheetbehavior.AnchorBottomSheetBehavior;

public class BottomSheetPlacePageController implements PlacePageController, LocationListener,
                                                       View.OnLayoutChangeListener,
                                                       BannerController.BannerDetailsRequester,
                                                       BannerController.BannerStateListener,
                                                       PlacePageButtonsListener,
                                                       Closable
{
  private static final float ANCHOR_RATIO = 0.3f;
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = BottomSheetPlacePageController.class.getSimpleName();
  private static final String EXTRA_MAP_OBJECT = "extra_map_object";
  private static final int PEEK_HEIGHT_ANIM_DURATION = 300;
  @NonNull
  private final Activity mActivity;
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
  private int mViewportMinHeight;
  private int mCurrentTop;
  private boolean mPeekHeightAnimating;
  private int mOpenBannerTouchSlop;
  /**
   * Represents a value that describes what the part of the banner is opened for details.
   * Must be in the range [0;1]. 0 means that the banner details is completely closed,
   * 1 - the details completely opened.
   */
  private float mBannerRatio;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private BannerController mBannerController;
  @NonNull
  private final AdsRemovalPurchaseControllerProvider mPurchaseControllerProvider;
  @NonNull
  private final SlideListener mSlideListener;
  @NonNull
  private final AnchorBottomSheetBehavior.BottomSheetCallback mSheetCallback
      = new AnchorBottomSheetBehavior.BottomSheetCallback()

  {
    @Override
    public void onStateChanged(@NonNull View bottomSheet, int oldState, int newState)
    {
      LOGGER.d(TAG, "State change, new = " + BottomSheetPlacePageController.toString(newState)
                    + " old = " + BottomSheetPlacePageController.toString(oldState)
                    + " placepage height = " + mPlacePage.getHeight());
      if (isSettlingState(newState) || isDraggingState(newState))
      {
        return;
      }

      if (isHiddenState(newState))
      {
        Framework.nativeDeactivatePopup();
        UiUtils.invisible(mButtonsLayout);
        mPlacePageTracker.onHidden();
      }

      if (isAnchoredState(newState) || isExpandedState(newState))
        mPlacePageTracker.onDetails();

      setPeekHeight();
    }

    @Override
    public void onSlide(@NonNull View bottomSheet, float slideOffset)
    {
      mSlideListener.onPlacePageSlide(bottomSheet.getTop());
      mPlacePageTracker.onMove();

      if (slideOffset < 0)
        return;

      updateViewPortRect();

      resizeBanner();
    }
  };

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

  public BottomSheetPlacePageController(@NonNull Activity activity,
                                        @NonNull AdsRemovalPurchaseControllerProvider provider,
                                        @NonNull SlideListener listener)
  {
    mActivity = activity;
    mPurchaseControllerProvider = provider;
    mSlideListener = listener;
  }

  @Override
  public void initialize()
  {
    Resources res = mActivity.getResources();
    mViewportMinHeight = res.getDimensionPixelSize(R.dimen.viewport_min_height);
    mOpenBannerTouchSlop = res.getDimensionPixelSize(R.dimen.placepage_banner_open_touch_slop);
    mToolbar = mActivity.findViewById(R.id.pp_toolbar);
    UiUtils.extendViewWithStatusBar(mToolbar);
    UiUtils.showHomeUpButton(mToolbar);
    mToolbar.setNavigationOnClickListener(v -> close());
    mPlacePage = mActivity.findViewById(R.id.placepage);
    mPlacePageBehavior = AnchorBottomSheetBehavior.from(mPlacePage);
    mPlacePageBehavior.addBottomSheetCallback(mSheetCallback);
    mPlacePage.addOnLayoutChangeListener(this);
    mPlacePage.addClosable(this);

    ViewGroup bannerContainer = mPlacePage.findViewById(R.id.banner_container);
    DefaultAdTracker tracker = new DefaultAdTracker();
    CompoundNativeAdLoader loader = com.mapswithme.maps.ads.Factory.createCompoundLoader(tracker,
                                                                                         tracker);
    mBannerController = new BannerController(bannerContainer, loader, tracker,
                                             mPurchaseControllerProvider, this, this);

    mButtonsLayout = mActivity.findViewById(R.id.pp_buttons_layout);
    ViewGroup buttons = mButtonsLayout.findViewById(R.id.container);
    mPlacePage.initButtons(buttons, this);
    UiUtils.bringViewToFrontOf(mButtonsLayout, mPlacePage);
    UiUtils.bringViewToFrontOf(mActivity.findViewById(R.id.app_bar), mPlacePage);
    mPlacePageTracker = new PlacePageTracker(mPlacePage, mButtonsLayout);
    LocationHelper.INSTANCE.addListener(this);
  }

  @Override
  public void destroy()
  {
    LocationHelper.INSTANCE.removeListener(this);
  }

  @Override
  public void openFor(@NonNull MapObject object)
  {
    mPlacePage.setMapObject(object, (policy, isSameObject) -> {
      @AnchorBottomSheetBehavior.State
      int state = mPlacePageBehavior.getState();
      // The method openFor could be invoked many times, e.g. when we leave the map and come back
      // on it. So, we should do nothing if the map object is not changed or place page is already
      // opened, but we have to refresh place page views, since the map object data could be changed
      // while we were on the another screen (e.g. editor, ugc, settings, etc.). This behavior is
      // produced by the framework 'UpdatePlacePageInfoForCurrentSelection' method that in turn calls
      // method 'ActivateMapSelection'.
      if (isSameObject && !isHiddenState(state))
      {
        mPlacePage.refreshViews(policy);
        return;
      }

      if (object.isExtendedView())
      {
        mPlacePageBehavior.setState(AnchorBottomSheetBehavior.STATE_EXPANDED);
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
                            && policy.сanUseNetwork();
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

    if (mBannerRatio > 0)
    {
      Log.d(TAG, "Banner is being opened, ignore.");
      return;
    }

    final int peekHeight = getPeekHeight();
    if (peekHeight == mPlacePageBehavior.getPeekHeight())
      return;

    @AnchorBottomSheetBehavior.State
    int currentState = mPlacePageBehavior.getState();
    if (isSettlingState(currentState) || isDraggingState(currentState))
    {
      LOGGER.d(TAG, "Sheet state inappropriate, ignore.");
      return;
    }

    if (isCollapsedState(currentState) && mPlacePageBehavior.getPeekHeight() > 0)
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
    animator.setDuration(PEEK_HEIGHT_ANIM_DURATION);
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

  private int getPeekHeight()
  {
    return mPlacePage.getPreviewHeight() + mButtonsLayout.getHeight();
  }

  @Override
  public void close()
  {
    mPlacePageBehavior.setState(AnchorBottomSheetBehavior.STATE_HIDDEN);
    mPlacePage.reset();
  }

  @Override
  public boolean isClosed()
  {
    return isHiddenState(mPlacePageBehavior.getState());
  }

  @Override
  public void onLocationUpdated(Location location)
  {
    mPlacePage.refreshLocation(location);
  }

  @Override
  public void onCompassUpdated(long time, double magneticNorth, double trueNorth, double accuracy)
  {
    @AnchorBottomSheetBehavior.State
    int currentState = mPlacePageBehavior.getState();
    if (isHiddenState(currentState) || isDraggingState(currentState) || isSettlingState(currentState))
      return;

    double north = trueNorth >= 0.0 ? trueNorth : magneticNorth;
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
    LOGGER.d(TAG, "Layout changed, current state  = " + toString(mPlacePageBehavior.getState()));
    if (mPlacePageBehavior.getPeekHeight() == 0)
    {
      LOGGER.d(TAG, "Layout change ignored, peek height not calculated yet");
      return;
    }

    updateViewPortRect();

    mPlacePage.post(this::setPeekHeight);
  }

  private void updateViewPortRect()
  {
    mPlacePage.post(() -> {
      View coordinatorLayout = (ViewGroup) mPlacePage.getParent();
      int viewPortWidth = coordinatorLayout.getWidth();
      int viewPortHeight = coordinatorLayout.getHeight();
      Rect sheetRect = new Rect();
      mPlacePage.getGlobalVisibleRect(sheetRect);
      if (sheetRect.top < mViewportMinHeight)
        return;

      if (sheetRect.top >= viewPortHeight)
      {
        Framework.nativeSetVisibleRect(0, 0, viewPortWidth, viewPortHeight);
        return;
      }
      viewPortHeight -= sheetRect.height();
      Framework.nativeSetVisibleRect(0, 0, viewPortWidth, viewPortHeight);
    });
  }

  @NonNull
  private static String toString(@AnchorBottomSheetBehavior.State int state)
  {
    switch (state)
    {
      case AnchorBottomSheetBehavior.STATE_EXPANDED:
        return "EXPANDED";
      case AnchorBottomSheetBehavior.STATE_COLLAPSED:
        return "COLLAPSED";
      case AnchorBottomSheetBehavior.STATE_ANCHORED:
        return "ANCHORED";
      case AnchorBottomSheetBehavior.STATE_DRAGGING:
        return "DRAGGING";
      case AnchorBottomSheetBehavior.STATE_SETTLING:
        return "SETTLING";
      case AnchorBottomSheetBehavior.STATE_HIDDEN:
        return "HIDDEN";
      default:
        throw new AssertionError("Unsupported state detected: " + state);
    }
  }

  @Override
  public void onSave(@NonNull Bundle outState)
  {
    mPlacePageTracker.onSave(outState);
    outState.putParcelable(EXTRA_MAP_OBJECT, mPlacePage.getMapObject());
  }

  @Override
  public void onRestore(@NonNull Bundle inState)
  {
    mPlacePageTracker.onRestore(inState);
    if (mPlacePageBehavior.getState() == AnchorBottomSheetBehavior.STATE_HIDDEN)
      return;

    MapObject object = inState.getParcelable(EXTRA_MAP_OBJECT);
    if (object == null)
      return;

    mPlacePage.setMapObject(object, (policy, isSameObject) -> {
      restorePlacePage(object, policy);
    });
    mToolbar.setTitle(object.getTitle());
  }

  private void restorePlacePage(MapObject object, NetworkPolicy policy)
  {
    mPlacePage.post(() -> {
      UiUtils.show(mButtonsLayout);
      setPeekHeight();
      setPlacePageAnchor();
      showBanner(object, policy);
    });
  }

  @Override
  public void onActivityCreated(Activity activity, Bundle savedInstanceState)
  {
    // No op.
  }

  @Override
  public void onActivityStarted(Activity activity)
  {
    mBannerController.attach();
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
  }

  @Override
  public void onActivitySaveInstanceState(Activity activity, Bundle outState)
  {
    // No op.
  }

  @Override
  public void onActivityDestroyed(Activity activity)
  {
    // No op.
  }

  private static boolean isSettlingState(@AnchorBottomSheetBehavior.State int state)
  {
    return state == AnchorBottomSheetBehavior.STATE_SETTLING;
  }

  private static boolean isDraggingState(@AnchorBottomSheetBehavior.State int state)
  {
    return state == AnchorBottomSheetBehavior.STATE_DRAGGING;
  }

  private static boolean isCollapsedState(@AnchorBottomSheetBehavior.State int state)
  {
    return state == AnchorBottomSheetBehavior.STATE_COLLAPSED;
  }

  private static boolean isAnchoredState(@AnchorBottomSheetBehavior.State int state)
  {
    return state == AnchorBottomSheetBehavior.STATE_ANCHORED;
  }

  private static boolean isExpandedState(@AnchorBottomSheetBehavior.State int state)
  {
    return state == AnchorBottomSheetBehavior.STATE_EXPANDED;
  }

  private static boolean isHiddenState(@AnchorBottomSheetBehavior.State int state)
  {
    return state == AnchorBottomSheetBehavior.STATE_HIDDEN;
  }

  @Override
  public boolean shouldShowBannerDetails()
  {
    @AnchorBottomSheetBehavior.State
    int state = mPlacePageBehavior.getState();
    return isAnchoredState(state) || isExpandedState(state);
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
  public void onBookmarkSet(boolean isSet)
  {
    if (!isSet)
      return;

    @AnchorBottomSheetBehavior.State
    int state = mPlacePageBehavior.getState();
    if (!isCollapsedState(state))
      return;

    mPlacePageBehavior.setState(AnchorBottomSheetBehavior.STATE_ANCHORED);
  }

  @Override
  public void closePlacePage()
  {
    close();
  }
}
