package com.mapswithme.maps.bookmarks.data;


public class Icon
{
  private final String mName;
  private final String mType;
  private final int mResId;
  private final int mSelectedResId;

  public Icon(String Name, String type, int resId, int selectedResId)
  {
    mName = Name;
    mType = type;
    mResId = resId;
    mSelectedResId = selectedResId;
  }

  public String getType()
  {
    return mType;
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
    return mType.equals(comparedIcon.getType());
  }

  @Override
  public int hashCode()
  {
    return mType.hashCode();
  }
}
