package com.mapswithme.maps.widget.placepage;

import android.animation.ArgbEvaluator;
import android.animation.ObjectAnimator;
import android.content.Context;
import android.content.res.Resources;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.FragmentActivity;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

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

import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static com.mapswithme.util.statistics.Statistics.EventName.PP_BANNER_CLICK;
import static com.mapswithme.util.statistics.Statistics.EventName.PP_BANNER_SHOW;
import static com.mapswithme.util.statistics.Statistics.PP_BANNER_STATE_DETAILS;
import static com.mapswithme.util.statistics.Statistics.PP_BANNER_STATE_PREVIEW;


final class BannerController
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE
      .getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = BannerController.class.getName();

  private static final int MAX_MESSAGE_LINES = 100;
  private static final int MIN_MESSAGE_LINES = 3;
  private static final int MAX_TITLE_LINES = 2;
  private static final int MIN_TITLE_LINES = 1;

  private static boolean isTouched(@Nullable View view, @NonNull MotionEvent event)
  {
    return view != null && !UiUtils.isHidden(view) && UiUtils.isViewTouched(event, view);
  }

  @NonNull
  private static View inflateBannerLayout(@NonNull NativeAdWrapper.UiType type, @NonNull ViewGroup containerView)
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

  private final float mCloseFrameHeight;

  @Nullable
  private final BannerListener mListener;

  private boolean mOpened = false;
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

  BannerController(@NonNull ViewGroup bannerContainer, @Nullable BannerListener listener,
                   @NonNull CompoundNativeAdLoader loader, @Nullable AdTracker tracker,
                   @NonNull AdsRemovalPurchaseControllerProvider adsRemovalProvider)
  {
    LOGGER.d(TAG, "Constructor()");
    mContainerView = bannerContainer;
    mContainerView.setOnClickListener(v -> animateActionButton());
    mBannerView = inflateBannerLayout(NativeAdWrapper.UiType.DEFAULT, mContainerView);
    mListener = listener;
    mAdsLoader = loader;
    mAdTracker = tracker;
    Resources resources = mBannerView.getResources();
    mCloseFrameHeight = resources.getDimension(R.dimen.placepage_banner_height);
    mAdsRemovalProvider = adsRemovalProvider;
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
    int state = mOpened ? PP_BANNER_STATE_DETAILS : PP_BANNER_STATE_PREVIEW;
    Statistics.INSTANCE.trackPPBannerClose(state, isCross);
    FragmentActivity activity = (FragmentActivity) mBannerView.getContext();
    AdsRemovalPurchaseDialog.show(activity);
  }

  private void setErrorStatus(boolean value)
  {
    mError = value;
  }

  boolean hasErrorOccurred()
  {
    return mError;
  }

  private void updateVisibility()
  {
    if (mBanners == null)
      return;

    UiUtils.showIf(!hasErrorOccurred(), mContainerView);
    if ((mAdsLoader.isAdLoading() || hasErrorOccurred())
        && mCurrentAd == null)
    {
      UiUtils.hide(mIcon, mTitle, mMessage, mActionSmall, mActionContainer, mActionLarge, mAdChoices,
                   mAdChoicesLabel, mAdsRemovalIcon, mAdsRemovalButton);
    }
    else if (mCurrentAd != null)
    {
      UiUtils.showIf(mCurrentAd.getType().showAdChoiceIcon(), mAdChoices);
      PurchaseController<?> purchaseController
          = mAdsRemovalProvider.getAdsRemovalPurchaseController();
      boolean showRemovalButtons = purchaseController != null
                                   && purchaseController.isPurchaseSupported();
      UiUtils.showIf(showRemovalButtons, mAdsRemovalIcon, mAdsRemovalButton);
      UiUtils.show(mIcon, mTitle, mMessage, mActionSmall, mActionContainer, mActionLarge,
                   mAdsRemovalButton, mAdChoicesLabel);
      if (mOpened)
        UiUtils.hide(mActionSmall);
      else
        UiUtils.hide(mActionContainer, mActionLarge, mAdsRemovalButton, mIcon);
    }
  }

  void updateData(@Nullable List<Banner> banners)
  {
    if (mBanners != null && !mBanners.equals(banners))
    {
      onChangedVisibility(false);
      unregisterCurrentAd();
    }

    UiUtils.hide(mContainerView);
    setErrorStatus(false);

    mBanners = banners != null ? Collections.unmodifiableList(banners) : null;
    if (mBanners == null)
      return;

    UiUtils.show(mContainerView);
    mAdsLoader.loadAd(mContainerView.getContext(), mBanners);
    updateVisibility();
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

  boolean isBannerContainerVisible()
  {
    return UiUtils.isVisible(mContainerView);
  }

  void open()
  {
    if (!isBannerContainerVisible() || mBanners == null || mOpened)
      return;

    mOpened = true;
    setFrameHeight(WRAP_CONTENT);
    mMessage.setMaxLines(MAX_MESSAGE_LINES);
    mTitle.setMaxLines(MAX_TITLE_LINES);
    updateVisibility();
    if (mCurrentAd != null)
    {
      loadIcon(mCurrentAd);
      Statistics.INSTANCE.trackPPBanner(PP_BANNER_SHOW, mCurrentAd, 1);
      mCurrentAd.registerView(mBannerView);
    }

  }

  boolean close()
  {
    if (!isBannerContainerVisible() || mBanners == null || !mOpened)
      return false;

    mOpened = false;
    setFrameHeight((int) mCloseFrameHeight);
    UiUtils.hide(mIcon);
    mMessage.setMaxLines(MIN_MESSAGE_LINES);
    mTitle.setMaxLines(MIN_TITLE_LINES);
    updateVisibility();
    if (mCurrentAd != null)
      mCurrentAd.registerView(mBannerView);
    return true;
  }

  int getLastBannerHeight()
  {
    return mBannerView.getHeight();
  }

  private void setFrameHeight(int height)
  {
    ViewGroup.LayoutParams lp = mBannerView.getLayoutParams();
    lp.height = height;
    mBannerView.setLayoutParams(lp);
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

  private void loadIconAndOpenIfNeeded(@NonNull MwmNativeAd data)
  {
    if (UiUtils.isLandscape(mContainerView.getContext()))
    {
      if (!mOpened)
        open();
      else
        loadIcon(data);
    }
    else if (!mOpened)
    {
      close();
      Statistics.INSTANCE.trackPPBanner(PP_BANNER_SHOW, data, PP_BANNER_STATE_PREVIEW);
    }
    else
    {
      loadIcon(data);
    }
  }

  boolean isActionButtonTouched(@NonNull MotionEvent event)
  {
    return isTouched(mBannerView, event);
  }

  private void animateActionButton()
  {
    View view = mOpened ? mActionLarge : mActionSmall;
    ObjectAnimator animator;
    if (mOpened)
    {
      Context context = mBannerView.getContext();
      Resources res = context.getResources();
      int colorFrom = ThemeUtils.isNightTheme() ? res.getColor(R.color.white_12)
                                                : res.getColor(R.color.black_12);
      int colorTo = ThemeUtils.isNightTheme() ? res.getColor(R.color.white_24)
                                              : res.getColor(R.color.black_24);
      animator = ObjectAnimator.ofObject(view, "backgroundColor", new ArgbEvaluator(),
                                         colorFrom, colorTo, colorFrom);
    }
    else
    {
      animator = ObjectAnimator.ofFloat(view, "alpha", 0.3f, 1f);
    }
    animator.setDuration(300);
    animator.start();
  }

  boolean isOpened()
  {
    return mOpened;
  }

  interface BannerListener
  {
    void onSizeChanged();
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

      mCurrentAd = new NativeAdWrapper(ad);
      if (mLastAdType != mCurrentAd.getType())
      {
        mBannerView = inflateBannerLayout(mCurrentAd.getType(), mContainerView);
        initBannerViews();
      }

      mLastAdType = mCurrentAd.getType();

      updateVisibility();

      fillViews(ad);

      ad.registerView(mBannerView);

      loadIconAndOpenIfNeeded(ad);

      if (mAdTracker != null)
      {
        onChangedVisibility(isBannerContainerVisible());
        mAdTracker.onContentObtained(ad.getProvider(), ad.getBannerId());
      }

      if (mListener != null && mOpened)
        mListener.onSizeChanged();
    }

    @Override
    public void onError(@NonNull String bannerId, @NonNull String provider, @NonNull NativeAdError error)
    {
      if (mBanners == null)
        return;

      boolean isNotCached = mCurrentAd == null;
      setErrorStatus(isNotCached);
      updateVisibility();

      if (mListener != null && isNotCached)
        mListener.onSizeChanged();

      Statistics.INSTANCE.trackPPBannerError(bannerId, provider, error, mOpened ? 1 : 0);
    }

    @Override
    public void onClick(@NonNull MwmNativeAd ad)
    {
      Statistics.INSTANCE.trackPPBanner(PP_BANNER_CLICK, ad,
                                        mOpened ? PP_BANNER_STATE_DETAILS : PP_BANNER_STATE_PREVIEW);
    }
  }
}
