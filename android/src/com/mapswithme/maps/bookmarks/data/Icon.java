package com.mapswithme.maps.bookmarks.data;


public class Icon
{
  private final String mName;
  private final String mType;

  public Icon(String Name, String type)
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
    return mType.equals(((Icon) o).getType());
  }
}
