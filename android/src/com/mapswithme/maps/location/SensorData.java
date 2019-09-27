package com.mapswithme.maps.location;

import androidx.annotation.Nullable;

class SensorData
{
  @Nullable
  private float[] mGravity;
  @Nullable
  private float[] mGeomagnetic;

  @Nullable
  public float[] getGravity()
  {
    return mGravity;
  }

  public void setGravity(@Nullable float[] gravity)
  {
    mGravity = gravity;
  }

  @Nullable
  public float[] getGeomagnetic()
  {
    return mGeomagnetic;
  }

  public void setGeomagnetic(@Nullable float[] geomagnetic)
  {
    mGeomagnetic = geomagnetic;
  }

  public boolean isAbsent()
  {
    return mGravity == null || mGeomagnetic == null;
  }
}
