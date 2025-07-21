package app.organicmaps.car.util;

import android.text.TextUtils;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.car.app.CarContext;
import androidx.car.app.ScreenManager;
import app.organicmaps.MwmApplication;
import app.organicmaps.car.screens.download.DownloadMapsScreen;
import app.organicmaps.car.screens.download.DownloadMapsScreenBuilder;
import app.organicmaps.routing.RoutingController;
import app.organicmaps.sdk.countryinfo.CountryInfo;
import app.organicmaps.sdk.countryinfo.CurrentCountryInfoManager;
import app.organicmaps.sdk.countryinfo.OnCurrentCountryChangedListener;
import app.organicmaps.sdk.downloader.CountryItem;

public class CurrentCountryChangedListener implements OnCurrentCountryChangedListener
{
  @Nullable
  private CarContext mCarContext;

  @Nullable
  private CurrentCountryInfoManager mCurrentCountryInfoManager;

  @Nullable
  private String mPreviousCountryId;

  @Override
  public void onCurrentCountryChanged(@NonNull CountryInfo countryInfo)
  {
    final String countryId = countryInfo.countryId();

    if (TextUtils.isEmpty(countryId))
    {
      mPreviousCountryId = countryId;
      return;
    }

    if (mPreviousCountryId != null && mPreviousCountryId.equals(countryId))
      return;

    if (mCarContext == null)
      return;

    final ScreenManager screenManager = mCarContext.getCarService(ScreenManager.class);

    if (DownloadMapsScreen.MARKER.equals(screenManager.getTop().getMarker()))
      return;

    if (CountryItem.fill(countryId).present || RoutingController.get().isNavigating())
      return;

    mPreviousCountryId = countryId;
    screenManager.push(new DownloadMapsScreenBuilder(mCarContext)
                           .setDownloaderType(DownloadMapsScreenBuilder.DownloaderType.View)
                           .setMissingMaps(new String[] {countryId})
                           .build());
  }

  public void onStart(@NonNull final CarContext carContext)
  {
    mCarContext = carContext;
    mCurrentCountryInfoManager = MwmApplication.from(mCarContext).getCurrentCountryInfoManager();
    mCurrentCountryInfoManager.addListener(this);
  }

  public void onStop()
  {
    assert mCurrentCountryInfoManager != null;
    mCurrentCountryInfoManager.removeListener(this);
    mCurrentCountryInfoManager = null;
    mCarContext = null;
  }
}
