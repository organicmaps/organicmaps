package com.mapswithme.maps.promo;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.maps.gallery.RegularAdapterStrategy;

public class PromoEntity extends RegularAdapterStrategy.Item
{
  @NonNull
  private final PromoCityGallery.LuxCategory mCategory;

  @NonNull
  private final String mImageUrl;

  public PromoEntity(int type, @NonNull String title, @Nullable String subtitle,
                     @Nullable String url, @NonNull PromoCityGallery.LuxCategory category,
                     @NonNull String imageUrl)
  {
    super(type, title, subtitle, url);
    mCategory = category;
    mImageUrl = imageUrl;
  }

  @NonNull
  public PromoCityGallery.LuxCategory getCategory()
  {
    return mCategory;
  }

  @NonNull
  public String getImageUrl()
  {
    return mImageUrl;
  }
}
