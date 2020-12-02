package com.mapswithme.maps.widget.placepage;

import android.animation.ArgbEvaluator;
import android.animation.ObjectAnimator;
import android.content.Context;
import android.content.res.Resources;
import android.text.TextUtils;
import android.util.DisplayMetrics;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.FragmentActivity;
import com.mapswithme.maps.R;
import com.mapswithme.maps.ads.AdTracker;
import com.mapswithme.maps.ads.Banner;
import com.mapswithme.maps.ads.CompoundNativeAdLoader;
import com.mapswithme.maps.ads.MwmNativeAd;
import com.mapswithme.maps.ads.NativeAdError;
import com.mapswithme.maps.ads.NativeAdListener;
import com.mapswithme.maps.purchase.AdsRemovalPurchaseControllerProvider;
import com.mapswithme.maps.purchase.AdsRemovalPurchaseDialog;
import com.mapswithme.maps.purchase.PurchaseController;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import com.mapswithme.util.statistics.Statistics;

import java.util.Collections;
import java.util.List;

import static com.mapswithme.util.statistics.Statistics.EventName.PP_BANNER_CLICK;
import static com.mapswithme.util.statistics.Statistics.PP_BANNER_STATE_DETAILS;
import static com.mapswithme.util.statistics.Statistics.PP_BANNER_STATE_PREVIEW;


