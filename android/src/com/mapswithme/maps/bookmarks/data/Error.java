package com.mapswithme.maps.bookmarks.data;

import android.support.annotation.Nullable;

public class Error
{
  @Nullable
  private final Throwable mThrowable;

  public Error(@Nullable Throwable throwable)
  {
    mThrowable = throwable;
  }

  @Nullable
  public Throwable getThrowable()
  {
    return mThrowable;
  }
}
