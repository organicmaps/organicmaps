package app.organicmaps.car.screens.download;

import androidx.annotation.NonNull;
import androidx.car.app.model.Action;
import app.organicmaps.R;
import app.organicmaps.routing.ResultCodesHelper;
import java.util.Objects;

class DownloadMapsForRouteScreen extends DownloadMapsScreen
{
  @NonNull
  private final String mTitle;

  DownloadMapsForRouteScreen(@NonNull final DownloadMapsScreenBuilder builder)
  {
    super(builder);

    mTitle = ResultCodesHelper
                 .getDialogTitleSubtitle(builder.mCarContext, builder.mResultCode,
                                         Objects.requireNonNull(builder.mMissingMaps).length)
                 .getTitleMessage()
                 .first;
  }

  @NonNull
  @Override
  protected String getTitle()
  {
    return mTitle;
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
