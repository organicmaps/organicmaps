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
  CIAN
      {
        @NonNull
        @Override
        public String getProvider()
        {
          return Statistics.ParamValue.CIAN;
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
      };

  @NonNull
  public abstract String getProvider();
}
