package com.mapswithme.maps.search;

import android.content.Context;

import androidx.annotation.IntDef;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.google.android.material.datepicker.CalendarConstraints;
import com.google.android.material.datepicker.CompositeDateValidator;
import com.google.android.material.datepicker.DateValidatorPointBackward;
import com.google.android.material.datepicker.DateValidatorPointForward;
import com.google.android.material.datepicker.MaterialDatePicker;
import com.mapswithme.maps.R;
import com.mapswithme.util.Utils;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Calendar;
import java.util.Date;
import java.util.Iterator;
import java.util.List;
import java.util.Locale;
import java.util.concurrent.TimeUnit;

public class FilterUtils
{
  private static final int MAX_STAYING_DAYS = 30;
  private static final int MAX_CHECKIN_WINDOW_IN_DAYS = 365;
  private static final String DAY_OF_MONTH_PATTERN = "MMM d";

  @Retention(RetentionPolicy.SOURCE)
  @IntDef({ RATING_ANY, RATING_GOOD, RATING_VERYGOOD, RATING_EXCELLENT })
  public @interface RatingDef
  {
  }

  public static final int RATING_ANY = 0;
  static final int RATING_GOOD = 1;
  static final int RATING_VERYGOOD = 2;
  static final int RATING_EXCELLENT = 3;

  private FilterUtils()
  {
  }

  @Nullable
  static HotelsFilter.OneOf makeOneOf(@NonNull Iterator<HotelsFilter.HotelType> iterator)
  {
    if (!iterator.hasNext())
      return null;

    HotelsFilter.HotelType type = iterator.next();
    return new HotelsFilter.OneOf(type, makeOneOf(iterator));
  }

  @Nullable
  static HotelsFilter combineFilters(@NonNull HotelsFilter... filters)
  {
    HotelsFilter result = null;
    for (HotelsFilter filter : filters)
    {
      if (result == null)
      {
        result = filter;
        continue;
      }

      if (filter != null)
        result = new HotelsFilter.And(filter, result);
    }

    return result;
  }

  @Nullable
  static HotelsFilter findPriceFilter(@NonNull HotelsFilter filter)
  {
    if (filter instanceof HotelsFilter.PriceRateFilter)
      return filter;

    if (filter instanceof HotelsFilter.Or)
    {
      HotelsFilter.Or or = (HotelsFilter.Or) filter;
      if (or.mLhs instanceof HotelsFilter.PriceRateFilter
          && or.mRhs instanceof HotelsFilter.PriceRateFilter)
      {
        return filter;
      }
      if (or.mLhs instanceof HotelsFilter.Or
          && ((HotelsFilter.Or) or.mLhs).mLhs instanceof HotelsFilter.PriceRateFilter
          && ((HotelsFilter.Or) or.mLhs).mRhs instanceof HotelsFilter.PriceRateFilter)
      {
        return filter;
      }
    }

    HotelsFilter result;
    if (filter instanceof HotelsFilter.And)
    {
      HotelsFilter.And and = (HotelsFilter.And) filter;
      result = findPriceFilter(and.mLhs);
      if (result == null)
        result = findPriceFilter(and.mRhs);

      return result;
    }

    return null;
  }

  @Nullable
  static HotelsFilter.OneOf findTypeFilter(@NonNull HotelsFilter filter)
  {
    if (filter instanceof HotelsFilter.OneOf)
      return (HotelsFilter.OneOf) filter;

    HotelsFilter.OneOf result;
    if (filter instanceof HotelsFilter.And)
    {
      HotelsFilter.And and = (HotelsFilter.And) filter;
      result = findTypeFilter(and.mLhs);
      if (result == null)
        result = findTypeFilter(and.mRhs);

      return result;
    }

    return null;
  }

  @Nullable
  static HotelsFilter.RatingFilter findRatingFilter(@NonNull HotelsFilter filter)
  {
    if (filter instanceof HotelsFilter.RatingFilter)
      return (HotelsFilter.RatingFilter) filter;

    HotelsFilter.RatingFilter result;
    if (filter instanceof HotelsFilter.And)
    {
      HotelsFilter.And and = (HotelsFilter.And) filter;
      result = findRatingFilter(and.mLhs);
      if (result == null)
        result = findRatingFilter(and.mRhs);

      return result;
    }

    return null;
  }

  @Nullable
  public static HotelsFilter createHotelFilter(@RatingDef int rating, int priceRate,
                                               @Nullable HotelsFilter.HotelType... types)
  {
    HotelsFilter ratingFilter = createRatingFilter(rating);
    HotelsFilter priceFilter = createPriceRateFilter(priceRate);
    HotelsFilter typesFilter = createHotelTypeFilter(types);
    return combineFilters(ratingFilter, priceFilter, typesFilter);
  }

