package com.mapswithme.maps.bookmarks.data;

import android.support.annotation.NonNull;

public class CatalogTagsGroup
{
  @NonNull
  private final String mLocalizedName;

  @NonNull
  private final CatalogTag[] mTags;

  public CatalogTagsGroup(@NonNull String localizedName, @NonNull CatalogTag[] tags)
  {
    mLocalizedName = localizedName;
    mTags = tags;
  }

  @NonNull
  public String getLocalizedName() { return mLocalizedName; }

  @NonNull
  public CatalogTag[] getTags() { return mTags; }
}
