package com.mapswithme.util;

import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.text.style.CharacterStyle;

import java.util.Locale;

public class StringUtils
{
  /**
   * Set span for the FIRST occurrence of EACH token.
   *
   * @param input
   * @param spanStyle
   * @param tokens
   * @return
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
   * @param text source text
   * @return result text
   */
  public static String removeEditTextHtmlTags(String text)

  {
    return text.replaceAll("</p>", "").replaceAll("<br>", "").replaceAll("<p dir=\"ltr\">", "");
  }

  private StringUtils() {}
}
