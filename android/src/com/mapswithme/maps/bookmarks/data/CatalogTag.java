package com.mapswithme.maps.bookmarks.data;

import android.graphics.Color;
import android.support.annotation.NonNull;

public class CatalogTag
{
  @NonNull
  private final String mId;

  @NonNull
  private final String mLocalizedName;

  private final int mColor;

  public CatalogTag(@NonNull String id, @NonNull String localizedName, float r, float g, float b)
  {
    mId = id;
    mLocalizedName = localizedName;
    mColor = Color.rgb((int)(r * 255), (int)(g * 255), (int)(b * 255));
  }

  @NonNull
  public String getId() { return mId; }

  @NonNull
  public String getLocalizedName() { return mLocalizedName; }

  public int getColor() { return mColor; }
}
