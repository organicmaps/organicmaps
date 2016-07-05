package com.mapswithme.maps.editor.data;

import android.support.annotation.NonNull;

public class LocalizedName
{
  // StringUtf8Multilang::kDefaultCode.
  public static final int DEFAULT_LANG_CODE = 0;

  public int code;
  @NonNull public String name;
  @NonNull public String lang;
  @NonNull public String langName;

  public LocalizedName(int code, @NonNull String name, @NonNull String lang, @NonNull String langName)
  {
    this.code = code;
    this.name = name;
    this.lang = lang;
    this.langName = langName;
  }
}
