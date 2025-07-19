package app.organicmaps.sdk.bookmarks.data;

import androidx.annotation.ColorInt;
import androidx.annotation.IntRange;
import androidx.annotation.NonNull;
import dalvik.annotation.optimization.FastNative;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.List;
import java.util.stream.IntStream;

public class PredefinedColors
{
  @Retention(RetentionPolicy.SOURCE)
  @IntRange(from = 0)
  public @interface Color
  {}

  /// @note Color format: ARGB
  @ColorInt
  private static final int[] PREDEFINED_COLORS = nativeGetPredefinedColors();

  @ColorInt
  public static int getColor(int index)
  {
    return PREDEFINED_COLORS[index];
  }

  @PredefinedColors.Color
  public static List<Integer> getAllPredefinedColors()
  {
    // 0 is reserved for "no color" option.
    return IntStream.range(1, PREDEFINED_COLORS.length).boxed().toList();
  }

  public static int getPredefinedColorIndex(@ColorInt int color)
  {
    // 0 is reserved for "no color" option.
    for (int index = 1; index < PREDEFINED_COLORS.length; index++)
    {
      if (PREDEFINED_COLORS[index] == color)
        return index;
    }
    return -1;
  }

  @FastNative
  @NonNull
  private static native int[] nativeGetPredefinedColors();
}
