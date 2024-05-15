package app.organicmaps.settings;

import android.Manifest;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.view.View;
import android.widget.Toast;

import androidx.annotation.Nullable;
import androidx.core.app.ActivityCompat;
import androidx.preference.ListPreference;
import androidx.preference.Preference;
import androidx.preference.TwoStatePreference;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.location.TrackRecorder;
import app.organicmaps.location.TrackRecordingService;
import app.organicmaps.util.LocationUtils;
import app.organicmaps.util.log.Logger;

import static app.organicmaps.location.TrackRecordingService.stopService;

public class TrackRecordSettingsFragment extends BaseXmlSettingsFragment
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

    mRecordTime.setValue(Integer.toString(TrackRecorder.nativeGetDuration()));
    mRecentTrack.setChecked(TrackRecorder.nativeIsEnabled());
    mRecordTime.setEnabled(TrackRecorder.nativeIsEnabled());
    mRecentTrack.setOnPreferenceChangeListener(((preference, newValue) -> {
      if (newValue == null)
        return false;

      boolean newVal = (boolean) newValue;

      if (newVal)
      {
        if (ActivityCompat.checkSelfPermission(getSettingsActivity(), Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED)
        {
          Toast.makeText(getActivity(),"Please give permission of precise location access",Toast.LENGTH_SHORT).show();
          return false;
        }
        if(!LocationUtils.areLocationServicesTurnedOn(MwmApplication.from(requireContext()))){
          Toast.makeText(getActivity(),"Please turn on location",Toast.LENGTH_SHORT).show();
          return false;
        }
        TrackRecordingService.startForegroundService(MwmApplication.from(getSettingsActivity()));
      }
      else
      {
        stopService(MwmApplication.from(requireContext()));
      }
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

      int hours = Integer.parseInt(newVal);
      TrackRecorder.nativeSetDuration(hours);
      return true;
    });
  }

}
