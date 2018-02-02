package com.mapswithme;

import android.content.res.Resources;
import android.support.annotation.NonNull;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.style.ForegroundColorSpan;

import com.mapswithme.maps.R;

public class HotelUtils
{
  @NonNull
  public static CharSequence formatStars(int stars, @NonNull Resources resources)
  {
    if (stars <= 0)
      throw new AssertionError("Start count must be > 0");

    stars = Math.min(stars, 5);
    // Colorize last dimmed stars
    final SpannableStringBuilder sb = new SpannableStringBuilder("★ ★ ★ ★ ★");
    if (stars < 5)
    {
      final int start = sb.length() - ((5 - stars) * 2 - 1);
      sb.setSpan(new ForegroundColorSpan(resources.getColor(R.color.search_star_dimmed)),
                 start, sb.length(), Spanned.SPAN_INCLUSIVE_EXCLUSIVE);
    }
    return sb;
  }
}
