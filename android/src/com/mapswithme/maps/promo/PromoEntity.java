package com.mapswithme.maps.promo;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.mapswithme.maps.gallery.RegularAdapterStrategy;

public class PromoEntity extends RegularAdapterStrategy.Item
{
  @Nullable
  private final PromoCityGallery.LuxCategory mCategory;

  @NonNull
  private final String mImageUrl;

  public PromoEntity(int type, @NonNull String title, @Nullable String subtitle,
                     @Nullable String url, @Nullable PromoCityGallery.LuxCategory category,
                     @NonNull String imageUrl)
  {
    super(type, title, subtitle, url);
    mCategory = category;
    mImageUrl = imageUrl;
  }

  @Nullable
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
