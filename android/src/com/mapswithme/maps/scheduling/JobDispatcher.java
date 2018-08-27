package com.mapswithme.maps.scheduling;

public interface JobDispatcher
{
  void dispatch();
  void cancelAll();
}
