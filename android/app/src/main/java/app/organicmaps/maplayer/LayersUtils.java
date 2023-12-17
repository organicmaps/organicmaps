package app.organicmaps.maplayer;

import java.util.ArrayList;
import java.util.List;

public class LayersUtils
{
  public static List<Mode> getAvailableLayers()
  {
    List<Mode> availableLayers = new ArrayList<>();
    availableLayers.add(Mode.ISOLINES);
    availableLayers.add(Mode.SUBWAY);
    return availableLayers;
  }
}