  @Nullable
  private static HotelsFilter createRatingFilter(@RatingDef int rating)
  {
    switch (rating)
    {
      case RATING_ANY:
        return null;
      case RATING_GOOD:
        return new HotelsFilter.RatingFilter(HotelsFilter.Op.OP_GE, RatingFilterView.GOOD);
      case RATING_VERYGOOD:
        return new HotelsFilter.RatingFilter(HotelsFilter.Op.OP_GE, RatingFilterView.VERY_GOOD);
      case RATING_EXCELLENT:
        return new HotelsFilter.RatingFilter(HotelsFilter.Op.OP_GE, RatingFilterView.EXCELLENT);
      default:
        throw new AssertionError("Unsupported rating type: " + rating);
    }
  }

  @Nullable
  private static HotelsFilter createPriceRateFilter(@PriceFilterView.PriceDef int priceRate)
  {
    if (priceRate != PriceFilterView.LOW && priceRate != PriceFilterView.MEDIUM
        && priceRate != PriceFilterView.HIGH)
      return null;

    return new HotelsFilter.PriceRateFilter(HotelsFilter.Op.OP_EQ, priceRate);
  }

  @Nullable
  private static HotelsFilter createHotelTypeFilter(@Nullable HotelsFilter.HotelType... types)
  {
    if (types == null)
      return null;

    List<HotelsFilter.HotelType> hotelTypes = new ArrayList<>(Arrays.asList(types));
    return makeOneOf(hotelTypes.iterator());
  }

  public static long getMaxCheckoutInMillis(long checkinMillis)
  {
    long difference = checkinMillis - MaterialDatePicker.todayInUtcMilliseconds();
    int daysToCheckin = (int) TimeUnit.MILLISECONDS.toDays(difference);
    int leftDays = MAX_CHECKIN_WINDOW_IN_DAYS - daysToCheckin;
    if (leftDays <= 0)
      throw new AssertionError("No available dates for checkout!");
    Calendar date = Utils.getCalendarInstance();
    date.setTimeInMillis(checkinMillis);
    date.add(Calendar.DAY_OF_YEAR, Math.min(leftDays, MAX_STAYING_DAYS));
    return date.getTimeInMillis();
  }

  private static long getMaxCheckinInMillis()
  {
    final long today = MaterialDatePicker.todayInUtcMilliseconds();
    Calendar calendar = Utils.getCalendarInstance();
    calendar.setTimeInMillis(today);
    calendar.add(Calendar.DAY_OF_YEAR, MAX_CHECKIN_WINDOW_IN_DAYS);
    return calendar.getTimeInMillis();
  }

  @NonNull
  public static CalendarConstraints.Builder createDateConstraintsBuilder()
  {
    final long today = MaterialDatePicker.todayInUtcMilliseconds();
    CalendarConstraints.Builder constraintsBuilder = new CalendarConstraints.Builder();
    constraintsBuilder.setStart(today);
    constraintsBuilder.setEnd(getMaxCheckinInMillis());
    List<CalendarConstraints.DateValidator> validators = new ArrayList<>();
    validators.add(DateValidatorPointForward.now());
    validators.add(DateValidatorPointBackward.before(getMaxCheckinInMillis()));
    constraintsBuilder.setValidator(CompositeDateValidator.allOf(validators));
    return constraintsBuilder;
  }

  @NonNull
  public static String makeDateRangeHeader(@NonNull Context context, long checkinMillis,
                                           long checkoutMillis)
  {
    final SimpleDateFormat dateFormater = new SimpleDateFormat(DAY_OF_MONTH_PATTERN,
                                                               Locale.getDefault());
    String checkin = dateFormater.format(new Date(checkinMillis));
    String checkout = dateFormater.format(new Date(checkoutMillis));
    return context.getString(R.string.booking_filter_date_range, checkin, checkout);
  }

  public static boolean isWithinMaxStayingDays(long checkinMillis, long checkoutMillis)
  {
    long difference = checkoutMillis - checkinMillis;
    int days = (int) TimeUnit.MILLISECONDS.toDays(difference);
    return days <= MAX_STAYING_DAYS;
  }

  public static long getDayAfter(long date)
  {
    Calendar dayAfter = Utils.getCalendarInstance();
    dayAfter.setTimeInMillis(date);
    dayAfter.add(Calendar.DAY_OF_YEAR, 1);
    return dayAfter.getTimeInMillis();
  }
}
