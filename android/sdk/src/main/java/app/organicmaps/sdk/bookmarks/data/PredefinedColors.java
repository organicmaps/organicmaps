package app.organicmaps.sdk.bookmarks.data;

import androidx.annotation.ColorInt;
import androidx.annotation.NonNull;
import dalvik.annotation.optimization.FastNative;
import java.util.List;
import java.util.stream.IntStream;

/// The canonical Organic Maps preset color palette: the brand colors that back kml::PredefinedColor,
/// fetched once from the core via JNI as ARGB. Used to seed the color picker's preset swatches so
/// they match the presets shown on desktop and iOS.
public class PredefinedColors
{
  /// ARGB color for every kml::PredefinedColor. Index 0 is the "no color" (None) slot.
  @ColorInt
  private static final int[] PREDEFINED_COLORS = nativeGetPredefinedColors();

  /// The selectable preset colors (ARGB), excluding the "no color" slot at index 0.
  @NonNull
  public static List<Integer> getAllPredefinedColors()
  {
    return IntStream.range(1, PREDEFINED_COLORS.length).map(i -> PREDEFINED_COLORS[i]).boxed().toList();
  }

  @FastNative
  @NonNull
  private static native int[] nativeGetPredefinedColors();
}
