package com.mapswithme.util.statistics;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class EventBuilder
{
  protected Event mEvent = new Event();
  protected List<StatisticsEngine> mEngines;

  public EventBuilder(List<StatisticsEngine> engine)
  {
    mEngines = engine;
  }

  public EventBuilder setName(String name)
  {
    mEvent.setName(name);
    return this;
  }

  public EventBuilder addParam(String key, String value)
  {
    Map<String, String> params = mEvent.getParams();
    if (params == null)
      params = new HashMap<String, String>();
    params.put(key, value);
    mEvent.setParams(params);
    return this;
  }

  public EventBuilder reset()
  {
    mEvent = new Event();
    return this;
  }

  public Event getEvent()
  {
    mEvent.setEngines(mEngines);
    return mEvent;
  }

  public Event getSimpleNamedEvent(String name)
  {
    return reset().setName(name).getEvent();
  }

  public void setEngines(List<StatisticsEngine> engine)
  {
    mEngines = engine;
  }
}
