package com.mopub.nativeads;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

public abstract class AdDataAdapter<T extends BaseNativeAd>
{
  @NonNull
  private T mAd;

  private AdDataAdapter(@NonNull T ad)
  {
    mAd = ad;
  }

  @NonNull
  protected T getAd()
  {
    return mAd;
  }

  @Nullable
  public abstract String getTitle();
  @Nullable
  public abstract String getText();
  @Nullable
  public abstract String getIconImageUrl();
  @Nullable
  public abstract String getCallToAction();
  @Nullable
  public abstract String getPrivacyInfoUrl();

  public static class StaticAd extends AdDataAdapter<StaticNativeAd>
  {
    public StaticAd(@NonNull StaticNativeAd ad)
    {
      super(ad);
    }

    @Nullable
    @Override
    public String getTitle()
    {
      return getAd().getTitle();
    }

    @Nullable
    @Override
    public String getText()
    {
      return getAd().getText();
    }

    @Nullable
    @Override
    public String getIconImageUrl()
    {
      return getAd().getIconImageUrl();
    }

    @Nullable
    @Override
    public String getCallToAction()
    {
      return getAd().getCallToAction();
    }

    @Nullable
    @Override
    public String getPrivacyInfoUrl()
    {
      return getAd().getPrivacyInformationIconClickThroughUrl();
    }
  }

  public static class GoogleAd extends AdDataAdapter<GooglePlayServicesNative.GooglePlayServicesNativeAd>
  {
    public GoogleAd(@NonNull GooglePlayServicesNative.GooglePlayServicesNativeAd ad)
    {
      super(ad);
    }

    @Nullable
    @Override
    public String getTitle()
    {
      return getAd().getTitle();
    }

    @Nullable
    @Override
    public String getText()
    {
      return getAd().getText();
    }

    @Nullable
    @Override
    public String getIconImageUrl()
    {
      return getAd().getIconImageUrl();
    }

    @Nullable
    @Override
    public String getCallToAction()
    {
      return getAd().getCallToAction();
    }

    @Nullable
    @Override
    public String getPrivacyInfoUrl()
    {
      return null;
    }
  }
}
