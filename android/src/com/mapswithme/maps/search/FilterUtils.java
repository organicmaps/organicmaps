package com.mapswithme.maps.search;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Iterator;
import java.util.List;

public class FilterUtils
{
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
          && or.mRhs instanceof HotelsFilter.PriceRateFilter )
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
  public static HotelsFilter createHotelFilter(int rating, int priceRate,
                                               @Nullable HotelsFilter.HotelType... types)
  {
    HotelsFilter priceFilter = createPriceRateFilter(priceRate);
    HotelsFilter typesFilter = createHotelTypeFilter(types);
    return combineFilters(priceFilter, typesFilter);
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
}
