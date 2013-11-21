package com.mapswithme.util;

import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.text.style.CharacterStyle;

public class StringUtils
{
  /**
   *  Set span for the FIRST occurrence of EACH token.
   *
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
}
