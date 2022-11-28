package app.organicmaps.util;

import android.content.Context;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Pair;

import androidx.annotation.NonNull;

import app.organicmaps.MwmApplication;
import app.organicmaps.R;

import java.util.Locale;

public class StringUtils
{
  public static String formatUsingUsLocale(String pattern, Object... args)
  {
    return String.format(Locale.US, pattern, args);
  }

  public static native boolean nativeIsHtml(String text);

  public static native boolean nativeContainsNormalized(String str, String substr);
  public static native String[] nativeFilterContainsNormalized(String[] strings, String substr);

  public static native Pair<String, String> nativeFormatSpeedAndUnits(double metersPerSecond);
  public static native String nativeFormatDistance(double meters);
  @NonNull
  public static native String nativeFormatDistanceWithLocalization(double meters,
                                                                   @NonNull String high,
                                                                   @NonNull String low);
  @NonNull
  public static native Pair<String, String> nativeGetLocalizedDistanceUnits();
  @NonNull
  public static native Pair<String, String> nativeGetLocalizedAltitudeUnits();
  @NonNull
  public static native String nativeGetLocalizedSpeedUnits();

  /**
   * Removes html tags, generated from edittext content after it's transformed to html.
   * In version 4.3.1 we converted descriptions, entered by users, to html automatically. Later html conversion was cancelled, but those converted descriptions should be converted back to
   * plain text, that's why that ugly util is introduced.
   *
   * @param text source text
   * @return result text
   */
  public static String removeEditTextHtmlTags(String text)
  {
    return text.replaceAll("</p>", "").replaceAll("<br>", "").replaceAll("<p dir=\"ltr\">", "");
  }

  /**
   * Formats size in bytes to "x MB" or "x.x GB" format.
   * Small values rounded to 1 MB without fractions.
   *
   * @param context context for getString()
   * @param size size in bytes
   * @return formatted string
   */
  public static String getFileSizeString(@NonNull Context context, long size)
  {
    if (size < Constants.GB)
    {
      int value = (int)((float)size / Constants.MB + 0.5f);
      if (value == 0)
        value = 1;

      return formatUsingUsLocale("%1$d %2$s", value, MwmApplication.from(context).getString(R.string.mb));
    }

    float value = ((float) size / Constants.GB);
    return formatUsingUsLocale("%1$.1f %2$s", value, MwmApplication.from(context).getString(R.string.gb));
  }

  public static boolean isRtl()
  {
    Locale defLocale = Locale.getDefault();
    return Character.getDirectionality(defLocale.getDisplayName(defLocale).charAt(0)) == Character.DIRECTIONALITY_RIGHT_TO_LEFT;
  }

  @NonNull
  public static String fixCaseInString(@NonNull String string)
  {
    char firstChar = string.charAt(0);
    return firstChar + toLowerCase(string.substring(1));
  }

  @NonNull
  public static String toLowerCase(@NonNull String string)
  {
    return string.toLowerCase(Locale.getDefault());
  }

  @NonNull
  public static String toUpperCase(@NonNull String string)
  {
    return string.toUpperCase(Locale.getDefault());
  }

  public static class SimpleTextWatcher implements TextWatcher
  {
    @Override
    public void beforeTextChanged(CharSequence s, int start, int count, int after) { }

    @Override
    public void onTextChanged(CharSequence s, int start, int before, int count) { }

    @Override
    public void afterTextChanged(Editable s) { }
  }

  private StringUtils() {}
}
