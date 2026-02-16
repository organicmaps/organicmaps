package app.organicmaps.car.screens.download;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.car.app.CarContext;
import app.organicmaps.routing.ResultCodesHelper;
import app.organicmaps.sdk.util.Assert;
import java.util.Objects;

public class DownloadMapsScreenBuilder
{
  public enum DownloaderType
  {
    FirstLaunch,
    BuildRoute,
    View
  }

  private DownloaderType mDownloaderType = null;

  @NonNull
  final CarContext mCarContext;

  @Nullable
  String[] mMissingMaps;

  int mResultCode = 0;

  public DownloadMapsScreenBuilder(@NonNull CarContext carContext)
  {
    mCarContext = carContext;
  }

  @NonNull
  public DownloadMapsScreenBuilder setDownloaderType(@NonNull DownloaderType downloaderType)
  {
    mDownloaderType = downloaderType;
    return this;
  }

  @NonNull
  public DownloadMapsScreenBuilder setMissingMaps(@NonNull String[] missingMaps)
  {
    mMissingMaps = missingMaps;
    return this;
  }

  @NonNull
  public DownloadMapsScreenBuilder setResultCode(int resultCode)
  {
    mResultCode = resultCode;
    return this;
  }

  @NonNull
  public DownloadMapsScreen build()
  {
    Objects.requireNonNull(mDownloaderType);

    if (mDownloaderType == DownloaderType.BuildRoute)
    {
      Assert.debug(mMissingMaps != null, "mMissingMaps must be initialized");
      Assert.debug(ResultCodesHelper.isDownloadable(mResultCode, mMissingMaps.length),
                   "Invalid result code for downloadable maps");
    }
    else if (mDownloaderType == DownloaderType.View)
    {
      Assert.debug(mMissingMaps != null, "mMissingMaps must be initialized");
      Assert.debug(mMissingMaps.length == 1, "mMissingMaps must contain exactly one element");
    }
    else if (mDownloaderType == DownloaderType.FirstLaunch)
      Assert.debug(mMissingMaps == null, "mMissingMaps must be null for FirstLaunch");

    return switch (mDownloaderType)
    {
      case FirstLaunch -> new DownloadMapsForFirstLaunchScreen(this);
      case BuildRoute -> new DownloadMapsForRouteScreen(this);
      case View -> new DownloadMapsForViewScreen(this);
    };
  }
}
