package com.mapswithme.util.statistics;

public class StatsClient
{
  public static native boolean trackSearchQuery(String query);

  private StatsClient() {}
}
