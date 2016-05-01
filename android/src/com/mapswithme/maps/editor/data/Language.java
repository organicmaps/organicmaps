package com.mapswithme.maps.editor.data;

import android.support.annotation.NonNull;

// Corresponds to StringUtf8Multilang::Lang in core.
public class Language
{
  public final String code;
  public final String name;

  public Language(@NonNull String code, @NonNull String name)
  {
    this.code = code;
    this.name = name;
  }
}
