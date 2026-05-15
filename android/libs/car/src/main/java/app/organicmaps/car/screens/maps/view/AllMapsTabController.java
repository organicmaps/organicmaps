package app.organicmaps.car.screens.maps.view;

import androidx.annotation.NonNull;
import app.organicmaps.car.R;
import app.organicmaps.sdk.car.screens.BaseScreen;

final class AllMapsTabController extends MapsTabController
{
  AllMapsTabController(@NonNull BaseScreen parentScreen)
  {
    super(parentScreen, "all_maps", R.string.all_maps, R.drawable.ic_downloader_maps, MapsProvider.Type.All);
  }
}
