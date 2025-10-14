package app.organicmaps.car.screens.download;

import androidx.annotation.NonNull;
import androidx.car.app.model.Action;
import app.organicmaps.R;

class DownloadMapsForViewScreen extends DownloadMapsScreen
{
  DownloadMapsForViewScreen(@NonNull final DownloadMapsScreenBuilder builder)
  {
    super(builder);
  }

  @NonNull
  @Override
  protected String getTitle()
  {
    return getCarContext().getString(R.string.downloader_download_map);
  }

  @NonNull
  @Override
  protected String getText(@NonNull String mapsSize)
  {
    return DownloaderHelpers.getCountryName(getMissingMaps().get(0)) + "\n" + mapsSize;
  }

  @NonNull
  @Override
  protected Action getHeaderAction()
  {
    return Action.BACK;
  }
}
