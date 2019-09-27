package com.mapswithme.maps.editor.data;

import androidx.annotation.NonNull;

public class LocalizedName
{
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
