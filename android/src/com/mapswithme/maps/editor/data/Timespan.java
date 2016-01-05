package com.mapswithme.maps.editor.data;

public class Timespan
{
  public final HoursMinutes start;
  public final HoursMinutes end;

  public Timespan(HoursMinutes start, HoursMinutes end)
  {
    this.start = start;
    this.end = end;
  }

  @Override
  public String toString()
  {
    return "From : " + start + " to  : " + end;
  }
}
