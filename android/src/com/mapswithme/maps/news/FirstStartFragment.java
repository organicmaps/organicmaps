package com.mapswithme.maps.news;

import android.app.Dialog;
import android.content.DialogInterface;
import android.location.Location;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;

import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.util.Config;
import com.mapswithme.util.statistics.Statistics;

public class FirstStartFragment extends BaseNewsFragment
{
  private static Location sLocation;
  private static int sError = LocationHelper.ERROR_UNKNOWN;

  private final LocationHelper.LocationListener mLocationListener = new LocationHelper.SimpleLocationListener()
  {
    @Override
    public void onLocationUpdated(Location loc)
    {
      sLocation = loc;
      sError = LocationHelper.ERROR_UNKNOWN;
    }

    @Override
    public void onLocationError(int errorCode)
    {
      sLocation = null;
      sError = errorCode;
    }
  };

  private class Adapter extends BaseNewsFragment.Adapter
  {
    @Override
    int getTitles()
    {
      return R.array.first_start_titles;
    }

    @Override
    int getSubtitles1()
    {
      return R.array.first_start_subtitles;
    }

    @Override
    int getSubtitles2()
    {
      return 0;
    }

    @Override
    int getSwitchTitles()
    {
      return R.array.first_start_switch_titles;
    }

    @Override
    int getSwitchSubtitles()
    {
      return R.array.first_start_switch_subtitles;
    }

    @Override
    int getImages()
    {
      return R.array.first_start_images;
    }
  }

  @Override
  BaseNewsFragment.Adapter createAdapter()
  {
    return new Adapter();
  }

  @NonNull
  @Override
  public Dialog onCreateDialog(Bundle savedInstanceState)
  {
    LocationHelper.INSTANCE.addLocationListener(mLocationListener, true);
    return super.onCreateDialog(savedInstanceState);
  }

  @Override
  public void onDismiss(DialogInterface dialog)
  {
    super.onDismiss(dialog);

    if (sLocation == null)
    {
      String reason;
      switch (sError)
      {
      case LocationHelper.ERROR_GPS_OFF:
        reason = "unavailable";
        break;

      case LocationHelper.ERROR_DENIED:
        reason = "not_permitted";
        break;

      default:
        Statistics.INSTANCE.trackEvent(Statistics.EventName.FIRST_START_DONT_ZOOM);
        return;
      }

      Statistics.INSTANCE.trackEvent(Statistics.EventName.FIRST_START_NO_LOCATION, Statistics.params().add("reason", reason));
      return;
    }

    Framework.nativeZoomToPoint(sLocation.getLatitude(), sLocation.getLongitude(), 14, true);

    sLocation = null;
    LocationHelper.INSTANCE.removeLocationListener(mLocationListener);
  }

  public static boolean showOn(FragmentActivity activity)
  {
    if (Config.getFirstInstallVersion() < BuildConfig.VERSION_CODE)
      return false;

    FragmentManager fm = activity.getSupportFragmentManager();
    if (fm.isDestroyed())
      return false;

    if (Config.isFirstStartDialogSeen() &&
        !recreate(activity, FirstStartFragment.class))
      return false;

    create(activity, FirstStartFragment.class);

    Config.setFirstStartDialogSeen();
    Statistics.INSTANCE.trackEvent(Statistics.EventName.FIRST_START_SHOWN);

    return true;
  }
}
