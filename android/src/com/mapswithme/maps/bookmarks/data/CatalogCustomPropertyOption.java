package com.mapswithme.maps.bookmarks.data;

import android.support.annotation.NonNull;

public class CatalogCustomPropertyOption
{
  @NonNull
  private final String mValue;

  @NonNull
  private final String mLocalizedName;

  public CatalogCustomPropertyOption(@NonNull String value, @NonNull String localizedName)
  {
    mValue = value;
    mLocalizedName = localizedName;
  }

  @NonNull
  public String getValue() { return mValue; }

  @NonNull
  public String getLocalizedName() { return mLocalizedName; }
}
