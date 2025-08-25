package app.organicmaps.settings;

import android.content.Context;
import android.os.Bundle;
import android.view.View;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.XmlRes;
import androidx.core.content.ContextCompat;
import androidx.core.view.ViewCompat;
import androidx.preference.Preference;
import androidx.preference.PreferenceFragmentCompat;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.R;
import app.organicmaps.util.Utils;
import app.organicmaps.util.WindowInsetUtils.ScrollableContentInsetsListener;

abstract class BaseXmlSettingsFragment extends PreferenceFragmentCompat
{
  protected abstract @XmlRes int getXmlResources();

  @NonNull
  public <T extends Preference> T getPreference(@NonNull CharSequence key)
  {
    final T pref = findPreference(key);
    if (pref == null)
      throw new RuntimeException("Can't get preference by key: " + key);
    return pref;
  }
  @Override
  public void onCreatePreferences(Bundle bundle, String root)
  {
    setPreferencesFromResource(getXmlResources(), root);
  }

  @Override
  public void onAttach(@NonNull Context context)
  {
    super.onAttach(context);
    Utils.detachFragmentIfCoreNotInitialized(context, this);
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    view.setBackgroundColor(ContextCompat.getColor(requireContext(), R.color.bg_cards));
    RecyclerView recyclerView = getListView();
    ViewCompat.setOnApplyWindowInsetsListener(recyclerView, new ScrollableContentInsetsListener(recyclerView));
  }

  protected SettingsActivity getSettingsActivity()
  {
    return (SettingsActivity) requireActivity();
  }
}
