package app.organicmaps.car.screens.search;

import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.TextUtils;

import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import androidx.car.app.model.CarColor;
import androidx.car.app.model.ForegroundCarColorSpan;

import app.organicmaps.R;
import app.organicmaps.search.SearchResult;

public final class SearchUiHelpers
{
  @NonNull
  public static CharSequence getOpeningHoursAndDistanceText(@NonNull CarContext carContext, @NonNull SearchResult searchResult)
  {
    final SpannableStringBuilder result = new SpannableStringBuilder();
    final Spannable openingHours = getOpeningHours(carContext, searchResult);

    if (openingHours.length() != 0)
      result.append(openingHours);

    if (!TextUtils.isEmpty(searchResult.description.distance.toString()))
    {
      if (result.length() != 0)
        result.append(" • ");

      final SpannableStringBuilder distance = new SpannableStringBuilder(searchResult.description.distance.toString());
      distance.setSpan(ForegroundCarColorSpan.create(CarColor.BLUE), 0, distance.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
      result.append(distance);
    }

    return result;
  }

  @NonNull
  public static Spannable getOpeningHours(@NonNull CarContext carContext, @NonNull SearchResult searchResult)
  {
    final SpannableStringBuilder result = new SpannableStringBuilder();

    String text = "";
    CarColor color = CarColor.DEFAULT;
    switch (searchResult.description.openNow)
    {
    case SearchResult.OPEN_NOW_YES:
      if (searchResult.description.minutesUntilClosed < 60)   // less than 1 hour
      {
        final String time = searchResult.description.minutesUntilClosed + " " +
            carContext.getString(R.string.minute);
        text = carContext.getString(R.string.closes_in, time);
        color = CarColor.YELLOW;
      }
      else
      {
        text = carContext.getString(R.string.editor_time_open);
        color = CarColor.GREEN;
      }
      break;
    case SearchResult.OPEN_NOW_NO:
      if (searchResult.description.minutesUntilOpen < 60) // less than 1 hour
      {
        final String time = searchResult.description.minutesUntilOpen + " " +
            carContext.getString(R.string.minute);
        text = carContext.getString(R.string.opens_in, time);
      }
      else
        text = carContext.getString(R.string.closed);
      color = CarColor.RED;
      break;
    }

    result.append(text);
    result.setSpan(ForegroundCarColorSpan.create(color), 0, result.length(), Spanned.SPAN_INCLUSIVE_EXCLUSIVE);

    return result;
  }
}
