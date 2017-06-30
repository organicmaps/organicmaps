package com.mapswithme.maps.taxi;

import android.support.annotation.NonNull;

public class TaxiInfoError
{
  @TaxiManager.TaxiType
  private final int mType;
  @NonNull
  private final TaxiManager.ErrorCode mCode;

  public TaxiInfoError(int type, @NonNull String errorCode)
  {
    mType = type;
    mCode = TaxiManager.ErrorCode.valueOf(errorCode);
  }

  @NonNull
  public TaxiManager.ErrorCode getCode()
  {
    return mCode;
  }

  @TaxiManager.TaxiType
  public int getType()
  {
    return mType;
  }

  @Override
  public String toString()
  {
    return "TaxiInfoError{" +
           "mType=" + mType +
           ", mCode=" + mCode +
           '}';
  }
}
