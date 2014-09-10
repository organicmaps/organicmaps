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
    final StringBuilder sb = new StringBuilder(getClass().getSimpleName());
    sb.append("[name = ")
        .append(getName())
        .append(", params: ")
        .append(Utils.mapPrettyPrint(getParams()))
        .append("]");
    return sb.toString();
  }

}
