package com.mapswithme.maps.ads;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.mopub.nativeads.BaseNativeAd;
import com.mopub.nativeads.StaticNativeAd;

public abstract class AdDataAdapter<T extends BaseNativeAd>
{
  @NonNull
  private final T mAd;

  protected AdDataAdapter(@NonNull T ad)
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
  @NonNull
  public abstract NetworkType getType();

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

    @NonNull
    @Override
    public NetworkType getType()
    {
      return NetworkType.MOPUB;
    }
  }
}
