package app.organicmaps.car.screens.maps.view;

import androidx.annotation.NonNull;
import app.organicmaps.car.R;
import app.organicmaps.sdk.car.screens.BaseScreen;

final class UpdatableMapsTabController extends MapsTabController
{
  UpdatableMapsTabController(@NonNull BaseScreen parentScreen)
  {
    super(parentScreen, "updatable_maps", R.string.updatable_maps, R.drawable.ic_downloader_update,
          MapsProvider.Type.Updatable);
  }
}
