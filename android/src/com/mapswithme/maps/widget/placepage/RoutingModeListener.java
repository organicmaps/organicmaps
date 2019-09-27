package com.mapswithme.maps.widget.placepage;

import androidx.annotation.NonNull;

import com.mapswithme.maps.settings.RoadType;

public interface RoutingModeListener
{
  void toggleRouteSettings(@NonNull RoadType roadType);
}
