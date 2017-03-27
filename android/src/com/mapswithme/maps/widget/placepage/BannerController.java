package com.mapswithme.maps.widget.placepage;

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
import com.mapswithme.maps.ads.MwmNativeAd;
import com.mapswithme.maps.ads.NativeAdError;
import com.mapswithme.maps.ads.NativeAdListener;
import com.mapswithme.maps.ads.NativeAdLoader;
import com.mapswithme.maps.bookmarks.data.Banner;
import com.mapswithme.util.Config;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import com.mapswithme.util.statistics.Statistics;

import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static com.mapswithme.util.SharedPropertiesUtils.isShowcaseSwitchedOnLocal;
import static com.mapswithme.util.statistics.Statistics.EventName.PP_BANNER_CLICK;
import static com.mapswithme.util.statistics.Statistics.EventName.PP_BANNER_SHOW;

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
  private Banner mBanner;

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
  private final View mAds;

  private final float mCloseFrameHeight;

  @Nullable
  private final BannerListener mListener;

  private boolean mOpened = false;
  private boolean mError = false;
  @Nullable
  private MwmNativeAd mCurrentAd;
  @NonNull
  private NativeAdLoader mAdsLoader;
  @Nullable
  private AdTracker mAdTracker;

  BannerController(@NonNull View bannerView, @Nullable BannerListener listener,
                   @NonNull NativeAdLoader loader, @Nullable AdTracker tracker)
  {
    LOGGER.d(TAG, "Constructor()");
    mFrame = bannerView;
    mListener = listener;
    Resources resources = mFrame.getResources();
    mCloseFrameHeight = resources.getDimension(R.dimen.placepage_banner_height);
    mIcon = (ImageView) bannerView.findViewById(R.id.iv__banner_icon);
    mTitle = (TextView) bannerView.findViewById(R.id.tv__banner_title);
    mMessage = (TextView) bannerView.findViewById(R.id.tv__banner_message);
    mActionSmall = (TextView) bannerView.findViewById(R.id.tv__action_small);
    mActionLarge = (TextView) bannerView.findViewById(R.id.tv__action_large);
    mAds = bannerView.findViewById(R.id.tv__ads);
    loader.setAdListener(new MyNativeAdsListener());
    mAdsLoader = loader;
    mAdTracker = tracker;
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
    if (mBanner == null || TextUtils.isEmpty(mBanner.getId()))
      return;

    UiUtils.showIf(!hasErrorOccurred(), mFrame);
    if ((mAdsLoader.isAdLoading(mBanner.getId()) || hasErrorOccurred())
        && mCurrentAd == null)
    {
      UiUtils.hide(mIcon, mTitle, mMessage, mActionSmall, mActionLarge, mAds);
      onChangedVisibility(mBanner, false);
    }
    else
    {
      UiUtils.show(mIcon, mTitle, mMessage, mActionSmall, mActionLarge, mAds);
      if (mOpened)
        UiUtils.hide(mActionSmall);
      else
        UiUtils.hide(mActionLarge, mIcon);
    }
  }

  void updateData(@Nullable Banner banner)
  {
    mCurrentAd = null;
    if (mBanner != null && !mBanner.equals(banner))
      onChangedVisibility(mBanner, false);

    UiUtils.hide(mFrame);
    setErrorStatus(false);

    mBanner = banner;
    if (mBanner == null || TextUtils.isEmpty(mBanner.getId()) || !isShowcaseSwitchedOnLocal()
        || Config.getAdForbidden())
    {
      return;
    }

    UiUtils.show(mFrame);

    mAdsLoader.loadAd(mFrame.getContext(), mBanner.getId());
    updateVisibility();
  }

  boolean isBannerVisible()
  {
    return !UiUtils.isHidden(mFrame);
  }

  void open()
  {
    if (!isBannerVisible() || mBanner == null || TextUtils.isEmpty(mBanner.getId()) || mOpened)
      return;

    mOpened = true;
    setFrameHeight(WRAP_CONTENT);
    if (mCurrentAd != null)
      loadIcon(mCurrentAd);
    mMessage.setMaxLines(MAX_MESSAGE_LINES);
    mTitle.setMaxLines(MAX_TITLE_LINES);
    updateVisibility();
    Statistics.INSTANCE.trackFacebookBanner(PP_BANNER_SHOW, mBanner, 1);
  }

  boolean close()
  {
    if (!isBannerVisible() || mBanner == null || !mOpened)
      return false;

    mOpened = false;
    setFrameHeight((int) mCloseFrameHeight);
    UiUtils.hide(mIcon);
    mMessage.setMaxLines(MIN_MESSAGE_LINES);
    mTitle.setMaxLines(MIN_TITLE_LINES);
    updateVisibility();

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

  private void onChangedVisibility(@NonNull Banner banner, boolean isVisible)
  {
    if (TextUtils.isEmpty(banner.getId()))
    {
      LOGGER.e(TAG, "Banner must have a non-null id!", new Throwable());
      return;
    }

    if (mAdTracker == null)
      return;

    if (isVisible)
      mAdTracker.onViewShown(banner.getId());
    else
      mAdTracker.onViewHidden(banner.getId());
  }

  void onChangedVisibility(boolean isVisible)
  {
    if (mBanner != null)
      onChangedVisibility(mBanner, isVisible);
  }

  private void fillViews(@NonNull MwmNativeAd data)
  {
    mTitle.setText(data.getTitle());
    mMessage.setText(data.getDescription());
    mActionSmall.setText(data.getAction());
    mActionLarge.setText(data.getAction());
  }

  private void loadIconAndOpenIfNeeded(@NonNull MwmNativeAd data, @NonNull Banner banner)
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
      Statistics.INSTANCE.trackFacebookBanner(PP_BANNER_SHOW, banner, 0);
    }
    else
    {
      loadIcon(data);
    }
  }

  boolean isActionButtonTouched(@NonNull MotionEvent event)
  {
    return isTouched(mActionSmall, event) || isTouched(mActionLarge, event)
           || isTouched(mTitle, event);
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
      if (mBanner == null || TextUtils.isEmpty(mBanner.getId()))
        return;

      mCurrentAd = ad;

      updateVisibility();

      fillViews(ad);

      ad.registerView(mFrame);

      loadIconAndOpenIfNeeded(ad, mBanner);

      if (mAdTracker != null)
        mAdTracker.onContentObtained(mBanner.getId());

      if (mListener != null && mOpened)
        mListener.onSizeChanged();
    }

    @Override
    public void onError(@NonNull MwmNativeAd ad, @NonNull NativeAdError error)
    {
      if (mBanner == null || TextUtils.isEmpty(mBanner.getId()))
        return;

      boolean isCacheEmpty = mCurrentAd != null;
      setErrorStatus(isCacheEmpty);
      updateVisibility();

      if (mListener != null && isCacheEmpty)
        mListener.onSizeChanged();

      Statistics.INSTANCE.trackNativeAdError(mBanner.getId(), ad, error, mOpened ? 1 : 0);
    }

    @Override
    public void onClick(@NonNull MwmNativeAd ad)
    {
      if (mBanner == null)
        return;

      Statistics.INSTANCE.trackFacebookBanner(PP_BANNER_CLICK, mBanner, mOpened ? 1 : 0);
    }
  }
}
