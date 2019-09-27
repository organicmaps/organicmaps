package com.mapswithme.maps.settings;

import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.preference.Preference;
import androidx.preference.PreferenceFragmentCompat;
import androidx.preference.PreferenceScreen;
import androidx.appcompat.widget.Toolbar;
import android.text.TextUtils;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseToolbarActivity;

public class SettingsActivity extends BaseToolbarActivity
                              implements PreferenceFragmentCompat.OnPreferenceStartFragmentCallback,
                                         PreferenceFragmentCompat.OnPreferenceStartScreenCallback
{
  @Nullable
  private String mLastTitle;

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
      replaceFragment(fragment, title, pref.getExtras());
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
    replaceFragment(SettingsPrefsFragment.class, preferenceScreen.getTitle().toString(), args);
    return true;
  }

  public void replaceFragment(@NonNull Class<? extends Fragment> fragmentClass,
                              @Nullable String title, @Nullable Bundle args)
  {
    final int resId = getFragmentContentResId();
    if (resId <= 0 || findViewById(resId) == null)
      throw new IllegalStateException("Fragment can't be added, since getFragmentContentResId() " +
                                      "isn't implemented or returns wrong resourceId.");

    String name = fragmentClass.getName();
    final Fragment fragment = Fragment.instantiate(this, name, args);
    getSupportFragmentManager().beginTransaction()
                               .replace(resId, fragment, name)
                               .addToBackStack(null)
                               .commitAllowingStateLoss();
    getSupportFragmentManager().executePendingTransactions();

    if (title != null)
    {
      Toolbar toolbar = getToolbar();
      if (toolbar != null && toolbar.getTitle() != null)
      {
        mLastTitle = toolbar.getTitle().toString();
        toolbar.setTitle(title);
      }
    }
  }

  @Override
  public void onBackPressed()
  {
    if (mLastTitle != null)
      getToolbar().setTitle(mLastTitle);

    super.onBackPressed();
  }
}
