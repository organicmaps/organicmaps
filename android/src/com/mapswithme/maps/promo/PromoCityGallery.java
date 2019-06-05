package com.mapswithme.maps.promo;

import android.support.annotation.NonNull;

public final class PromoCityGallery
{
  @NonNull
  private Item[] mItems;
  @NonNull
  private String mMoreUrl;

  public static final class Item
  {
    @NonNull
    private String mName;
    @NonNull
    private String mUrl;
    @NonNull
    private String mImageUrl;
    @NonNull
    private String mAccess;
    @NonNull
    private String mTier;
    @NonNull
    private Author mAuthor;
    @NonNull
    private LuxCategory mLuxCategory;

    public Item(@NonNull String name, @NonNull String url, @NonNull String imageUrl,
                @NonNull String access, @NonNull String tier, @NonNull Author author,
                @NonNull LuxCategory luxCategory)
    {
      mName = name;
      mUrl = url;
      mImageUrl = imageUrl;
      mAccess = access;
      mTier = tier;
      mAuthor = author;
      mLuxCategory = luxCategory;
    }
  }

  public static final class Author
  {
    @NonNull
    private String mId;
    @NonNull
    private String mName;

    Author(@NonNull String id, @NonNull String name)
    {
      mId = id;
      mName = name;
    }
  }

  public static final class LuxCategory
  {
    @NonNull
    private String mName;
    @NonNull
    private String mColor;

    LuxCategory(@NonNull String name, @NonNull String color)
    {
      mName = name;
      mColor = color;
    }
  }

  PromoCityGallery(@NonNull Item[] items, @NonNull String moreUrl)
  {
    mItems = items;
    mMoreUrl = moreUrl;
  }
}
