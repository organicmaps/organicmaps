package app.organicmaps.car.screens.download;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.car.app.CarContext;
import app.organicmaps.routing.ResultCodesHelper;
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
      assert mMissingMaps != null;
      assert ResultCodesHelper.isDownloadable(mResultCode, mMissingMaps.length);
    }
    else if (mDownloaderType == DownloaderType.View)
    {
      assert mMissingMaps != null;
      assert mMissingMaps.length == 1;
    }
    else if (mDownloaderType == DownloaderType.FirstLaunch)
      assert mMissingMaps == null;

    return switch (mDownloaderType)
    {
      case FirstLaunch -> new DownloadMapsForFirstLaunchScreen(this);
      case BuildRoute -> new DownloadMapsForRouteScreen(this);
      case View -> new DownloadMapsForViewScreen(this);
    };
  }
}
