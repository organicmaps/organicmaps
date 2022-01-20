package com.mapswithme.maps.editor.data;

import androidx.annotation.NonNull;

public class LocalizedStreet
{
  public final String defaultName;
  public final String localizedName;

  public LocalizedStreet(@NonNull String defaultName, @NonNull String localizedName)
  {
    this.defaultName = defaultName;
    this.localizedName = localizedName;
  }
}
