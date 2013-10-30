package com.mapswithme.maps.bookmarks.data;

import android.graphics.Bitmap;

public class Icon
{
  private final String mName;
  private final String mType;

  public Icon(String Name, String type, Bitmap Icon)
  {
    super();
    this.mName = Name;
    mType = type;
  }

  public String getType()
  {
    return mType;
  }

  public String getName()
  {
    return mName;
  }

  @Override
  public boolean equals(Object o)
  {
    return mType.equals(((Icon)o).getType());
  }
}
