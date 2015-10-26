package com.mapswithme.util;

import android.text.Editable;
import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.text.TextWatcher;
import android.text.style.CharacterStyle;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;

import java.util.Locale;

public class StringUtils
{
  /**
   * Set span for the FIRST occurrence of EACH token.
   */
  public static CharSequence setSpansForTokens(String input, CharacterStyle spanStyle, String... tokens)
  {
    final SpannableStringBuilder spanStrBuilder = new SpannableStringBuilder(input);

    for (final String token : tokens)
    {
      final int indexStart = input.indexOf(token);
      if (indexStart != -1)
      {
        spanStrBuilder.setSpan(
            spanStyle,
            indexStart,
            indexStart + token.length(),
            Spannable.SPAN_INCLUSIVE_INCLUSIVE);
      }
    }

    return spanStrBuilder;
  }

  public static String formatUsingUsLocale(String pattern, Object... args)
  {
    return String.format(Locale.US, pattern, args);
  }

  public static String joinSkipEmpty(String delim, Object[] arr)
  {
    String res = null;
    for (Object item : arr)
    {
      String s = (item != null) ? item.toString() : null;
      if (s != null && s.length() > 0)
        res = (res == null) ? s : res + delim + s;
    }
    return res;
  }

  public static native boolean isHtml(String text);

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
   * Formats size in bytes to "x MB" or "x KB" format.
   *
   * @param size Size in bytes
   * @return formatted string
   */
  public static String getFileSizeString(long size)
  {
    if (size > Constants.MB)
      return (size + Constants.MB / 2) / Constants.MB + " " + MwmApplication.get().getString(R.string.mb);
    else
      return (size + Constants.KB - 1) / Constants.KB + " " + MwmApplication.get().getString(R.string.kb);
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
