package app.organicmaps.car.screens.search;

import android.text.SpannableStringBuilder;
import android.text.Spanned;

import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import androidx.car.app.model.CarColor;
import androidx.car.app.model.DistanceSpan;
import androidx.car.app.model.ForegroundCarColorSpan;

import app.organicmaps.R;
import app.organicmaps.car.util.Colors;
import app.organicmaps.car.util.RoutingHelpers;
import app.organicmaps.search.SearchResult;

public final class SearchUiHelpers
{
  @NonNull
  public static CharSequence getOpeningHoursAndDistanceText(@NonNull CarContext carContext, @NonNull SearchResult searchResult)
  {
    final CharSequence openingHours = getOpeningHoursText(carContext, searchResult);
    final CharSequence distance = getDistanceText(searchResult);

    return getOpeningHoursAndDistanceText(openingHours, distance);
  }

  @NonNull
  public static CharSequence getOpeningHoursAndDistanceText(@NonNull CharSequence openingHours, @NonNull CharSequence distance)
  {
    final SpannableStringBuilder result = new SpannableStringBuilder();
    if (openingHours.length() != 0)
      result.append(openingHours);
    if (result.length() != 0 && distance.length() != 0)
      result.append(" • ");
    if (distance.length() != 0)
      result.append(distance);

    return result;
  }

  @NonNull
  public static CharSequence getDistanceText(@NonNull SearchResult searchResult)
  {
    if (!searchResult.description.distance.isValid())
      return "";

    final SpannableStringBuilder distance = new SpannableStringBuilder(" ");
    distance.setSpan(DistanceSpan.create(RoutingHelpers.createDistance(searchResult.description.distance)), 0, 1, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
    distance.setSpan(ForegroundCarColorSpan.create(Colors.DISTANCE), 0, 1, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
    return distance;
  }

  @NonNull
  public static CharSequence getOpeningHoursText(@NonNull CarContext carContext, @NonNull SearchResult searchResult)
  {
    final SpannableStringBuilder result = new SpannableStringBuilder();

    String text = "";
    CarColor color = Colors.DEFAULT;
    switch (searchResult.description.openNow)
    {
    case SearchResult.OPEN_NOW_YES:
      if (searchResult.description.minutesUntilClosed < 60)   // less than 1 hour
      {
        final String time = searchResult.description.minutesUntilClosed + " " +
            carContext.getString(R.string.minute);
        text = carContext.getString(R.string.closes_in, time);
        color = Colors.OPENING_HOURS_CLOSES_SOON;
      }
      else
      {
        text = carContext.getString(R.string.editor_time_open);
        color = Colors.OPENING_HOURS_OPEN;
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
      color = Colors.OPENING_HOURS_CLOSED;
      break;
    }

    result.append(text);
    result.setSpan(ForegroundCarColorSpan.create(color), 0, result.length(), Spanned.SPAN_INCLUSIVE_EXCLUSIVE);

    return result;
  }
}
