package app.organicmaps.settings;

import android.Manifest;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.location.Location;
import android.os.Bundle;
import android.view.View;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.app.ActivityCompat;
import androidx.preference.ListPreference;
import androidx.preference.Preference;
import androidx.preference.TwoStatePreference;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.location.LocationHelper;
import app.organicmaps.location.LocationListener;
import app.organicmaps.location.TrackRecorder;
import app.organicmaps.location.TrackRecordingService;
import app.organicmaps.util.LocationUtils;
import app.organicmaps.util.log.Logger;

import static app.organicmaps.location.TrackRecordingService.stopService;

public class TrackRecordSettingsFragment extends BaseXmlSettingsFragment implements LocationListener
{
  private ListPreference mRecordTime;
  private TwoStatePreference mRecentTrack;

  @Override
  protected int getXmlResources()
  {
    return R.xml.pref_track_record;
  }

  @Override
  public void onViewCreated(View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    mRecentTrack = getPreference(getString(R.string.pref_recent_track));
    mRecordTime = getPreference(getString(R.string.pref_track_record_time));
    LocationHelper.from(requireContext()).addListener(this);
  }

  @Override
  public void onStart()
  {
    super.onStart();
    mRecentTrack.setChecked(TrackRecorder.nativeIsEnabled());
    mRecordTime.setValue(Integer.toString(TrackRecorder.nativeGetDuration()));
    if(TrackRecorder.nativeIsEnabled())
    {
      mRecordTime.setSummary("Last " + TrackRecorder.nativeGetDuration() + " hours trails will be shown on map");
    }
    else
    {
      mRecordTime.setSummary("Enable Recent Track Recorder to select duration for which recorded point will be shown on map");
    }
    mRecordTime.setEnabled(TrackRecorder.nativeIsEnabled());
    mRecentTrack.setOnPreferenceChangeListener(((preference, newValue) -> {
      if (newValue == null)
        return false;

      boolean newVal = (boolean) newValue;

      if (newVal)
      {
        if(!LocationUtils.areLocationServicesTurnedOn(MwmApplication.from(requireContext()))){
          Toast.makeText(getActivity(),"Please turn on location",Toast.LENGTH_SHORT).show();
          return false;
        }
        if (!LocationUtils.checkLocationPermission(MwmApplication.from(requireContext())))
        {
          Toast.makeText(getActivity(),"Please give permission of precise location access",Toast.LENGTH_SHORT).show();
          return false;
        }
        TrackRecordingService.startForegroundService(MwmApplication.from(requireContext()));
        mRecordTime.setSummary("Last " + TrackRecorder.nativeGetDuration() + " hour trails will be shown on map");
      }
      else
      {
        stopService(MwmApplication.from(requireContext()));
        mRecordTime.setSummary("Enable Recent Track Recorder to select duration for which recorded point will be shown on map");
      }
      mRecentTrack.setChecked(newVal);
      mRecordTime.setEnabled(newVal);
      TrackRecorder.nativeSetEnabled(newVal);

      return true;
    }));

    mRecordTime.setOnPreferenceChangeListener((preference, newValue) -> {
      if (newValue == null)
        return false;
      String newVal = (String) newValue;
      if(newVal.equals(mRecordTime.getValue()))
        return false;

      mRecordTime.setSummary("Last " + newVal + " hour trails will be shown on map");
      int hours = Integer.parseInt(newVal);
      TrackRecorder.nativeSetDuration(hours);
      return true;
    });
  }

  @Override
  public void onDestroyView()
  {
    super.onDestroyView();
    LocationHelper.from(requireContext()).removeListener(this);
  }

  @Override
  public void onLocationUpdated(@NonNull Location location)
  {
    //No requirement of this method here
  }

  //This method has been added to make the toggle switch of Record Recent Track alive
  @Override
  public void onLocationDisabled()
  {
    LocationListener.super.onLocationDisabled();
    Logger.i("kavi","yaha tak aya");
    if(mRecentTrack == null || mRecentTrack.getOnPreferenceChangeListener() == null) return;
    Logger.i("kavi","yaha tak bhi aya waah");
    mRecentTrack.getOnPreferenceChangeListener().onPreferenceChange(getPreference(getString(R.string.pref_recent_track)),false);
//    mRecentTrack.setChecked(false);
  }
}
