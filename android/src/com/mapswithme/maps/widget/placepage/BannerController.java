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

import com.facebook.ads.Ad;
import com.facebook.ads.AdError;
import com.facebook.ads.NativeAd;
import com.mapswithme.maps.R;
import com.mapswithme.maps.ads.FacebookAdsLoader;
import com.mapswithme.maps.bookmarks.data.Banner;
import com.mapswithme.util.Config;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import com.mapswithme.util.statistics.Statistics;

import java.util.ArrayList;
import java.util.List;

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

  @NonNull
  private final FacebookAdsLoader mAdsLoader;

  BannerController(@NonNull View bannerView, @Nullable BannerListener listener)
  {
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
    //TODO: pass as constructor arguments
    mAdsLoader = new FacebookAdsLoader();
    mAdsLoader.setAdsListener(new NativeAdsListener());
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
    if ((mAdsLoader.isAdLoadingForId(mBanner.getId()) || hasErrorOccurred())
        && mAdsLoader.isCacheEmptyForId(mBanner.getId()))
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

    NativeAd data = mAdsLoader.load(mFrame.getContext(), mBanner.getId());
    updateVisibility();

    if (data != null)
    {
      LOGGER.d(TAG, "A cached ad '" + mBanner + "' is shown");
      fillViews(data);
      registerViewsForInteraction(data);
      loadIconAndOpenIfNeeded(data, mBanner);
    }

    if (mOpened && mListener != null)
      mListener.onSizeChanged();
  }

  boolean isBannerVisible()
  {
    return !UiUtils.isHidden(mFrame);
  }

  void open()
  {
    if (!isBannerVisible() || mBanner == null || mOpened)
      return;

    mOpened = true;
    setFrameHeight(WRAP_CONTENT);
    //FIXME NPE
    NativeAd data = mAdsLoader.getAdByIdFromCache(mBanner.getId());
    if (data != null)
      loadIcon(data.getAdIcon());
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

  private void loadIcon(@NonNull NativeAd.Image icon)
  {
    UiUtils.show(mIcon);
    NativeAd.downloadAndDisplayImage(icon, mIcon);
  }

  private void onChangedVisibility(@NonNull Banner banner, boolean isVisible)
  {
  }

  void onChangedVisibility(boolean isVisible)
  {
    if (mBanner != null)
      onChangedVisibility(mBanner, isVisible);
  }

  private void registerViewsForInteraction(@NonNull NativeAd ad)
  {
    List<View> clickableViews = new ArrayList<>();
    clickableViews.add(mTitle);
    clickableViews.add(mActionSmall);
    clickableViews.add(mActionLarge);
    ad.registerViewForInteraction(mFrame, clickableViews);
  }

  private void fillViews(@NonNull NativeAd data)
  {
    mTitle.setText(data.getAdSubtitle());
    mMessage.setText(data.getAdBody());
    mActionSmall.setText(data.getAdCallToAction());
    mActionLarge.setText(data.getAdCallToAction());
  }

  private void loadIconAndOpenIfNeeded(@NonNull NativeAd data, @NonNull Banner banner)
  {
    if (UiUtils.isLandscape(mFrame.getContext()))
    {
      if (!mOpened)
        open();
      else
        loadIcon(data.getAdIcon());
    }
    else if (!mOpened)
    {
      close();
      Statistics.INSTANCE.trackFacebookBanner(PP_BANNER_SHOW, banner, 0);
    }
    else
    {
      loadIcon(data.getAdIcon());
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

  private class NativeAdsListener implements FacebookAdsLoader.FacebookAdsListener
  {
    @Override
    public void onAdClicked(@NonNull Ad ad)
    {
      if (mBanner == null)
        return;

      Statistics.INSTANCE.trackFacebookBanner(PP_BANNER_CLICK, mBanner, mOpened ? 1 : 0);
    }

    @Override
    public void onError(@NonNull Ad ad, @NonNull AdError adError, boolean isCacheEmpty)
    {
      setErrorStatus(isCacheEmpty);
      updateVisibility();

      if (mListener != null && isCacheEmpty)
        mListener.onSizeChanged();
      Statistics.INSTANCE.trackFacebookBannerError(mBanner, adError, mOpened ? 1 : 0);
    }

    @Override
    public void onFacebookAdLoaded(@NonNull NativeAd ad)
    {
      if (mBanner == null)
        return;

      updateVisibility();

      fillViews(ad);

      registerViewsForInteraction(ad);

      loadIconAndOpenIfNeeded(ad, mBanner);

      if (mListener != null && mOpened)
        mListener.onSizeChanged();
    }
  }
}
