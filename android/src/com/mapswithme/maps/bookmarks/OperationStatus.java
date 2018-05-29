package com.mapswithme.maps.bookmarks;

import android.support.annotation.Nullable;

public class OperationStatus<R, E> {
  @Nullable private final R mResult;
  @Nullable private final E mError;

  OperationStatus(@Nullable R result, @Nullable E error) {
    mResult = result;
    mError = error;
  }

  public boolean isOk() {
    return mError == null;
  }

  public boolean isNotOk() {
    return !isOk();
  }

  @Nullable
  public R getResult() {
    return mResult;
  }

  @Nullable
  public E getError() {
    return mError;
  }
}
