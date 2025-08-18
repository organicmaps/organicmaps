package app.organicmaps.car.screens.download;

import android.location.Location;
import android.text.TextUtils;
import androidx.annotation.NonNull;
import androidx.car.app.model.Action;
import androidx.lifecycle.LifecycleOwner;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.sdk.downloader.CountryItem;
import app.organicmaps.sdk.downloader.MapManager;

class DownloadMapsForFirstLaunchScreen extends DownloadMapsScreen
{
  DownloadMapsForFirstLaunchScreen(@NonNull final DownloadMapsScreenBuilder builder)
  {
    super(builder);
    disableCancelAction();
    getMissingMaps().add(CountryItem.fill(DownloaderHelpers.WORLD_MAPS[0]));
    getMissingMaps().add(CountryItem.fill(DownloaderHelpers.WORLD_MAPS[1]));
  }

  @Override
  public void onResume(@NonNull LifecycleOwner owner)
  {
    // Attempting to streamline initial download by including the current country in the list of missing maps for
    // simultaneous retrieval.
    final Location location = MwmApplication.from(getCarContext()).getLocationHelper().getSavedLocation();
    if (location == null)
      return;
    final String countryId = MapManager.nativeFindCountry(location.getLatitude(), location.getLongitude());
    if (TextUtils.isEmpty(countryId))
      return;
    final CountryItem countryItem = CountryItem.fill(countryId);
    if (!countryItem.present)
      getMissingMaps().add(countryItem);
  }

  @NonNull
  @Override
  protected String getTitle()
  {
    return getCarContext().getString(R.string.download_map_title);
  }

  @NonNull
  @Override
  protected String getText(@NonNull String mapsSize)
  {
    return getCarContext().getString(R.string.download_resources, mapsSize);
  }

  @NonNull
  @Override
  protected Action getHeaderAction()
  {
    return Action.APP_ICON;
  }
}
