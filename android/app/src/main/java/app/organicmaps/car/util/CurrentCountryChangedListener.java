package app.organicmaps.car.util;

import android.text.TextUtils;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.car.app.CarContext;
import androidx.car.app.ScreenManager;
import app.organicmaps.car.screens.download.DownloadMapsScreen;
import app.organicmaps.car.screens.download.DownloadMapsScreenBuilder;
import app.organicmaps.sdk.downloader.CountryItem;
import app.organicmaps.sdk.downloader.MapManager;
import app.organicmaps.sdk.routing.RoutingController;

public class CurrentCountryChangedListener implements MapManager.CurrentCountryChangedListener
{
  @Nullable
  private CarContext mCarContext;

  @Nullable
  private String mPreviousCountryId;

  @Override
  public void onCurrentCountryChanged(@Nullable String countryId)
  {
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
    MapManager.nativeSubscribeOnCountryChanged(this);
  }

  public void onStop()
  {
    MapManager.nativeUnsubscribeOnCountryChanged();
    mCarContext = null;
  }
}
