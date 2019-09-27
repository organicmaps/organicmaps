package com.mapswithme.maps.promo;

import androidx.annotation.NonNull;

public class PromoAfterBooking
{
  @NonNull
  private String mId;
  @NonNull
  private String mGuidesUrl;
  @NonNull
  private String mImageUrl;

  // Called from JNI.
  @SuppressWarnings("unused")
  public PromoAfterBooking(@NonNull String id, @NonNull String guidesUrl, @NonNull String imageUrl)
  {
    mId = id;
    mGuidesUrl = guidesUrl;
    mImageUrl = imageUrl;
  }

  @NonNull
  public String getId()
  {
    return mId;
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
