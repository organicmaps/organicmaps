package com.mapswithme.maps.taxi;

import androidx.annotation.NonNull;

public class TaxiInfoError
{
  @NonNull
  private final TaxiType mType;
  @NonNull
  private final TaxiManager.ErrorCode mCode;

  public TaxiInfoError(int type, @NonNull String errorCode)
  {
    mType = TaxiType.values()[type];
    mCode = TaxiManager.ErrorCode.valueOf(errorCode);
  }

  @NonNull
  public TaxiManager.ErrorCode getCode()
  {
    return mCode;
  }

  @NonNull
  public TaxiType getTaxiType()
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
