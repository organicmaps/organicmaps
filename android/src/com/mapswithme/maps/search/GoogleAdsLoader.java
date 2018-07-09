package com.mapswithme.maps.search;

import android.content.Context;
import android.content.res.TypedArray;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.google.ads.mediation.admob.AdMobAdapter;
import com.google.android.gms.ads.AdListener;
import com.google.android.gms.ads.AdSize;
import com.google.android.gms.ads.search.SearchAdRequest;
import com.google.android.gms.ads.search.SearchAdView;
import com.mapswithme.maps.R;
import com.mapswithme.maps.ads.GoogleSearchAd;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.concurrency.UiThread;

class GoogleAdsLoader
{
  private long mLoadingDelay;
  @NonNull
  private final Bundle mStyleParams = new Bundle();
  @Nullable
  private GoogleSearchAd mGoogleSearchAd;
  @Nullable
  private Runnable mLoadingTask;
  @NonNull
  private String mQuery = "";
  @Nullable
  private AdvertLoadingListener mLoadingListener;

  GoogleAdsLoader(@NonNull Context context, long loadingDelay)
  {
    this.mLoadingDelay = loadingDelay;
    initStyle(context);
  }

  void scheduleAdsLoading(@NonNull final Context context, @NonNull final String query)
  {
    cancelAdsLoading();
    mQuery = query;

    mGoogleSearchAd = new GoogleSearchAd();
    if (mGoogleSearchAd.getAdUnitId().isEmpty())
      return;

    mLoadingTask = new Runnable()
    {
      @Override
      public void run()
      {
        performLoading(context);
      }
    };
    UiThread.runLater(mLoadingTask, mLoadingDelay);
  }

  void cancelAdsLoading()
  {
    if (mLoadingTask != null)
    {
      UiThread.cancelDelayedTasks(mLoadingTask);
      mLoadingTask = null;
    }
  }

  private void updateAdView(SearchAdView searchAdView)
  {
    SearchAdRequest.Builder builder = new SearchAdRequest.Builder()
      .setQuery(mQuery)
      .addNetworkExtrasBundle(AdMobAdapter.class, mStyleParams);

    searchAdView.loadAd(builder.build());
  }

  void attach(@NonNull AdvertLoadingListener listener)
  {
    mLoadingListener = listener;
  }

  void detach()
  {
    mLoadingListener = null;
  }

  private void initStyle(@NonNull Context context)
  {
    TypedArray attrs = context.obtainStyledAttributes(ThemeUtils.isNightTheme() ?
      R.style.GoogleAdsDark : R.style.GoogleAdsLight, R.styleable.GoogleAds);

    mStyleParams.putString("csa_width", "auto");
    mStyleParams.putString("csa_colorLocation", attrs.getString(R.styleable.GoogleAds_colorLocation));
    mStyleParams.putString("csa_fontSizeLocation", attrs.getString(R.styleable.GoogleAds_fontSizeLocation));
    mStyleParams.putString("csa_clickToCall", attrs.getString(R.styleable.GoogleAds_clickToCall));
    mStyleParams.putString("csa_location", attrs.getString(R.styleable.GoogleAds_location));
    mStyleParams.putString("csa_sellerRatings", attrs.getString(R.styleable.GoogleAds_sellerRatings));
    mStyleParams.putString("csa_siteLinks", attrs.getString(R.styleable.GoogleAds_siteLinks));
    mStyleParams.putString("csa_number", attrs.getString(R.styleable.GoogleAds_number));
    mStyleParams.putString("csa_fontSizeAnnotation", attrs.getString(R.styleable.GoogleAds_fontSizeAnnotation));
    mStyleParams.putString("csa_fontSizeAttribution", attrs.getString(R.styleable.GoogleAds_fontSizeAttribution));
    mStyleParams.putString("csa_fontSizeDescription", attrs.getString(R.styleable.GoogleAds_fontSizeDescription));
    mStyleParams.putString("csa_fontSizeDomainLink", attrs.getString(R.styleable.GoogleAds_fontSizeDomainLink));
    mStyleParams.putString("csa_fontSizeTitle", attrs.getString(R.styleable.GoogleAds_fontSizeTitle));
    mStyleParams.putString("csa_colorAdBorder", attrs.getString(R.styleable.GoogleAds_colorAdBorder));
    mStyleParams.putString("csa_colorAnnotation", attrs.getString(R.styleable.GoogleAds_colorAnnotation));
    mStyleParams.putString("csa_colorAttribution", attrs.getString(R.styleable.GoogleAds_colorAttribution));
    mStyleParams.putString("csa_colorBackground", attrs.getString(R.styleable.GoogleAds_colorBackground));
    mStyleParams.putString("csa_colorDomainLink", attrs.getString(R.styleable.GoogleAds_colorDomainLink));
    mStyleParams.putString("csa_colorText", attrs.getString(R.styleable.GoogleAds_colorText));
    mStyleParams.putString("csa_colorTitleLink", attrs.getString(R.styleable.GoogleAds_colorTitleLink));
    mStyleParams.putString("csa_attributionSpacingBelow", attrs.getString(R.styleable.GoogleAds_attributionSpacingBelow));
    mStyleParams.putString("csa_noTitleUnderline", attrs.getString(R.styleable.GoogleAds_noTitleUnderline));
    mStyleParams.putString("csa_titleBold", attrs.getString(R.styleable.GoogleAds_titleBold));
    attrs.recycle();
  }

  private void performLoading(@NonNull Context context)
  {
    if (mGoogleSearchAd == null)
      throw new AssertionError("mGoogleSearchAd can't be null here");

    final SearchAdView view = new SearchAdView(context);
    view.setAdSize(AdSize.SEARCH);
    view.setAdUnitId(mGoogleSearchAd.getAdUnitId());
    updateAdView(view);
    view.setAdListener(new AdListener()
    {
      @Override
      public void onAdLoaded()
      {
        mLoadingTask = null;
        if (mLoadingListener != null)
        {
          mLoadingListener.onLoadingFinished(view);
        }
      }

      @Override
      public void onAdFailedToLoad(int i)
      {
        mLoadingTask = null;
      }
    });
  }

  interface AdvertLoadingListener
  {
    void onLoadingFinished(@NonNull SearchAdView searchAdView);
  }
}
