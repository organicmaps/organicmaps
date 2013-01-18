package com.mapswithme.maps.pins.pins;

import java.util.ArrayList;
import java.util.List;

public class PinSet
{
  private String mName;
  private boolean mVisibility = true;
  private final List<Pin> mPins = new ArrayList<Pin>();

  public String getName()
  {
    return mName;
  }

  public List<Pin> getPins()
  {
    return mPins;
  }

  public boolean isVisible()
  {
    return mVisibility;
  }

  public void setVisibility(boolean b)
  {
    mVisibility = b;
  }

  public void setName(String name)
  {
    this.mName = name;
  }

  PinSet(String name)
  {
    mName = name;
  }

  void addPin(Pin pin)
  {
    mPins.add(pin);
  }

  void removePin(Pin pin)
  {
    mPins.remove(pin);
  }
}
