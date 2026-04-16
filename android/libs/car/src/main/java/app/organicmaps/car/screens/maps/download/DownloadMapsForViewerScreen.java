package app.organicmaps.car.screens.maps.download;

import androidx.annotation.NonNull;
import androidx.car.app.model.Action;
import app.organicmaps.car.R;

final class DownloadMapsForViewerScreen extends DownloadMapsScreen
{
  DownloadMapsForViewerScreen(@NonNull final DownloadMapsScreenBuilder builder)
  {
    super(builder);
  }

  @NonNull
  @Override
  protected String getTitle()
  {
    return getCarContext().getString(R.string.download_maps);
  }

  @NonNull
  @Override
  protected String getText(@NonNull String mapsSize)
  {
    final int mapsCount = getMissingMaps().size();
    if (mapsCount == 1)
      return DownloaderHelpers.getCountryName(getMissingMaps().get(0)) + "\n" + mapsSize;
    return getCarContext().getString(R.string.downloader_status_maps) + " (" + getMissingMaps().size()
  + "): " + mapsSize;
  }

  @NonNull
  @Override
  protected Action getHeaderAction()
  {
    return Action.BACK;
  }
}
