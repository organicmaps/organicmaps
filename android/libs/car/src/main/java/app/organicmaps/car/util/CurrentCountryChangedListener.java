package app.organicmaps.car.util;

import android.text.TextUtils;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.car.app.CarContext;
import androidx.car.app.ScreenManager;
import app.organicmaps.car.screens.download.DownloadMapsScreen;
import app.organicmaps.car.screens.download.DownloadMapsScreenBuilder;
import app.organicmaps.sdk.OrganicMaps;
import app.organicmaps.sdk.display.DisplayManager;
import app.organicmaps.sdk.downloader.CountryItem;
import app.organicmaps.sdk.downloader.MapManager;
import app.organicmaps.sdk.routing.RoutingController;
import java.util.function.BooleanSupplier;

public class CurrentCountryChangedListener implements MapManager.CurrentCountryChangedListener
{
  @NonNull
  private final DisplayManager mDisplayManager;
  @NonNull
  private final OrganicMaps mOrganicMapsContext;
  @NonNull
  private final BooleanSupplier mWaitForRequiredScreen;
  @Nullable
  private CarContext mCarContext;
  @Nullable
  private String mPendingCountryId;
  @Nullable
  private String mPreviousCountryId;

  public CurrentCountryChangedListener(@NonNull OrganicMaps organicMapsContext, @NonNull DisplayManager displayManager,
                                       @NonNull BooleanSupplier waitForRequiredScreen)
  {
    mOrganicMapsContext = organicMapsContext;
    mDisplayManager = displayManager;
    mWaitForRequiredScreen = waitForRequiredScreen;
  }

  @Override
  public void onCurrentCountryChanged(@Nullable String countryId)
  {
    if (mCarContext == null || !mDisplayManager.isCarDisplayUsed())
      return;

    if (!mOrganicMapsContext.getMapsReadiness().isReady() || mWaitForRequiredScreen.getAsBoolean())
    {
      // Native code reports a country only when the viewport enters it, so keep the latest one for later.
      mPendingCountryId = countryId;
      return;
    }

    if (TextUtils.isEmpty(countryId))
    {
      mPreviousCountryId = countryId;
      return;
    }

    if (mPreviousCountryId != null && mPreviousCountryId.equals(countryId))
      return;

    final ScreenManager screenManager = mCarContext.getCarService(ScreenManager.class);
    if (DownloadMapsScreen.MARKER.equals(screenManager.getTop().getMarker()))
      return;

    if (CountryItem.fill(countryId).present || RoutingController.get().isNavigating())
      return;

    mPreviousCountryId = countryId;
    screenManager.push(new DownloadMapsScreenBuilder(mCarContext, mOrganicMapsContext)
                           .setDownloaderType(DownloadMapsScreenBuilder.DownloaderType.View)
                           .setMissingMaps(new String[] {countryId})
                           .build());
  }

  public void onStart(@NonNull CarContext carContext)
  {
    mCarContext = carContext;
    MapManager.nativeSubscribeOnCountryChanged(this);
  }

  public void onStop()
  {
    mPendingCountryId = null;
    MapManager.nativeUnsubscribeOnCountryChanged();
    mCarContext = null;
  }

  public void showPendingCountry()
  {
    final String countryId = mPendingCountryId;
    mPendingCountryId = null;
    // An earlier viewport can be stale once restoration or a launch intent has opened another screen.
    if (countryId != null && mCarContext != null && mCarContext.getCarService(ScreenManager.class).getStackSize() == 1)
      onCurrentCountryChanged(countryId);
  }
}
