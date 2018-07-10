package com.mopub.nativeads;

import android.support.annotation.NonNull;
import android.view.View;

import com.google.android.gms.ads.formats.NativeAppInstallAd;
import com.google.android.gms.ads.formats.NativeAppInstallAdView;
import com.google.android.gms.ads.formats.NativeContentAd;
import com.google.android.gms.ads.formats.NativeContentAdView;
import com.mapswithme.maps.R;
import com.mapswithme.util.UiUtils;
import com.mopub.nativeads.GooglePlayServicesNative.GooglePlayServicesNativeAd;

public class GoogleAdDelegate implements AdDelegate
{
  @Override
  public void registerView(@NonNull BaseNativeAd ad, @NonNull View view)
  {
    GooglePlayServicesNativeAd googleAd = (GooglePlayServicesNativeAd) ad;

    if (googleAd.isNativeAppInstallAd())
    {
      registerAppInstallAdView(googleAd.getAppInstallAd(), view, (NativeAppInstallAdView) view);
    }
    else if (googleAd.isNativeContentAd())
    {
      registerContentAdView(googleAd.getContentAd(), view, (NativeContentAdView) view);
    }
  }

  @Override
  public void unregisterView(@NonNull BaseNativeAd ad, @NonNull View view)
  {
    GooglePlayServicesNativeAd googleAd = (GooglePlayServicesNativeAd) ad;

    if (googleAd.isNativeAppInstallAd())
    {
      unregisterAppInstallAdView(googleAd.getAppInstallAd(), (NativeAppInstallAdView) view);
    }
    else if (googleAd.isNativeContentAd())
    {
      unregisterContentAdView(googleAd.getContentAd(), (NativeContentAdView) view);
    }
  }

  private void registerAppInstallAdView(@NonNull NativeAppInstallAd appInstallAd,
                                        @NonNull View view, NativeAppInstallAdView adView)
  {
    // Is not supported yet.
  }

  private void registerContentAdView(@NonNull NativeContentAd contentAd,
                                     @NonNull View view, NativeContentAdView adView)
  {
    adView.setHeadlineView(view.findViewById(R.id.tv__banner_title));
    adView.setBodyView(view.findViewById(R.id.tv__banner_message));
    adView.setLogoView(view.findViewById(R.id.iv__banner_icon));

    View largeAction = view.findViewById(R.id.tv__action_large);
    if (UiUtils.isVisible(largeAction))
      adView.setCallToActionView(largeAction);
    else
      adView.setCallToActionView(view.findViewById(R.id.tv__action_small));

    adView.setNativeAd(contentAd);
  }

  private void unregisterAppInstallAdView(@NonNull NativeAppInstallAd appInstallAd,
                                          @NonNull NativeAppInstallAdView adView)
  {
    // Is not supported yet.
  }

  private void unregisterContentAdView(@NonNull NativeContentAd contentAd,
                                       @NonNull NativeContentAdView adView)
  {
    adView.setHeadlineView(null);
    adView.setBodyView(null);
    adView.setCallToActionView(null);
    adView.setImageView(null);
    adView.setLogoView(null);
  }
}
