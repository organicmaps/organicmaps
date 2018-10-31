package com.mapswithme.maps.bookmarks.data;

import android.support.annotation.NonNull;

import java.util.Arrays;
import java.util.Collections;
import java.util.List;

public class CatalogCustomProperty
{
  @NonNull
  private final String mKey;

  @NonNull
  private final String mLocalizedName;

  private final boolean mRequired;

  @NonNull
  private final List<CatalogCustomPropertyOption> mOptions;

  public CatalogCustomProperty(@NonNull String key, @NonNull String localizedName,
                               boolean required, @NonNull CatalogCustomPropertyOption[] options)
  {
    mKey = key;
    mLocalizedName = localizedName;
    mRequired = required;
    mOptions = Collections.unmodifiableList(Arrays.asList(options));
  }

  @NonNull
  public String getKey() { return mKey; }

  @NonNull
  public String getLocalizedName() { return mLocalizedName; }

  public boolean isRequired() { return mRequired; }

  @NonNull
  public List<CatalogCustomPropertyOption> getOptions() { return mOptions; }
}
