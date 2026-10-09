package app.organicmaps.maplayer;

import androidx.annotation.NonNull;
import app.organicmaps.sdk.maplayer.Mode;
import java.util.ArrayList;
import java.util.List;

public class LayersUtils
{
  @NonNull
  public static List<Mode> getAvailableLayers()
  {
    List<Mode> availableLayers = new ArrayList<>();
    for (Mode mode : Mode.values())
    {
      if (mode.isAvailable())
        availableLayers.add(mode);
    }
    return availableLayers;
  }
}
