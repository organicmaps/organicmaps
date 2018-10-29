package com.mapswithme.maps.bookmarks.data;

import android.support.annotation.NonNull;

public class CatalogCustomProperty
{
  @NonNull
  private final String mKey;

  @NonNull
  private final String mLocalizedName;

  private final boolean mRequired;

  @NonNull
  private final CatalogCustomPropertyOption[] mOptions;

  public CatalogCustomProperty(@NonNull String key, @NonNull String localizedName,
                               boolean required, @NonNull CatalogCustomPropertyOption[] options)
  {
    mKey = key;
    mLocalizedName = localizedName;
    mRequired = required;
    mOptions = options;
  }

  @NonNull
  public String getKey() { return mKey; }

  @NonNull
  public String getLocalizedName() { return mLocalizedName; }

  public boolean isRequired() { return mRequired; }

  @NonNull
  public CatalogCustomPropertyOption[] getOptions() { return mOptions; }
}
