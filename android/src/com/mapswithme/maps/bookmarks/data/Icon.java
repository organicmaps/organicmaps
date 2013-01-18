package com.mapswithme.maps.bookmarks.data;

import android.graphics.Bitmap;

public class Icon
{
  private String mName;
  private String mType;
  private Bitmap mIcon;

  public Icon(String Name, String type, Bitmap Icon)
  {
    super();
    this.mName = Name;
    this.mIcon = Icon;
    mType = type;
  }

  public String getType()
  {
    return mType;
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
