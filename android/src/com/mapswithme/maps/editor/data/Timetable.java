package com.mapswithme.maps.editor.data;

import android.support.annotation.NonNull;

public class Timetable
{
  public final Timespan workingTimespan;
  public final Timespan[] closedTimespans;
  public final boolean isFullday;
  public final int[] weekdays;

  public Timetable(@NonNull Timespan workingTime, @NonNull Timespan[] closedHours, boolean isFullday, @NonNull int[] weekdays)
  {
    this.workingTimespan = workingTime;
    this.closedTimespans = closedHours;
    this.isFullday = isFullday;
    this.weekdays = weekdays;
  }

  @Override
  public String toString()
  {
    StringBuilder stringBuilder = new StringBuilder();
    stringBuilder.append("Working timespan : ").append(workingTimespan).append("\n")
                 .append("Closed timespans : ");
    for (Timespan timespan : closedTimespans)
      stringBuilder.append(timespan).append("   ");
    stringBuilder.append("\n");
    stringBuilder.append("Fullday : ").append(isFullday).append("\n")
                 .append("Weekdays : ");
    for (int i : weekdays)
      stringBuilder.append(i);
    return stringBuilder.toString();
  }
}
