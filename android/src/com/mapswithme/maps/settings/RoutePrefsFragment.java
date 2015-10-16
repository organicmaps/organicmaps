package com.mapswithme.maps.settings;

import android.content.Intent;
import android.os.Bundle;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceFragment;
import android.preference.SwitchPreference;
import android.speech.tts.TextToSpeech;
import android.support.annotation.NonNull;
import com.mapswithme.maps.R;
import com.mapswithme.maps.sound.LanguageData;
import com.mapswithme.maps.sound.TtsPlayer;
import com.mapswithme.util.Config;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class RoutePrefsFragment extends PreferenceFragment
{
  private static final int REQUEST_INSTALL_DATA = 1;

  private SwitchPreference mPrefEnabled;
  private ListPreference mPrefLanguages;

  private final Map<String, LanguageData> mLanguages = new HashMap<>();
  private String mSelectedLanguage;

  private void setLanguage(@NonNull LanguageData lang)
  {
    TtsPlayer.INSTANCE.setLanguage(lang);
    mPrefLanguages.setSummary(lang.name);

    Config.setTtsLanguageSetByUser();
    update();
  }

  private void update()
  {
    List<LanguageData> languages = TtsPlayer.INSTANCE.getAvailableLanguages(true);
    mLanguages.clear();

    if (languages.isEmpty())
    {
      mPrefEnabled.setEnabled(false);
      mPrefEnabled.setSummary(R.string.pref_tts_unavailable);
      mPrefLanguages.setEnabled(false);
      return;
    }

    mPrefEnabled.setChecked(TtsPlayer.INSTANCE.isEnabled());

    final CharSequence[] entries = new CharSequence[languages.size()];
    final CharSequence[] values = new CharSequence[languages.size()];
    for (int i = 0; i < languages.size(); i++)
    {
      LanguageData lang = languages.get(i);
      entries[i] = lang.name;
      values[i] = lang.internalCode;

      mLanguages.put(lang.internalCode, lang);
    }

    mPrefLanguages.setEntries(entries);
    mPrefLanguages.setEntryValues(values);

    LanguageData curLang = TtsPlayer.INSTANCE.getSelectedLanguage(languages);
    if (curLang != null)
    {
      mPrefLanguages.setSummary(curLang.name);
      mPrefLanguages.setValueIndex(languages.indexOf(curLang));
    }
  }

  @Override
  public void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    addPreferencesFromResource(R.xml.prefs_route);

    mPrefEnabled = (SwitchPreference) findPreference(getString(R.string.pref_tts_enabled));
    mPrefLanguages = (ListPreference) findPreference(getString(R.string.pref_tts_language));

    update();

    mPrefEnabled.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener()
    {
      @Override
      public boolean onPreferenceChange(Preference preference, Object newValue)
      {
        boolean set = (Boolean)newValue;
        mPrefLanguages.setEnabled(set);
        Config.setTtsEnabled(set);
        TtsPlayer.INSTANCE.setEnabled(set);
        return true;
      }
    });

    mPrefLanguages.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener()
    {
      @Override
      public boolean onPreferenceChange(Preference preference, Object newValue)
      {
        mSelectedLanguage = (String)newValue;
        LanguageData lang = mLanguages.get(mSelectedLanguage);
        if (lang.getStatus() == TextToSpeech.LANG_MISSING_DATA)
          startActivityForResult(new Intent(TextToSpeech.Engine.ACTION_INSTALL_TTS_DATA), REQUEST_INSTALL_DATA);
        else
          setLanguage(lang);
        return false;
      }
    });
  }

  @Override
  public void onActivityResult(int requestCode, int resultCode, Intent data)
  {
    // Do not check resultCode here as it is always RESULT_CANCELED
    super.onActivityResult(requestCode, resultCode, data);

    if (requestCode == REQUEST_INSTALL_DATA)
    {
      update();

      LanguageData lang = mLanguages.get(mSelectedLanguage);
      if (lang != null && lang.getStatus() == TextToSpeech.LANG_AVAILABLE)
        setLanguage(lang);
    }
  }
}
