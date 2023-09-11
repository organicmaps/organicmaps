package app.organicmaps.search;

import android.annotation.SuppressLint;
import android.content.res.Resources;

import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.annotation.StringRes;

public class DisplayedCategories
{
  @NonNull
  public static String[] getKeys()
  {
    return nativeGetKeys();
  }

  @SuppressLint("DiscouragedApi")
  @StringRes
  public static int getTitleResId(@NonNull Resources resources, @NonNull String packageName,
                                  @NonNull String key)
  {
    return resources.getIdentifier(key, "string", packageName);
  }

  @SuppressLint("DiscouragedApi")
  @DrawableRes
  public static int getDrawableResId(@NonNull Resources resources,
                                     @NonNull String packageName,
                                     boolean isNightTheme,
                                     @NonNull String key)
  {
    String iconId = "ic_category_" + key;
    if (isNightTheme)
      iconId = iconId + "_night";
    return resources.getIdentifier(iconId, "drawable", packageName);
  }

  @NonNull
  private static native String[] nativeGetKeys();
}
