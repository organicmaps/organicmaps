package com.mapswithme.maps.bookmarks.data;

import android.support.annotation.NonNull;

public class CatalogCustomProperty
{
  @NonNull
  private final String mKey;

  @NonNull
  private final String mLocalizedName;

  private final boolean mIsRequired;

  @NonNull
  private final CatalogCustomPropertyOption[] mOptions;

  public CatalogCustomProperty(@NonNull String key, @NonNull String localizedName,
                               boolean isRequired, @NonNull CatalogCustomPropertyOption[] options)
  {
    mKey = key;
    mLocalizedName = localizedName;
    mIsRequired = isRequired;
    mOptions = options;
  }

  @NonNull
  public String getKey() { return mKey; }

  @NonNull
  public String getLocalizedName() { return mLocalizedName; }

  public boolean isRequired() { return mIsRequired; }

  @NonNull
  public CatalogCustomPropertyOption[] getOptions() { return mOptions; }
}
