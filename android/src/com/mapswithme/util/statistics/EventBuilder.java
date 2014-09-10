package com.mapswithme.util.statistics;

import java.util.HashMap;
import java.util.Map;

public class EventBuilder
{
  protected String mName;
  protected Map<String, String> mParams;

  public EventBuilder setName(String name)
  {
    mName = name;
    return this;
  }

  public EventBuilder addParam(String key, String value)
  {
    if (mParams == null)
      mParams = new HashMap<String, String>();
    mParams.put(key, value);
    return this;
  }

  public Event buildEvent()
  {
    final Event event = new Event();
    event.setName(mName);
    event.setParams(mParams);
    mName = "";
    mParams = null;
    return event;
  }
}
