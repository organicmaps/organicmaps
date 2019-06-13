package com.mapswithme.maps.promo;

import android.graphics.Color;
import android.support.annotation.ColorInt;
import android.support.annotation.NonNull;

import com.mapswithme.maps.gallery.Constants;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

public final class PromoCityGallery
{
  @NonNull
  private final List<Item> mItems;
  @NonNull
  private final String mMoreUrl;

  PromoCityGallery(@NonNull Item[] items, @NonNull String moreUrl)
  {
    mItems = Collections.unmodifiableList(Arrays.asList(items));
    mMoreUrl = moreUrl;
  }

  @NonNull
  public static List<PromoEntity> toEntities(@NonNull PromoCityGallery gallery)
  {
    List<PromoEntity> items = new ArrayList<>();
    for (PromoCityGallery.Item each : gallery.getItems())
    {
      PromoEntity item = new PromoEntity(Constants.TYPE_PRODUCT,
                                         each.getName(),
                                         each.getAuthor().getName(),
                                         each.getUrl(),
                                         each.getLuxCategory());
      items.add(item);
    }
    return items;
  }

  @NonNull
  public List<Item> getItems()
  {
    return mItems;
  }

  @NonNull
  public String getMoreUrl()
  {
    return mMoreUrl;
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
    private final Author mAuthor;
    @NonNull
    private final LuxCategory mLuxCategory;

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
    public Author getAuthor()
    {
      return mAuthor;
    }
    @NonNull
    public LuxCategory getLuxCategory()
    {
      return mLuxCategory;
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
