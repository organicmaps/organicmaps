package com.mapswithme.maps.widget.placepage;

import android.animation.ArgbEvaluator;
import android.animation.ObjectAnimator;
import android.content.Context;
import android.content.res.Resources;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.text.TextUtils;
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
import com.mapswithme.util.Config;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import com.mapswithme.util.statistics.Statistics;

import java.util.List;

import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static com.mapswithme.util.SharedPropertiesUtils.isShowcaseSwitchedOnLocal;
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

  @Nullable
  private List<Banner> mBanners;
  @NonNull
  private final View mFrame;
  @NonNull
  private final ImageView mIcon;
  @NonNull
  private final TextView mTitle;
  @NonNull
  private final TextView mMessage;
  @NonNull
  private final TextView mActionSmall;
  @NonNull
  private final TextView mActionLarge;
  @NonNull
  private final ImageView mAdChoices;

  private final float mCloseFrameHeight;

  @Nullable
  private final BannerListener mListener;

  private boolean mOpened = false;
  private boolean mError = false;
  @Nullable
  private MwmNativeAd mCurrentAd;
  @NonNull
  private CompoundNativeAdLoader mAdsLoader;
  @Nullable
  private AdTracker mAdTracker;
  @NonNull
  private MyNativeAdsListener mAdsListener = new MyNativeAdsListener();

  BannerController(@NonNull View bannerView, @Nullable BannerListener listener,
                   @NonNull CompoundNativeAdLoader loader, @Nullable AdTracker tracker)
  {
    LOGGER.d(TAG, "Constructor()");
    mFrame = bannerView;
    mListener = listener;
    Resources resources = mFrame.getResources();
    mCloseFrameHeight = resources.getDimension(R.dimen.placepage_banner_height);
    mIcon = bannerView.findViewById(R.id.iv__banner_icon);
    mTitle = bannerView.findViewById(R.id.tv__banner_title);
    mMessage = bannerView.findViewById(R.id.tv__banner_message);
    mActionSmall = bannerView.findViewById(R.id.tv__action_small);
    mActionLarge = bannerView.findViewById(R.id.tv__action_large);
    mAdChoices = bannerView.findViewById(R.id.ad_choices);
    mAdChoices.setOnClickListener(v -> handlePrivacyInfoUrl());
    Resources res = mFrame.getResources();
    UiUtils.expandTouchAreaForView(mAdChoices, (int) res.getDimension(R.dimen.margin_quarter_plus));
    mAdsLoader = loader;
    mAdTracker = tracker;
    mFrame.setOnClickListener(v -> animateActionButton());
  }

  private void handlePrivacyInfoUrl()
  {
    if (mCurrentAd == null)
      return;

    String privacyUrl = mCurrentAd.getPrivacyInfoUrl();
    if (TextUtils.isEmpty(privacyUrl))
      return;

    Utils.openUrl(mFrame.getContext(), privacyUrl);
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

    UiUtils.showIf(!hasErrorOccurred(), mFrame);
    if ((mAdsLoader.isAdLoading() || hasErrorOccurred())
        && mCurrentAd == null)
    {
      UiUtils.hide(mIcon, mTitle, mMessage, mActionSmall, mActionLarge, mAdChoices);
    }
    else
    {
      UiUtils.show(mIcon, mTitle, mMessage, mActionSmall, mActionLarge, mAdChoices);
      if (mOpened)
        UiUtils.hide(mActionSmall);
      else
        UiUtils.hide(mActionLarge, mIcon);
    }
  }

  void updateData(@Nullable List<Banner> banners)
  {
    if (mBanners != null && !mBanners.equals(banners))
    {
      onChangedVisibility(false);
      unregisterCurrentAd();
    }

    UiUtils.hide(mFrame);
    setErrorStatus(false);

    mBanners = banners;
    if (mBanners == null || !isShowcaseSwitchedOnLocal()
        || Config.getAdForbidden())
    {
      return;
    }

    UiUtils.show(mFrame);
    mAdsLoader.loadAd(mFrame.getContext(), mBanners);
    updateVisibility();
  }

  private void unregisterCurrentAd()
  {
    if (mCurrentAd != null)
    {
      LOGGER.d(TAG, "Unregister view for the ad: " + mCurrentAd.getTitle());
      mCurrentAd.unregisterView(mFrame);
      mCurrentAd = null;
    }
  }

  boolean isBannerVisible()
  {
    return !UiUtils.isHidden(mFrame);
  }

  void open()
  {
    if (!isBannerVisible() || mBanners == null || mOpened)
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
      mCurrentAd.registerView(mFrame);
    }

  }

  boolean close()
  {
    if (!isBannerVisible() || mBanners == null || !mOpened)
      return false;

    mOpened = false;
    setFrameHeight((int) mCloseFrameHeight);
    UiUtils.hide(mIcon);
    mMessage.setMaxLines(MIN_MESSAGE_LINES);
    mTitle.setMaxLines(MIN_TITLE_LINES);
    updateVisibility();
    if (mCurrentAd != null)
      mCurrentAd.registerView(mFrame);
    return true;
  }

  int getLastBannerHeight()
  {
    return mFrame.getHeight();
  }

  private void setFrameHeight(int height)
  {
    ViewGroup.LayoutParams lp = mFrame.getLayoutParams();
    lp.height = height;
    mFrame.setLayoutParams(lp);
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
      mCurrentAd.registerView(mFrame);
    }
    else
    {
      mAdTracker.onViewHidden(mCurrentAd.getProvider(), mCurrentAd.getBannerId());
      mCurrentAd.unregisterView(mFrame);
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
    fillAdChoicesIcon(data);
  }

  private void fillAdChoicesIcon(@NonNull MwmNativeAd data)
  {
    MwmNativeAd.NetworkType type = data.getNetworkType();
    if (type == MwmNativeAd.NetworkType.MOPUB)
      mAdChoices.setImageResource(R.drawable.ic_ads_fb);
    else
      mAdChoices.setImageResource(ThemeUtils.getResource(mFrame.getContext(), R.attr.adChoicesIcon));
  }

  private void loadIconAndOpenIfNeeded(@NonNull MwmNativeAd data)
  {
    if (UiUtils.isLandscape(mFrame.getContext()))
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
    return isTouched(mFrame, event);
  }

  private void animateActionButton()
  {
    View view = mOpened ? mFrame.findViewById(R.id.tv__action_large)
                        : mFrame.findViewById(R.id.tv__action_small);
    ObjectAnimator animator;
    if (mOpened)
    {
      Context context = mFrame.getContext();
      Resources res = context.getResources();
      int colorFrom = ThemeUtils.isNightTheme() ? res.getColor(R.color.banner_action_btn_start_anim_night)
                                                : res.getColor(R.color.bg_banner_button);
      int colorTo = ThemeUtils.isNightTheme() ? res.getColor(R.color.banner_action_btn_end_anim_night)
                                              : res.getColor(R.color.banner_action_btn_end_anim);
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
    @Override
    public void onAdLoaded(@NonNull MwmNativeAd ad)
    {
      LOGGER.d(TAG, "onAdLoaded, title = " + ad.getTitle() + " provider = " + ad.getProvider());
      if (mBanners == null)
        return;

      unregisterCurrentAd();

      mCurrentAd = ad;

      updateVisibility();

      fillViews(ad);

      ad.registerView(mFrame);

      loadIconAndOpenIfNeeded(ad);

      if (mAdTracker != null)
      {
        onChangedVisibility(isBannerVisible());
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
