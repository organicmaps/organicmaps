package com.mapswithme.maps.pins.pins;

import android.graphics.Bitmap;

public class Icon
{
  private int mDrawableId;
  private String mName;
  private Bitmap mIcon;

  public Icon(String Name, Bitmap Icon, int id)
  {
    super();
    this.mName = Name;
    this.mIcon = Icon;
    mDrawableId = id;
  }

  public int getDrawableId()
  {
    return mDrawableId;
  }

  public Bitmap getIcon()
  {
    return mIcon;
  }

  public String getName()
  {
    return mName;
  }
}
