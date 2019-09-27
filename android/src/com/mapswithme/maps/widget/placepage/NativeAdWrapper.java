package com.mapswithme.maps.widget.placepage;

import androidx.annotation.LayoutRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import android.view.View;

import com.mapswithme.maps.R;
import com.mapswithme.maps.ads.MwmNativeAd;
import com.mapswithme.maps.ads.NetworkType;

import java.util.EnumMap;
import java.util.Map;

public class NativeAdWrapper implements MwmNativeAd
{
  private final static Map<NetworkType, UiType> TYPES
      = new EnumMap<NetworkType, UiType>(NetworkType.class)
  {
    {
      put(NetworkType.MOPUB, UiType.DEFAULT);
      put(NetworkType.FACEBOOK, UiType.DEFAULT);
      put(NetworkType.MYTARGET, UiType.DEFAULT);
    }
  };

  @NonNull
  private final MwmNativeAd mNativeAd;
  @NonNull
  private final UiType mType;

  NativeAdWrapper(@NonNull MwmNativeAd nativeAd)
  {
    mNativeAd = nativeAd;
    mType = TYPES.get(nativeAd.getNetworkType());
  }

  @NonNull
  @Override
  public String getBannerId()
  {
    return mNativeAd.getBannerId();
  }

  @NonNull
  @Override
  public String getTitle()
  {
    return mNativeAd.getTitle();
  }

  @NonNull
  @Override
  public String getDescription()
  {
    return mNativeAd.getDescription();
  }

  @NonNull
  @Override
  public String getAction()
  {
    return mNativeAd.getAction();
  }

  @Override
  public void loadIcon(@NonNull View view)
  {
    mNativeAd.loadIcon(view);
  }

  @Override
  public void registerView(@NonNull View bannerView)
  {
    mNativeAd.registerView(bannerView);
  }

  @Override
  public void unregisterView(@NonNull View bannerView)
  {
    mNativeAd.unregisterView(bannerView);
  }

  @NonNull
  @Override
  public String getProvider()
  {
    return mNativeAd.getProvider();
  }

  @Nullable
  @Override
  public String getPrivacyInfoUrl()
  {
    return mNativeAd.getPrivacyInfoUrl();
  }

  @NonNull
  @Override
  public NetworkType getNetworkType()
  {
    throw new UnsupportedOperationException("It's not supported for UI!");
  }

  @NonNull
  public UiType getType()
  {
    return mType;
  }

  public enum UiType
  {
    DEFAULT(R.layout.place_page_banner, true);

    @LayoutRes
    private final int mLayoutId;
    private final boolean mShowAdChoiceIcon;

    UiType(int layoutId, boolean showAdChoiceIcon)
    {
      mLayoutId = layoutId;
      mShowAdChoiceIcon = showAdChoiceIcon;
    }

    @LayoutRes
    public int getLayoutId()
    {
      return mLayoutId;
    }

    public boolean showAdChoiceIcon()
    {
      return mShowAdChoiceIcon;
    }
  }
}
