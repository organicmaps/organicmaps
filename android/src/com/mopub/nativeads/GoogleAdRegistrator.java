package com.mopub.nativeads;

import android.support.annotation.NonNull;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;

import com.google.android.gms.ads.formats.NativeAppInstallAd;
import com.google.android.gms.ads.formats.NativeAppInstallAdView;
import com.google.android.gms.ads.formats.NativeContentAd;
import com.google.android.gms.ads.formats.NativeContentAdView;
import com.mapswithme.maps.R;
import com.mapswithme.util.UiUtils;
import com.mopub.nativeads.GooglePlayServicesNative.GooglePlayServicesNativeAd;

public class GoogleAdRegistrator implements AdRegistrator
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
    View largeAction = view.findViewById(R.id.tv__action_large);

    adView.setCallToActionView(UiUtils.isVisible(largeAction)
                               ? largeAction
                               : view.findViewById(R.id.tv__action_small));

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
    adView.setCallToActionView(null);
  }
}
