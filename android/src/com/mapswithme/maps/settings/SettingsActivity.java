package com.mapswithme.maps.settings;

import static android.Manifest.permission.ACCESS_BACKGROUND_LOCATION;

import android.app.Dialog;
import android.os.Bundle;
import android.text.TextUtils;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;
import androidx.core.app.ActivityCompat;
import androidx.fragment.app.Fragment;
import androidx.preference.Preference;
import androidx.preference.PreferenceFragmentCompat;
import androidx.preference.PreferenceScreen;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseToolbarActivity;
import com.mapswithme.util.PermissionsUtils;

public class SettingsActivity extends BaseToolbarActivity
                              implements PreferenceFragmentCompat.OnPreferenceStartFragmentCallback,
                                         PreferenceFragmentCompat.OnPreferenceStartScreenCallback
{
  private static final int REQ_CODE_BACKGROUND_LOCATION_PERMISSION = 1;

  @Nullable
  private Dialog mLocationErrorDialog;

  @Override
  protected int getContentLayoutResId()
  {
    return R.layout.activity_settings;
  }

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return SettingsPrefsFragment.class;
  }

  @Override
  @SuppressWarnings("unchecked")
  public boolean onPreferenceStartFragment(PreferenceFragmentCompat caller, Preference pref)
  {
    String title = TextUtils.isEmpty(pref.getTitle()) ? null : pref.getTitle().toString();
    try
    {
      Class<? extends Fragment> fragment = (Class<? extends Fragment>) Class.forName(pref.getFragment());
      stackFragment(fragment, title, pref.getExtras());
    } catch (ClassNotFoundException e)
    {
      e.printStackTrace();
    }
    return true;
  }

  @Override
  public boolean onPreferenceStartScreen(PreferenceFragmentCompat preferenceFragmentCompat, PreferenceScreen preferenceScreen)
  {
    Bundle args = new Bundle();
    args.putString(PreferenceFragmentCompat.ARG_PREFERENCE_ROOT, preferenceScreen.getKey());
    stackFragment(SettingsPrefsFragment.class, preferenceScreen.getTitle().toString(), args);
    return true;
  }

  void checkBackgroundLocationPermission()
  {
    if (PermissionsUtils.isBackgroundLocationGranted(this))
      return;

    if (mLocationErrorDialog != null && mLocationErrorDialog.isShowing())
      return;

    if (ActivityCompat.shouldShowRequestPermissionRationale(this, ACCESS_BACKGROUND_LOCATION))
    {
      mLocationErrorDialog = new AlertDialog.Builder(this)
          .setTitle(R.string.enable_location_services)
          .setMessage(R.string.recent_track_background_dialog_message)
          .setPositiveButton(R.string.ok, (dialog, which) -> {
            ActivityCompat.requestPermissions(this, new String[]{ ACCESS_BACKGROUND_LOCATION }, REQ_CODE_BACKGROUND_LOCATION_PERMISSION);
          })
          .show();
    }
    else
    {
      ActivityCompat.requestPermissions(this, new String[]{ ACCESS_BACKGROUND_LOCATION }, REQ_CODE_BACKGROUND_LOCATION_PERMISSION);
    }
  }

  @Override
  public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions,
                                         @NonNull int[] grantResults)
  {
    super.onRequestPermissionsResult(requestCode, permissions, grantResults);
    // Note: the track recorder is disabled automatically if permission is not granted.
  }
}
