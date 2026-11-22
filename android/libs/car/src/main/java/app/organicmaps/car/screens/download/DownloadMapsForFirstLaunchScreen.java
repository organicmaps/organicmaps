package app.organicmaps.car.screens.download;

import android.location.Location;
import android.text.TextUtils;
import androidx.annotation.NonNull;
import androidx.car.app.model.Action;
import androidx.lifecycle.LifecycleOwner;
import app.organicmaps.car.R;
import app.organicmaps.car.util.UserActionRequired;
import app.organicmaps.sdk.downloader.CountryItem;
import app.organicmaps.sdk.downloader.MapManager;
import java.util.List;

class DownloadMapsForFirstLaunchScreen extends DownloadMapsScreen implements UserActionRequired
{
  private final List<CountryItem> mWorldMaps =
      List.of(CountryItem.fill(DownloaderHelpers.WORLD_MAPS[0]), CountryItem.fill(DownloaderHelpers.WORLD_MAPS[1]));

  DownloadMapsForFirstLaunchScreen(@NonNull final DownloadMapsScreenBuilder builder)
  {
    super(builder);
    disableCancelAction();
    getMissingMaps().addAll(mWorldMaps);
  }

  @Override
  public void onResume(@NonNull LifecycleOwner owner)
  {
    super.onResume(owner);
    getMissingMaps().clear();
    getMissingMaps().addAll(mWorldMaps);
    final Location location = getLocationHelper().getSavedLocation();
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