final class BannerController implements PlacePageStateListener
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE
      .getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = BannerController.class.getName();

  private static final int MAX_MESSAGE_LINES = 100;
  private static final int MIN_MESSAGE_LINES = 3;
  private static final int MAX_TITLE_LINES = 2;
  private static final int MIN_TITLE_LINES = 1;

  @NonNull
  private static View inflateBannerLayout(@NonNull NativeAdWrapper.UiType type,
                                          @NonNull ViewGroup containerView)
  {
    Context context = containerView.getContext();
    LayoutInflater li = LayoutInflater.from(context);
    View bannerView = li.inflate(type.getLayoutId(), containerView, false);
    containerView.removeAllViews();
    containerView.addView(bannerView);
    return bannerView;
  }

  @Nullable
  private List<Banner> mBanners;
  @NonNull
  private final ViewGroup mContainerView;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private View mBannerView;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private ImageView mIcon;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private TextView mTitle;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private TextView mMessage;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private TextView mActionSmall;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private View mActionContainer;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private TextView mActionLarge;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private ImageView mAdChoices;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private ImageView mAdChoicesLabel;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private View mAdsRemovalIcon;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private View mAdsRemovalButton;
  @Nullable
  private BannerState mState;
  private boolean mError = false;
  @Nullable
  private NativeAdWrapper mCurrentAd;
  @NonNull
  private CompoundNativeAdLoader mAdsLoader;
  @Nullable
  private AdTracker mAdTracker;
  @NonNull
  private MyNativeAdsListener mAdsListener = new MyNativeAdsListener();
  @NonNull
  private final AdsRemovalPurchaseControllerProvider mAdsRemovalProvider;
  private int mClosedHeight;
  private int mOpenedHeight;
  @NonNull
  private final BannerStateRequester mBannerStateRequester;
  @NonNull
  private final BannerStateListener mBannerStateListener;

  BannerController(@NonNull ViewGroup bannerContainer, @NonNull CompoundNativeAdLoader loader,
                   @Nullable AdTracker tracker,
                   @NonNull AdsRemovalPurchaseControllerProvider adsRemovalProvider,
                   @NonNull BannerStateRequester bannerStateRequester,
                   @NonNull BannerStateListener bannerStateListener)
  {
    LOGGER.d(TAG, "Constructor()");
    mContainerView = bannerContainer;
    mContainerView.setOnClickListener(v -> animateActionButton());
    mBannerView = inflateBannerLayout(NativeAdWrapper.UiType.DEFAULT, mContainerView);
    mAdsLoader = loader;
    mAdTracker = tracker;
    mAdsRemovalProvider = adsRemovalProvider;
    mBannerStateRequester = bannerStateRequester;
    mBannerStateListener = bannerStateListener;
    initBannerViews();
  }

  private void initBannerViews()
  {
    mIcon = mBannerView.findViewById(R.id.iv__banner_icon);
    mTitle = mBannerView.findViewById(R.id.tv__banner_title);
    mMessage = mBannerView.findViewById(R.id.tv__banner_message);
    mActionSmall = mBannerView.findViewById(R.id.tv__action_small);
    mActionContainer = mBannerView.findViewById(R.id.action_container);
    mActionLarge = mActionContainer.findViewById(R.id.tv__action_large);
    mAdsRemovalButton = mActionContainer.findViewById(R.id.tv__action_remove);
    mAdsRemovalButton.setOnClickListener(this::handleAdsRemoval);
    mAdChoices = mBannerView.findViewById(R.id.ad_choices_icon);
    mAdChoices.setOnClickListener(v -> handlePrivacyInfoUrl());
    mAdChoicesLabel = mBannerView.findViewById(R.id.ad_choices_label);
    mAdsRemovalIcon = mBannerView.findViewById(R.id.remove_btn);
    mAdsRemovalIcon.setOnClickListener(this::handleAdsRemoval);
    expandTouchArea();
  }

  private void expandTouchArea()
  {
    Resources res = mBannerView.getResources();
    final int tapArea = res.getDimensionPixelSize(R.dimen.margin_quarter_plus);
    UiUtils.expandTouchAreaForViews(tapArea, mAdChoices);
    int crossArea = res.getDimensionPixelSize(R.dimen.margin_base_plus);
    UiUtils.expandTouchAreaForView(mAdsRemovalIcon,  tapArea, crossArea, tapArea, crossArea);
  }

  private void handlePrivacyInfoUrl()
  {
    if (mCurrentAd == null)
      return;

    String privacyUrl = mCurrentAd.getPrivacyInfoUrl();
    if (TextUtils.isEmpty(privacyUrl))
      return;

    Utils.openUrl(mBannerView.getContext(), privacyUrl);
  }

  private void handleAdsRemoval(@NonNull View clickedView)
  {
    boolean isCross = clickedView.getId() == R.id.remove_btn;
    @Statistics.BannerState
    int state = isDetailsState(mState) ? PP_BANNER_STATE_DETAILS
                                       : PP_BANNER_STATE_PREVIEW;
    Statistics.INSTANCE.trackPPBannerClose(state, isCross);
    FragmentActivity activity = (FragmentActivity) mBannerView.getContext();
    AdsRemovalPurchaseDialog.show(activity);
  }

  private void setErrorStatus(boolean value)
  {
    mError = value;
  }

  private boolean hasErrorOccurred()
  {
    return mError;
  }

  private void updateVisibility()
  {
    if (mBanners == null)
      throw new AssertionError("Banners must be non-null at this point!");

    UiUtils.hideIf(hasErrorOccurred() || mCurrentAd == null, mContainerView);

    if (mCurrentAd == null)
      throw new AssertionError("Banners must be non-null at this point!");

    UiUtils.showIf(mCurrentAd.getType().showAdChoiceIcon(), mAdChoices);
    PurchaseController<?> purchaseController
        = mAdsRemovalProvider.getAdsRemovalPurchaseController();
    boolean showRemovalButtons = purchaseController != null
                                 && purchaseController.isPurchaseSupported();
    UiUtils.showIf(showRemovalButtons, mAdsRemovalIcon, mAdsRemovalButton);
    UiUtils.show(mIcon, mTitle, mMessage, mActionSmall, mActionContainer, mActionLarge,
                 mAdsRemovalButton, mAdChoicesLabel);
    if (isDetailsState(mState))
      UiUtils.hide(mActionSmall);
    else
      UiUtils.hide(mActionContainer, mActionLarge, mAdsRemovalButton, mIcon);
    UiUtils.show(mBannerView);
  }

  void updateData(@Nullable List<Banner> banners)
  {
    if (mBanners != null && !mBanners.equals(banners))
    {
      onChangedVisibility(false);
      unregisterCurrentAd();
    }

    setErrorStatus(false);

    mBanners = banners != null ? Collections.unmodifiableList(banners) : null;
    UiUtils.showIf(mBanners != null, mContainerView);
    if (mBanners == null)
      return;

    UiUtils.hide(mBannerView);
    mAdsLoader.loadAd(mContainerView.getContext(), mBanners);
  }

  private void unregisterCurrentAd()
  {
    if (mCurrentAd != null)
    {
      LOGGER.d(TAG, "Unregister view for the ad: " + mCurrentAd.getTitle());
      mCurrentAd.unregisterView(mBannerView);
      mCurrentAd = null;
    }
  }

  private boolean isBannerContainerVisible()
  {
    return UiUtils.isVisible(mContainerView);
  }

  void open()
  {
    if (!isBannerContainerVisible() || mBanners == null || isDetailsState(mState))
      return;

    setOpenedStateInternal();

    if (mCurrentAd != null)
    {
      loadIcon(mCurrentAd);
      mCurrentAd.registerView(mBannerView);
      mBannerStateListener.onBannerDetails(mCurrentAd);
    }
  }

  void zoomIn(float ratio)
  {
    ViewGroup banner = mContainerView.findViewById(R.id.banner);
    ViewGroup.LayoutParams lp = banner.getLayoutParams();
    lp.height = (int) ((mOpenedHeight - mClosedHeight) * ratio + mClosedHeight);
    banner.setLayoutParams(lp);
  }

  void zoomOut(float ratio)
  {
    ViewGroup banner = mContainerView.findViewById(R.id.banner);
    ViewGroup.LayoutParams lp = banner.getLayoutParams();
    lp.height = (int) (mClosedHeight - (mClosedHeight - mOpenedHeight) * ratio);
    banner.setLayoutParams(lp);
  }

  void close()
  {
    if (!isBannerContainerVisible() || mBanners == null)
      return;

    setClosedStateInternal();

    if (mCurrentAd != null)
    {
      mCurrentAd.registerView(mBannerView);
      mBannerStateListener.onBannerPreview(mCurrentAd);
    }
  }

  private void discardBannerSize()
  {
    zoomOut(0);
  }

  private void measureBannerSizes()
  {
    DisplayMetrics dm = mContainerView.getResources().getDisplayMetrics();
    final float screenWidth = dm.widthPixels;

    int heightMeasureSpec = View.MeasureSpec.makeMeasureSpec(0, View.MeasureSpec.UNSPECIFIED);
    int widthMeasureSpec = View.MeasureSpec.makeMeasureSpec((int) screenWidth, View.MeasureSpec.AT_MOST);

    setClosedStateInternal();
    mBannerView.measure(widthMeasureSpec, heightMeasureSpec);
    mClosedHeight = mBannerView.getMeasuredHeight();
    LOGGER.d(TAG, "Banner close height = " + mClosedHeight);

    setOpenedStateInternal();
    mBannerView.measure(widthMeasureSpec, heightMeasureSpec);
    mOpenedHeight = mBannerView.getMeasuredHeight();
    LOGGER.d(TAG, "Banner open height = " + mOpenedHeight);
  }

  private void setOpenedStateInternal()
  {
    mState = BannerState.DETAILS;
    mMessage.setMaxLines(MAX_MESSAGE_LINES);
    mTitle.setMaxLines(MAX_TITLE_LINES);
    updateVisibility();
  }

  private void setClosedStateInternal()
  {
    mState = BannerState.PREVIEW;
    UiUtils.hide(mIcon);
    mMessage.setMaxLines(MIN_MESSAGE_LINES);
    mTitle.setMaxLines(MIN_TITLE_LINES);
    updateVisibility();
  }

  private void loadIcon(@NonNull MwmNativeAd ad)
  {
    UiUtils.show(mIcon);
    ad.loadIcon(mIcon);
  }

  void onChangedVisibility(boolean isVisible)
  {
    if (mAdTracker == null || mCurrentAd == null)
      return;

    if (isVisible)
    {
      mAdTracker.onViewShown(mCurrentAd.getProvider(), mCurrentAd.getBannerId());
      mCurrentAd.registerView(mBannerView);
    }
    else
    {
      mAdTracker.onViewHidden(mCurrentAd.getProvider(), mCurrentAd.getBannerId());
      mCurrentAd.unregisterView(mBannerView);
    }
  }

  void detach()
  {
    mAdsLoader.detach();
    mAdsLoader.setAdListener(null);
  }

  void attach()
  {
    mAdsLoader.setAdListener(mAdsListener);
  }

  private void fillViews(@NonNull MwmNativeAd data)
  {
    mTitle.setText(data.getTitle());
    mMessage.setText(data.getDescription());
    mActionSmall.setText(data.getAction());
    mActionLarge.setText(data.getAction());
  }

  private void animateActionButton()
  {
    ObjectAnimator animator;
    if (isDetailsState(mState))
    {
      Context context = mBannerView.getContext();
      Resources res = context.getResources();
      int colorFrom = ThemeUtils.isNightTheme(context) ? res.getColor(R.color.bg_banner_action_button_night)
                                                       : res.getColor(R.color.bg_banner_action_button);
      int colorTo = ThemeUtils.isNightTheme(context) ? res.getColor(R.color.bg_banner_action_button_pressed_night)
                                                     : res.getColor(R.color.bg_banner_action_button_pressed);
      animator = ObjectAnimator.ofObject(mActionLarge, "backgroundColor", new ArgbEvaluator(),
                                         colorFrom, colorTo, colorFrom);
    }
    else
    {
      animator = ObjectAnimator.ofFloat(mActionSmall, "alpha", 0.3f, 1f);
    }
    animator.setDuration(300);
    animator.start();
  }

  private static boolean isDetailsState(@Nullable BannerState state)
  {
    return state == BannerState.DETAILS;
  }

  private static boolean isPreviewState(@Nullable BannerState state)
  {
    return state == BannerState.PREVIEW;
  }

  boolean hasAd()
  {
    return mCurrentAd != null;
  }

  int getClosedHeight()
  {
    return mClosedHeight;
  }

  private void setBannerState(@Nullable BannerState state)
  {
    if (mCurrentAd == null)
      throw new AssertionError("Current ad must be non-null at this point!");

    if (state == null)
    {
      LOGGER.d(TAG, "Banner state not determined yet, discard banner size");
      setBannerInitialHeight(0);
      mState = null;
      return;
    }

    if (isDetailsState(state))
    {
      open();
      loadIcon(mCurrentAd);
      setBannerInitialHeight(mOpenedHeight);
      return;
    }

    if (isPreviewState(state))
    {
      close();
      setBannerInitialHeight(mClosedHeight);
      mBannerStateListener.onBannerPreview(mCurrentAd);
    }
  }

  private void setBannerInitialHeight(int height)
  {
    LOGGER.d(TAG, "Set banner initial height = " + height);
    ViewGroup banner = mContainerView.findViewById(R.id.banner);
    ViewGroup.LayoutParams lp = banner.getLayoutParams();
    lp.height = height;
    banner.setLayoutParams(lp);
  }

  private void onPlacePageStateChanged()
  {
    if (mCurrentAd == null)
      return;

    BannerState newState =  mBannerStateRequester.requestBannerState();
    setBannerState(newState);
  }

  @Override
  public void onPlacePageDetails()
  {
    onPlacePageStateChanged();
  }

  @Override
  public void onPlacePagePreview()
  {
    onPlacePageStateChanged();
  }

  @Override
  public void onPlacePageClosed()
  {
    // Do nothing.
  }

  private class MyNativeAdsListener implements NativeAdListener
  {
    @Nullable
    private NativeAdWrapper.UiType mLastAdType;

    @Override
    public void onAdLoaded(@NonNull MwmNativeAd ad)
    {
      LOGGER.d(TAG, "onAdLoaded, ad = " + ad);
      if (mBanners == null)
        return;

      unregisterCurrentAd();
      discardBannerSize();

      mCurrentAd = new NativeAdWrapper(ad);
      if (mLastAdType != mCurrentAd.getType())
      {
        mBannerView = inflateBannerLayout(mCurrentAd.getType(), mContainerView);
        initBannerViews();
      }

      mLastAdType = mCurrentAd.getType();

      fillViews(ad);
      measureBannerSizes();
      BannerState state = mBannerStateRequester.requestBannerState();
      setBannerState(state);
      ad.registerView(mBannerView);

      if (mAdTracker != null)
      {
        onChangedVisibility(isBannerContainerVisible());
        mAdTracker.onContentObtained(ad.getProvider(), ad.getBannerId());
      }
    }

    @Override
    public void onError(@NonNull String bannerId, @NonNull String provider,
                        @NonNull NativeAdError error)
    {
      if (mBanners == null)
        return;

      boolean isNotCached = mCurrentAd == null;
      setErrorStatus(isNotCached);
      UiUtils.hide(mContainerView);

      Statistics.INSTANCE.trackPPBannerError(bannerId, provider, error,
                                             isDetailsState(mState) ? 1 : 0);
    }

    @Override
    public void onClick(@NonNull MwmNativeAd ad)
    {
      Statistics.INSTANCE.trackPPBanner(PP_BANNER_CLICK, ad,
                                        isDetailsState(mState) ? PP_BANNER_STATE_DETAILS
                                                               : PP_BANNER_STATE_PREVIEW);
    }
  }

  interface BannerStateRequester
  {
    @Nullable
    BannerState requestBannerState();
  }

  enum BannerState
  {
    PREVIEW,
    DETAILS
  }

  interface BannerStateListener
  {
    void onBannerDetails(@NonNull MwmNativeAd ad);
    void onBannerPreview(@NonNull MwmNativeAd ad);
  }
}
