package app.organicmaps.settings;

import android.content.Context;
import android.os.Bundle;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.XmlRes;
import androidx.core.content.ContextCompat;
import androidx.preference.Preference;
import androidx.preference.PreferenceFragmentCompat;

import app.organicmaps.R;
import app.organicmaps.util.ThemeUtils;
import app.organicmaps.util.Utils;

abstract class BaseXmlSettingsFragment extends PreferenceFragmentCompat
{
  protected abstract @XmlRes int getXmlResources();

  @NonNull
  public <T extends Preference> T getPreference(@NonNull CharSequence key)
  {
    final T pref = findPreference(key);
    if (pref == null)
      throw new RuntimeException("Can't get preference by key: "+key);
    return pref;
  }
  @Override
  public void onCreatePreferences(Bundle bundle, String root)
  {
    setPreferencesFromResource(getXmlResources(), root);
  }

  @Override
  public void onAttach(Context context)
  {
    super.onAttach(context);
    Utils.detachFragmentIfCoreNotInitialized(context, this);
  }

  @Override
  public void onViewCreated(View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    int color;
    if (ThemeUtils.isDefaultTheme(requireContext()))
      color = ContextCompat.getColor(requireContext(), R.color.bg_cards);
    else
      color = ContextCompat.getColor(requireContext(), R.color.bg_cards_night);
    view.setBackgroundColor(color);
  }

  protected SettingsActivity getSettingsActivity()
  {
    return (SettingsActivity) requireActivity();
  }
}
