package com.mapswithme.maps.maplayer;

import com.mapswithme.maps.content.CoreDetachable;

public interface MapLayerController extends CoreDetachable
{
  void turnOn();
  void turnOff();
  void show();
  void showImmediately();
  void hide();
  void hideImmediately();
  void adjust(int offsetX, int offsetY);
}
