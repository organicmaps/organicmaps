package com.mapswithme.maps.bookmarks.data;

import android.support.annotation.Nullable;

import java.net.HttpURLConnection;

public class Error
{
  private final int mHttpCode;
  @Nullable
  private final String mMessage;

  public Error(int httpCode, @Nullable String message)
  {
    mHttpCode = httpCode;
    mMessage = message;
  }

  public Error(@Nullable String message)
  {
    this(HttpURLConnection.HTTP_UNAVAILABLE, message);
  }

  public int getHttpCode()
  {
    return mHttpCode;
  }

  @Nullable
  public String getMessage()
  {
    return mMessage;
  }

  public boolean isForbidden()
  {
    return mHttpCode == HttpURLConnection.HTTP_FORBIDDEN;
  }

  public boolean isPaymentRequired()
  {
    return mHttpCode == HttpURLConnection.HTTP_PAYMENT_REQUIRED;
  }
}
