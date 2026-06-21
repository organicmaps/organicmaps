package app.organicmaps.settings;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.text.TextUtils;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.preference.Preference;
import androidx.preference.PreferenceFragmentCompat;
import androidx.preference.PreferenceScreen;
import app.organicmaps.R;
import app.organicmaps.base.BaseToolbarActivity;

public class SettingsActivity
    extends BaseToolbarActivity implements PreferenceFragmentCompat.OnPreferenceStartFragmentCallback,
                                           PreferenceFragmentCompat.OnPreferenceStartScreenCallback
{
  private static final String EXTRA_OPEN_VOICE_INSTRUCTIONS = "open_voice_instructions";

  // Optional: open directly on a specific settings sub-screen (fully-qualified Fragment class name)
  // with the given toolbar title (@StringRes). Used e.g. by the Satellite layer button.
  public static final String EXTRA_FRAGMENT = "extra_fragment";
  public static final String EXTRA_TITLE = "extra_title";

  public static void startForVoiceInstructions(@NonNull Context context)
  {
    final Intent intent = new Intent(context, SettingsActivity.class).putExtra(EXTRA_OPEN_VOICE_INSTRUCTIONS, true);
    context.startActivity(intent);
  }

  @Override
  protected int getContentLayoutResId()
  {
    return R.layout.activity_settings;
  }

  @Override
  @SuppressWarnings("unchecked")
  protected Class<? extends Fragment> getFragmentClass()
  {
    final String name = getIntent().getStringExtra(EXTRA_FRAGMENT);
    if (name != null)
    {
      try
      {
        return (Class<? extends Fragment>) Class.forName(name);
      }
      catch (ClassNotFoundException e)
      {
        e.printStackTrace();
      }
    }
    return SettingsPrefsFragment.class;
  }

  @Override
  protected int getToolbarTitle()
  {
    return getIntent().getIntExtra(EXTRA_TITLE, 0);
  }

  @Override
  protected void onSafeCreate(@Nullable Bundle savedInstanceState)
  {
    super.onSafeCreate(savedInstanceState);

    if (savedInstanceState == null && getIntent().getBooleanExtra(EXTRA_OPEN_VOICE_INSTRUCTIONS, false))
      stackFragment(VoiceInstructionsSettingsFragment.class, getString(R.string.pref_tts_enable_title), null);
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
    }
    catch (ClassNotFoundException e)
    {
      e.printStackTrace();
    }
    return true;
  }

  @Override
  public boolean onPreferenceStartScreen(PreferenceFragmentCompat preferenceFragmentCompat,
                                         PreferenceScreen preferenceScreen)
  {
    Bundle args = new Bundle();
    args.putString(PreferenceFragmentCompat.ARG_PREFERENCE_ROOT, preferenceScreen.getKey());
    stackFragment(SettingsPrefsFragment.class, preferenceScreen.getTitle().toString(), args);
    return true;
  }
}
