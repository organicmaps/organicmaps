package com.mapswithme.maps.api;

import androidx.annotation.IntDef;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

public class ParsingResult
{
  @Retention(RetentionPolicy.SOURCE)
  @IntDef({ TYPE_INCORRECT, TYPE_MAP, TYPE_ROUTE, TYPE_SEARCH, TYPE_LEAD, TYPE_CATALOGUE,
            TYPE_CATALOGUE_PATH, TYPE_SUBSCRIPTION})
  public @interface UrlType {}

  // Represents url_scheme::ParsedMapApi::UrlType from c++ part.
  public static final int TYPE_INCORRECT = 0;
  public static final int TYPE_MAP = 1;
  public static final int TYPE_ROUTE = 2;
  public static final int TYPE_SEARCH = 3;
  public static final int TYPE_LEAD = 4;
  public static final int TYPE_CATALOGUE = 5;
  public static final int TYPE_CATALOGUE_PATH = 6;
  public static final int TYPE_SUBSCRIPTION = 7;

  private final int mUrlType;
  private final boolean mSuccess;

  @SuppressWarnings("unused")
  public ParsingResult(@UrlType int urlType, boolean isSuccess)
  {
    mUrlType = urlType;
    mSuccess = isSuccess;
  }

  @UrlType
  public int getUrlType()
  {
    return mUrlType;
  }

  public boolean isSuccess()
  {
    return mSuccess;
  }
}
