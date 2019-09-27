package com.mapswithme.maps.search;

import androidx.annotation.IntDef;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

final class BookingFilter
{
  // This list should correspond to the booking::filter::Type enum on c++ side.
  public static final int TYPE_DEALS = 0;
  public static final int TYPE_AVAILABILITY = 1;

  @Retention(RetentionPolicy.SOURCE)
  @IntDef({ TYPE_DEALS, TYPE_AVAILABILITY })
  public @interface Type {}
}
