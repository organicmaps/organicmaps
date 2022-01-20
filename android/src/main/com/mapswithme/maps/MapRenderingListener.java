package com.mapswithme.maps;

interface MapRenderingListener
{
  void onRenderingCreated();
  void onRenderingRestored();
  void onRenderingInitializationFinished();
}
