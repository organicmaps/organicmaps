package com.mapswithme.util.statistics;

import android.support.annotation.NonNull;

public enum GalleryType
{
  VIATOR
      {
        @NonNull
        @Override
        public String getProvider()
        {
          return Statistics.ParamValue.VIATOR;
        }
      },
  LOCAL_EXPERTS
      {
        @NonNull
        @Override
        public String getProvider()
        {
          return Statistics.ParamValue.LOCALS_EXPERTS;
        }
      },
  SEARCH_RESTAURANTS
      {
        @NonNull
        @Override
        public String getProvider()
        {
          return Statistics.ParamValue.SEARCH_RESTAURANTS;
        }
      },
  SEARCH_ATTRACTIONS
      {
        @NonNull
        @Override
        public String getProvider()
        {
          return Statistics.ParamValue.SEARCH_ATTRACTIONS;
        }
      },
  SEARCH_HOTELS
      {
        @NonNull
        @Override
        public String getProvider()
        {
          return Statistics.ParamValue.BOOKING_COM;
        }
      };

  @NonNull
  public abstract String getProvider();
}
