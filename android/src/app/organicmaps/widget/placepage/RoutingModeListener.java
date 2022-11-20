package app.organicmaps.widget.placepage;

import androidx.annotation.NonNull;

import app.organicmaps.settings.RoadType;

public interface RoutingModeListener
{
  void toggleRouteSettings(@NonNull RoadType roadType);
}
