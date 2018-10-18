package com.mapswithme.maps.bookmarks.data;

import android.support.annotation.NonNull;

import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.List;

public class CatalogTagsGroup
{
  @NonNull
  private final String mLocalizedName;

  @NonNull
  private final List<CatalogTag> mTags;

  public CatalogTagsGroup(@NonNull String localizedName, @NonNull CatalogTag[] tags)
  {
    mLocalizedName = localizedName;
    mTags = Collections.unmodifiableList(Arrays.asList(tags));
  }

  @NonNull
  public String getLocalizedName() { return mLocalizedName; }

  @NonNull
  public List<CatalogTag> getTags() { return mTags; }
}
