package com.mapswithme.util;

import java.util.Locale;

import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.text.style.CharacterStyle;

public class StringUtils
{
  /**
   *  Set span for the FIRST occurrence of EACH token.
   *
   * @param input
   * @param spanStyle
   * @param tokens
   * @return
   */
  public static CharSequence setSpansForTokens(String input, CharacterStyle spanStyle, String ... tokens)
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

  public static String formatUsingUsLocale(String pattern, Object ... args)
  {
    return String.format(Locale.US, pattern, args);
  }

  public static String joinSkipEmpty(String delim, Object[] arr)
  {
    String res = null;
    for (int i = 0; i < arr.length; ++i)
    {
      String s = (arr[i] != null) ? arr[i].toString() : null;
      if (s != null && s.length() > 0)
        res = (res == null) ? s : res + delim + s;
    }
    return res;
  }

  private StringUtils() {}
}
