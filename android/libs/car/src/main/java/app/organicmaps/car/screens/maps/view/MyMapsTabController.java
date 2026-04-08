package app.organicmaps.car.screens.maps.view;

import androidx.annotation.NonNull;
import app.organicmaps.car.R;
import app.organicmaps.sdk.car.screens.BaseScreen;

final class MyMapsTabController extends MapsTabController
{
  MyMapsTabController(@NonNull BaseScreen parentScreen)
  {
    super(parentScreen, "my_maps", R.string.my_maps, R.drawable.ic_address, MapsProvider.Type.Mine);
  }
}
