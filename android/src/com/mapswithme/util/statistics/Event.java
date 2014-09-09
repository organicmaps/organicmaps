package com.mapswithme.util.statistics;

import com.mapswithme.util.Utils;

import java.util.List;
import java.util.Map;

public class Event
{
  protected String mName;
  protected Map<String, String> mParams;
  protected List<StatisticsEngine> mEngines;

  public void setName(String name) { mName = name; }

  public void setParams(Map<String, String> params) { mParams = params; }

  public void setEngines(List<StatisticsEngine> engines) { mEngines = engines; }

  public String getName() { return mName; }

  public Map<String, String> getParams() { return mParams; }

  public List<StatisticsEngine> getEngines() { return mEngines; }

  public boolean hasParams()
  {
    return mParams != null && !mParams.isEmpty();
  }

  public void post()
  {
    for (StatisticsEngine engine : mEngines)
      engine.postEvent(this);
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
