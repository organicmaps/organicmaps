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
import com.facebook.ads.AdListener;
import com.facebook.ads.AdSettings;
import com.facebook.ads.NativeAd;
import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.Banner;
import com.mapswithme.util.Config;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import com.mapswithme.util.statistics.Statistics;

import java.util.ArrayList;
import java.util.EnumSet;
import java.util.List;

import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static com.mapswithme.util.SharedPropertiesUtils.isShowcaseSwitchedOnLocal;

final class BannerController implements AdListener
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

  @Nullable
  private NativeAd mNativeAd;

  private boolean mOpened = false;
  private boolean mProgress = false;
  private boolean mError = false;

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
  }

  private void showProgress()
  {
    mProgress = true;
    updateVisibility();
  }

  private void hideProgress()
  {
    mProgress = false;
    updateVisibility();
  }

  private boolean isDownloading()
  {
    return mProgress;
  }

  private void showError(boolean value)
  {
    mError = value;
  }

  private boolean hasErrorOccurred()
  {
    return mError;
  }

  private void updateVisibility()
  {
    UiUtils.showIf(!hasErrorOccurred(), mFrame);
    if (isDownloading() || hasErrorOccurred())
    {
      UiUtils.hide(mIcon, mTitle, mMessage, mActionSmall, mActionLarge, mAds);
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
    UiUtils.hide(mFrame);
    showError(false);

    mBanner = banner;
    if (mBanner == null || TextUtils.isEmpty(mBanner.getId()) || !isShowcaseSwitchedOnLocal()
        || Config.getAdForbidden())
      return;

    if (BuildConfig.DEBUG || BuildConfig.BUILD_TYPE.equals("beta"))
    {
      AdSettings.addTestDevice("cbbc8cd2b6564ea727b5ca56bcf22622");
      AdSettings.addTestDevice("c36b141fff9e11866d8cf9c601d2b7e0");
      AdSettings.addTestDevice("189055740336d9d2687f41a775eaf867");
      AdSettings.addTestDevice("36dd04f33c4cf92e3b7d21e9a5a9d985");
      AdSettings.addTestDevice("b39d3c00580d17b291ff4e161a423525");
      AdSettings.addTestDevice("51bae0d8b2ba8290659840f9098e3026");
      AdSettings.addTestDevice("583a068aa7293c6743855343251e1fad");
      AdSettings.addTestDevice("c1c9f7f1c06b8c2c4e15607e5c766cb3");
    }
    mNativeAd = new NativeAd(mFrame.getContext(), mBanner.getId());
    mNativeAd.setAdListener(BannerController.this);
    mNativeAd.loadAd(EnumSet.of(NativeAd.MediaCacheFlag.ICON));
    UiUtils.show(mFrame);
    showProgress();
  }

  boolean isBannerVisible()
  {
    return !UiUtils.isHidden(mFrame);
  }

  void open()
  {
    if (!isBannerVisible() || mNativeAd == null || mBanner == null || mOpened)
      return;

    mOpened = true;
    setFrameHeight(WRAP_CONTENT);
    loadIcon(mNativeAd);
    mMessage.setMaxLines(MAX_MESSAGE_LINES);
    mTitle.setMaxLines(MAX_TITLE_LINES);
    updateVisibility();

    Statistics.INSTANCE.trackEvent(Statistics.EventName.PP_BANNER_SHOW,
                                   Statistics.params()
                                             .add("banner:", mBanner.getId())
                                             .add("state:", "1"));
  }

  boolean close()
  {
    if (!isBannerVisible() || mNativeAd == null || mBanner == null || !mOpened)
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

  private void loadIcon(@NonNull NativeAd nativeAd)
  {
    UiUtils.show(mIcon);
    NativeAd.Image icon = nativeAd.getAdIcon();
    NativeAd.downloadAndDisplayImage(icon, mIcon);
  }

  @Override
  public void onError(Ad ad, AdError adError)
  {
    showError(true);
    updateVisibility();
    if (mListener != null)
      mListener.onSizeChanged();
    LOGGER.e(TAG, adError.getErrorMessage());
  }

  @Override
  public void onAdLoaded(Ad ad)
  {
    if (mNativeAd == null || mBanner == null)
      return;

    hideProgress();
    updateVisibility();

    mTitle.setText(mNativeAd.getAdTitle());
    mMessage.setText(mNativeAd.getAdBody());
    mActionSmall.setText(mNativeAd.getAdCallToAction());
    mActionLarge.setText(mNativeAd.getAdCallToAction());

    List<View> clickableViews = new ArrayList<>();
    clickableViews.add(mTitle);
    clickableViews.add(mActionSmall);
    clickableViews.add(mActionLarge);
    mNativeAd.registerViewForInteraction(mFrame, clickableViews);

    if (UiUtils.isLandscape(mFrame.getContext()))
    {
      if (!mOpened)
        open();
      else
        loadIcon(mNativeAd);
    }
    else if (!mOpened)
    {
      close();
      Statistics.INSTANCE.trackEvent(Statistics.EventName.PP_BANNER_SHOW,
                                     Statistics.params()
                                               .add("banner:", mBanner.getId())
                                               .add("state:", "0"));
    }
    else
    {
      loadIcon(mNativeAd);
    }

    if (mListener != null && mOpened)
      mListener.onSizeChanged();
  }

  @Override
  public void onAdClicked(Ad ad)
  {
    if (mBanner == null)
      return;

    Statistics.INSTANCE.trackEvent(Statistics.EventName.PP_BANNER_CLICK,
                                   Statistics.params()
                                             .add("banner:", mBanner.getId())
                                             .add("state:", mOpened ? "1" : "0"));
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
}
