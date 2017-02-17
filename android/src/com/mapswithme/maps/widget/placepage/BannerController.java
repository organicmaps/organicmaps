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

  private static boolean isTouched(@Nullable View view, @NonNull MotionEvent event)
  {
    return view != null && !UiUtils.isHidden(view) && UiUtils.isViewTouched(event, view);
  }

  @Nullable
  private Banner mBanner;

  @NonNull
  private final View mFrame;
  @Nullable
  private final ImageView mIcon;
  @Nullable
  private final TextView mTitle;
  @Nullable
  private final TextView mMessage;
  @Nullable
  private final TextView mActionSmall;
  @Nullable
  private final TextView mActionLarge;

  private final float mCloseFrameHeight;

  @Nullable
  private NativeAd mNativeAd;

  private boolean mOpened = false;

  BannerController(@NonNull View bannerView)
  {
    mFrame = bannerView;
    Resources resources = mFrame.getResources();
    mCloseFrameHeight = resources.getDimension(R.dimen.placepage_banner_height);
    mIcon = (ImageView) bannerView.findViewById(R.id.iv__banner_icon);
    mTitle = (TextView) bannerView.findViewById(R.id.tv__banner_title);
    mMessage = (TextView) bannerView.findViewById(R.id.tv__banner_message);
    mActionSmall = (TextView) bannerView.findViewById(R.id.tv__action_small);
    mActionLarge = (TextView) bannerView.findViewById(R.id.tv__action_large);
  }

  void updateData(@Nullable Banner banner)
  {
    UiUtils.hide(mFrame);

    mBanner = banner;
    if (mBanner == null || TextUtils.isEmpty(mBanner.getId()) || !isShowcaseSwitchedOnLocal()
        || Config.getAdForbidden())
      return;

    if (BuildConfig.DEBUG)
    {
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
    if (mMessage != null)
      mMessage.setMaxLines(Integer.MAX_VALUE);
    if (mTitle != null)
      mTitle.setMaxLines(2);
    if (mActionSmall != null)
      UiUtils.hide(mActionSmall);
    if (mActionLarge != null)
      UiUtils.show(mActionLarge);

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
    if (mMessage != null)
      mMessage.setMaxLines(3);
    if (mTitle != null)
      mTitle.setMaxLines(1);
    if (mActionSmall != null)
      UiUtils.show(mActionSmall);
    if (mActionLarge != null)
      UiUtils.hide(mActionLarge);

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
    if (mIcon == null)
      return;

    UiUtils.show(mIcon);
    NativeAd.Image icon = nativeAd.getAdIcon();
    NativeAd.downloadAndDisplayImage(icon, mIcon);
  }

  @Override
  public void onError(Ad ad, AdError adError)
  {
    UiUtils.hide(mFrame);
    LOGGER.e(TAG, adError.getErrorMessage());
  }

  @Override
  public void onAdLoaded(Ad ad)
  {
    if (mNativeAd == null || mBanner == null)
      return;

    if (mTitle != null)
      mTitle.setText(mNativeAd.getAdTitle());
    if (mMessage != null)
      mMessage.setText(mNativeAd.getAdBody());
    if (mActionSmall != null)
      mActionSmall.setText(mNativeAd.getAdCallToAction());
    if (mActionLarge != null)
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
}
