package com.mapswithme.maps.pins.pins;

public class Pin
{
  private Icon mIcon;
  private String mName = "";
  private PinSet mSet;

  Pin(String name, Icon icon)
  {
    mName = name;
    mIcon = icon;
  }

  public Icon getIcon()
  {
    return mIcon;
  }

  public void setIcon(Icon icon)
  {
    mIcon = icon;
  }

  public String getName()
  {
    return mName;
  }

  public void setName(String Name)
  {
    mName = Name;
  }

  public PinSet getPinSet()
  {
    return mSet;
  }

  public void setPinSet(PinSet set)
  {
    if (mSet != null && mSet != set)
    {
      mSet.removePin(this);
    }
    set.addPin(this);
    mSet = set;
  }
}
