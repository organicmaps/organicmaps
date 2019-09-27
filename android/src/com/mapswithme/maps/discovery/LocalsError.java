package com.mapswithme.maps.discovery;

import androidx.annotation.IntDef;
import androidx.annotation.NonNull;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

class LocalsError
{
  static final int UNKNOWN_ERROR = 0;

  @Retention(RetentionPolicy.SOURCE)
  @IntDef({ UNKNOWN_ERROR })
  @interface ErrorCode {}

  @ErrorCode
  private final int mCode;
  @NonNull
  private final String mMessage;

  public LocalsError(@ErrorCode int code, @NonNull String message)
  {
    mCode = code;
    mMessage = message;
  }

  @ErrorCode
  public int getCode()
  {
    return mCode;
  }

  @NonNull
  public String getMessage()
  {
    return mMessage;
  }

  @Override
  public String toString()
  {
    return "LocalsError{" +
           "mCode=" + mCode +
           ", mMessage=" + mMessage +
           '}';
  }
}
