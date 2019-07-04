package com.mapswithme.maps.promo;

import android.support.annotation.NonNull;

public class PromoAfterBooking
{
  @NonNull
  private String mGuidesUrl;
  @NonNull
  private String mImageUrl;

  // Called from JNI.
  @SuppressWarnings("unused")
  public PromoAfterBooking(@NonNull String guidesUrl, @NonNull String imageUrl)
  {
    mGuidesUrl = guidesUrl;
    mImageUrl = imageUrl;
  }

  @NonNull
  public String getGuidesUrl()
  {
    return mGuidesUrl;
  }

  @NonNull
  public String getImageUrl()
  {
    return mImageUrl;
  }
}
