package com.mapswithme.util.statistics;

import com.mapswithme.util.Utils;

import java.util.Map;

public class Event
{
  protected String mName;
  protected Map<String, String> mParams;

  public void setName(String name) { mName = name; }

  public void setParams(Map<String, String> params) { mParams = params; }

  public String getName() { return mName; }

  public Map<String, String> getParams() { return mParams; }

  public boolean hasParams()
  {
    return mParams != null && !mParams.isEmpty();
  }

  @Override
  public String toString()
  {
    return (getClass().getSimpleName() +
            "[name = " + getName() +
            ", params: " + Utils.mapPrettyPrint(getParams()) +
            "]");
  }
}
