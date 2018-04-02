package com.mapswithme.maps.bookmarks.data;


public class Icon
{
  private final String mName;
  private final int mColor;
  private final int mResId;
  private final int mSelectedResId;

  public Icon(String Name, int color, int resId, int selectedResId)
  {
    mName = Name;
    mColor = color;
    mResId = resId;
    mSelectedResId = selectedResId;
  }

  public int getColor()
  {
    return mColor;
  }

  public String getName()
  {
    return mName;
  }

  public int getResId()
  {
    return mResId;
  }

  public int getSelectedResId()
  {
    return mSelectedResId;
  }

  @Override
  public boolean equals(Object o)
  {
    if (o == null || !(o instanceof Icon))
      return false;
    final Icon comparedIcon = (Icon) o;
    return mColor == comparedIcon.getColor();
  }

  @Override
  public int hashCode()
  {
    return mColor;
  }
}
