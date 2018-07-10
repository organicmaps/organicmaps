package com.mopub.nativeads;

import android.support.annotation.IntDef;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mopub.nativeads.FacebookNative.FacebookStaticNativeAd;
import com.mopub.nativeads.GooglePlayServicesNative.GooglePlayServicesNativeAd;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

public class AdData
{
  public static final int TYPE_NONE = 0;
  public static final int TYPE_FACEBOOK = 1;
  public static final int TYPE_GOOGLE = 2;

  @Retention(RetentionPolicy.SOURCE)
  @IntDef({ TYPE_NONE, TYPE_FACEBOOK, TYPE_GOOGLE })
  public @interface AdDataType
  {
  }

  @AdDataType
  private int mAdType = TYPE_NONE;
  @Nullable
  private String mTitle;
  @Nullable
  private String mText;
  @Nullable
  private String mIconimageUrl;
  @Nullable
  private String mCallToAction;
  @Nullable
  private String mPrivacyInfoUrl;

  public static AdData Make(@NonNull BaseNativeAd src)
  {
    if (src instanceof FacebookStaticNativeAd)
      return new AdData((FacebookStaticNativeAd) src);

    if (src instanceof GooglePlayServicesNativeAd)
      return new AdData((GooglePlayServicesNativeAd) src);

    return new AdData();
  }

  private AdData()
  {
  }

  private AdData(@NonNull FacebookStaticNativeAd src)
  {
    mAdType = TYPE_FACEBOOK;
    mTitle = src.getTitle();
    mText = src.getText();
    mIconimageUrl = src.getIconImageUrl();
    mCallToAction = src.getCallToAction();
    mPrivacyInfoUrl = src.getPrivacyInformationIconClickThroughUrl();
  }

  private AdData(@NonNull GooglePlayServicesNativeAd src)
  {
    mAdType = TYPE_GOOGLE;
    mTitle = src.getTitle();
    mText = src.getText();
    mIconimageUrl = src.getIconImageUrl();
    mCallToAction = src.getCallToAction();
  }

  @AdDataType
  public int getAdType()
  {
    return mAdType;
  }

  @Nullable
  public String getTitle()
  {
    return mTitle;
  }

  @Nullable
  public String getText()
  {
    return mText;
  }

  @Nullable
  public String getIconImageUrl()
  {
    return mIconimageUrl;
  }

  @Nullable
  public String getCallToAction()
  {
    return mCallToAction;
  }

  @Nullable
  public String getPrivacyInfoUrl()
  {
    return mPrivacyInfoUrl;
  }

  public static boolean supports(@NonNull BaseNativeAd ad)
  {
    if (ad instanceof FacebookStaticNativeAd)
      return true;

    return ad instanceof GooglePlayServicesNativeAd
           && ((GooglePlayServicesNativeAd) ad).isNativeContentAd();
  }
}
