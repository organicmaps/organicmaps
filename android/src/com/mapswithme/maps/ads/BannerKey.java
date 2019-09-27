package com.mapswithme.maps.ads;

import androidx.annotation.NonNull;

import net.jcip.annotations.Immutable;

@Immutable
class BannerKey
{
  @NonNull
  private final String mProvider;
  @NonNull
  private final String mId;

  BannerKey(@NonNull String provider, @NonNull String id)
  {
    mProvider = provider;
    mId = id;
  }

  @Override
  public String toString()
  {
    return "BannerKey{" +
           "mProvider='" + mProvider + '\'' +
           ", mId='" + mId + '\'' +
           '}';
  }

  @Override
  public boolean equals(Object o)
  {
    if (this == o) return true;
    if (o == null || getClass() != o.getClass()) return false;

    BannerKey bannerKey = (BannerKey) o;

    if (!mProvider.equals(bannerKey.mProvider)) return false;
    return mId.equals(bannerKey.mId);
  }

  @Override
  public int hashCode()
  {
    int result = mProvider.hashCode();
    result = 31 * result + mId.hashCode();
    return result;
  }
}
