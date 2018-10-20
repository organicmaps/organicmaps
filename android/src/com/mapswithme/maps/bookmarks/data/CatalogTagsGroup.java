package com.mapswithme.maps.bookmarks.data;

import android.support.annotation.NonNull;

import java.util.Arrays;
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

  @Override
  public String toString()
  {
    final StringBuilder sb = new StringBuilder("CatalogTagsGroup{");
    sb.append("mLocalizedName='").append(mLocalizedName).append('\'');
    sb.append(", mTags=").append(mTags);
    sb.append('}');
    return sb.toString();
  }
}
