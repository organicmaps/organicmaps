package com.mapswithme.util;

import android.text.Editable;
import android.text.TextWatcher;

import java.util.Locale;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;

public class StringUtils
{
  public static String formatUsingUsLocale(String pattern, Object... args)
  {
    return String.format(Locale.US, pattern, args);
  }

  public static native boolean nativeIsHtml(String text);

  public static native boolean nativeContainsNormalized(String str, String substr);
  public static native String[] nativeFilterContainsNormalized(String[] strings, String substr);

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
   * Formats size in bytes to "x.x MB" or "x.x KB" format.
   *
   * @param size size in bytes
   * @return formatted string
   */
  public static String getFileSizeString(long size)
  {
    boolean mb = (size >= Constants.MB);
    float value = ((float)size / (mb ? Constants.MB : Constants.KB));
    return formatUsingUsLocale("%1$.1f %2$s", value, MwmApplication.get().getString(mb ? R.string.mb : R.string.kb));
  }

  public static boolean isRtl()
  {
    Locale defLocale = Locale.getDefault();
    return Character.getDirectionality(defLocale.getDisplayName(defLocale).charAt(0)) == Character.DIRECTIONALITY_RIGHT_TO_LEFT;
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
