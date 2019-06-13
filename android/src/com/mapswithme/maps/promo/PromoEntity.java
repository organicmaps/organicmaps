package com.mapswithme.maps.promo;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.maps.gallery.RegularAdapterStrategy;

public class PromoEntity extends RegularAdapterStrategy.Item
{
  @NonNull
  private final PromoCityGallery.LuxCategory mCategory;

  public PromoEntity(int type, @NonNull String title, @Nullable String subtitle,
                     @Nullable String url, @NonNull PromoCityGallery.LuxCategory category)
  {
    super(type, title, subtitle, url);
    mCategory = category;
  }

  @NonNull
  public PromoCityGallery.LuxCategory getCategory()
  {
    return mCategory;
  }
}
