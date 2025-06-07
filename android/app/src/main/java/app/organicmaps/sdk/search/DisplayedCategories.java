package app.organicmaps.sdk.search;

import androidx.annotation.NonNull;

public class DisplayedCategories
{
  @NonNull
  public static native String[] nativeGetKeys();
  public static native boolean nativeIsLangSupported(String langCode);
}
