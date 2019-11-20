package com.mapswithme.maps.promo;

import android.graphics.Color;
import androidx.annotation.ColorInt;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

public final class PromoCityGallery
{
  @NonNull
  private final Item[] mItems;
  @NonNull
  private final String mMoreUrl;
  @NonNull
  private final String mCategory;

  PromoCityGallery(@NonNull Item[] items, @NonNull String moreUrl, @NonNull String category)
  {
    mItems = items;
    mMoreUrl = moreUrl;
    mCategory = category;
  }

  @NonNull
  public Item[] getItems()
  {
    return mItems;
  }

  @NonNull
  public String getMoreUrl()
  {
    return mMoreUrl;
  }

  @NonNull
  public String getCategory()
  {
    return mCategory;
  }

  public static final class Item
  {
    @NonNull
    private final String mName;
    @NonNull
    private final String mUrl;
    @NonNull
    private final String mImageUrl;
    @NonNull
    private final String mAccess;
    @NonNull
    private final String mTier;
    @NonNull
    private final String mTourCategory;
    @NonNull
    private final Place mPlace;
    @NonNull
    private final Author mAuthor;
    @Nullable
    private final LuxCategory mLuxCategory;

    public Item(@NonNull String name, @NonNull String url, @NonNull String imageUrl,
                @NonNull String access, @NonNull String tier, @NonNull String tourCategory,
                @NonNull Place place, @NonNull Author author, @Nullable LuxCategory luxCategory)
    {
      mName = name;
      mUrl = url;
      mImageUrl = imageUrl;
      mAccess = access;
      mTier = tier;
      mTourCategory = tourCategory;
      mPlace = place;
      mAuthor = author;
      mLuxCategory = luxCategory;
    }

    @NonNull
    public String getName()
    {
      return mName;
    }

    @NonNull
    public String getUrl()
    {
      return mUrl;
    }

    @NonNull
    public String getImageUrl()
    {
      return mImageUrl;
    }

    @NonNull
    public String getAccess()
    {
      return mAccess;
    }

    @NonNull
    public String getTier()
    {
      return mTier;
    }

    @NonNull
    public String getTourCategory()
    {
      return mTourCategory;
    }

    @NonNull
    public Place getPlace()
    {
      return mPlace;
    }

    @NonNull
    public Author getAuthor()
    {
      return mAuthor;
    }

    @Nullable
    public LuxCategory getLuxCategory()
    {
      return mLuxCategory;
    }

  }

  public static final class Place
  {
    @NonNull
    private String mName;

    @NonNull
    private String mDescription;

    Place(@NonNull String name, @NonNull String description)
    {
      mName = name;
      mDescription = description;
    }

    @NonNull
    public String getName()
    {
      return mName;
    }

    @NonNull
    public String getDescription()
    {
      return mDescription;
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

    @NonNull
    public String getId()
    {
      return mId;
    }

    @NonNull
    public String getName()
    {
      return mName;
    }
  }

  public static final class LuxCategory
  {
    @NonNull
    private final String mName;

    @ColorInt
    private final int mColor;

    LuxCategory(@NonNull String name, @NonNull String color)
    {
      mName = name;
      mColor = makeColorSafely(color);
    }

    @NonNull
    public String getName()
    {
      return mName;
    }

    public int getColor()
    {
      return mColor;
    }

    private static int makeColorSafely(@NonNull String color)
    {
      try {
        return makeColor(color);
      }
      catch (IllegalArgumentException exception) {
        return 0;
      }
    }
    private static int makeColor(@NonNull String color)
    {
      return Color.parseColor("#" + color);
    }

  }
}
