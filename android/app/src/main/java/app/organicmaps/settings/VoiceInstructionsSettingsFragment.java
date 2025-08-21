package app.organicmaps.settings;

import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.content.res.Configuration;
import android.os.Bundle;
import android.speech.tts.TextToSpeech;
import android.text.Spannable;
import android.text.SpannableString;
import android.text.style.ForegroundColorSpan;
import android.view.View;
import android.widget.Toast;
import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;
import androidx.preference.ListPreference;
import androidx.preference.Preference;
import androidx.preference.SeekBarPreference;
import androidx.preference.TwoStatePreference;
import app.organicmaps.R;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.settings.SpeedCameraMode;
import app.organicmaps.sdk.sound.LanguageData;
import app.organicmaps.sdk.sound.TtsPlayer;
import app.organicmaps.sdk.util.Config;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.Utils;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class VoiceInstructionsSettingsFragment extends BaseXmlSettingsFragment
{
  @NonNull
  @SuppressWarnings("NotNullFieldNotInitialized")
  private TwoStatePreference mTtsPrefEnabled;
  @NonNull
  @SuppressWarnings("NotNullFieldNotInitialized")
  private ListPreference mTtsPrefLanguages;
  @NonNull
  @SuppressWarnings("NotNullFieldNotInitialized")
  private SeekBarPreference mTtsVolume;
  @NonNull
  @SuppressWarnings("NotNullFieldNotInitialized")
  private TwoStatePreference mTtsPrefStreetNames;
  @NonNull
  @SuppressWarnings("NotNullFieldNotInitialized")
  private ListPreference mPrefLanguages;
  @NonNull
  @SuppressWarnings("NotNullFieldNotInitialized")
  private Preference mTtsLangInfo;
  @NonNull
  @SuppressWarnings("NotNullFieldNotInitialized")
  private Preference mTtsVoiceTest;

  private List<String> mTtsTestStringArray;
  private int mTestStringIndex;

  // Track if voice preferences are currently visible and should stay constant
  private boolean mVoicePreferencesVisible = false;
  // Track if this is the first time setting up the fragment
  private boolean mIsFirstSetup = true;

  @NonNull
  private final Map<String, LanguageData> mLanguages = new HashMap<>();
  private LanguageData mCurrentLanguage;
  private String mSelectedLanguage;

  private final ActivityResultLauncher<Intent> startInstallDataIntentForResult =
          registerForActivityResult(new ActivityResultContracts.StartActivityForResult(), activityResult -> {
            if (activityResult.getResultCode() == Activity.RESULT_OK)
            {
              onInstallDataResult();
            }
          });

  private final Preference.OnPreferenceChangeListener mEnabledListener = (preference, newValue) ->
  {
    final boolean set = (Boolean) newValue;

    if (!set)
    {
      // Disabling voice instructions - hide preferences and mark as not visible
      TtsPlayer.setEnabled(false);
      hideVoicePreferences();
      mVoicePreferencesVisible = false;
      mTtsLangInfo.setSummary(R.string.prefs_languages_information_off);
      return true;
    }

    // Enabling voice instructions
    mTtsLangInfo.setSummary(R.string.prefs_languages_information);

    if (mIsFirstSetup && TtsPlayer.isEnabled())
    {
      showVoicePreferencesImmediately();
      mVoicePreferencesVisible = true;
    }
    else if (!mVoicePreferencesVisible)
    {
      showVoicePreferencesWithAnimation();
      mVoicePreferencesVisible = true;
    }

    if (mCurrentLanguage != null && mCurrentLanguage.downloaded)
    {
      setLanguage(mCurrentLanguage);
      return true;
    }

    return false;
  };

  private final Preference.OnPreferenceChangeListener mLangListener = (preference, newValue) ->
  {
    if (newValue == null)
      return false;

    mSelectedLanguage = (String) newValue;
    final LanguageData lang = mLanguages.get(mSelectedLanguage);
    if (lang == null)
      return false;

    if (lang.downloaded)
      setLanguage(lang);
    else
      UiUtils.startActivityForResult(startInstallDataIntentForResult,
              new Intent(TextToSpeech.Engine.ACTION_INSTALL_TTS_DATA));

    return false;
  };

  private final Preference.OnPreferenceChangeListener mStreetNameListener = (preference, newValue) ->
  {
    boolean set = (Boolean) newValue;
    Config.TTS.setAnnounceStreets(set);

    return true;
  };

  @Override
  protected int getXmlResources()
  {
    return R.xml.prefs_voice_instructions;
  }

  @Override
  public void onCreatePreferences(Bundle bundle, String root)
  {
    super.onCreatePreferences(bundle, root);

    if (TtsPlayer.isEnabled())
    {
      Preference langPref = findPreference(getString(R.string.pref_tts_language));
      Preference streetPref = findPreference(getString(R.string.pref_tts_street_names));
      Preference volumePref = findPreference(getString(R.string.pref_tts_volume));
      Preference testPref = findPreference(getString(R.string.pref_tts_test_voice));

      if (langPref != null) langPref.setVisible(true);
      if (streetPref != null) streetPref.setVisible(true);
      if (volumePref != null) volumePref.setVisible(true);
      if (testPref != null) testPref.setVisible(true);

      mVoicePreferencesVisible = true;
    }
  }

  @Override
  public void onViewCreated(View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    mTtsPrefEnabled = getPreference(getString(R.string.pref_tts_enabled));
    mTtsPrefLanguages = getPreference(getString(R.string.pref_tts_language));
    mTtsPrefStreetNames = findPreference(getString(R.string.pref_tts_street_names));
    mTtsLangInfo = getPreference(getString(R.string.pref_tts_info));

    Preference mTtsOpenSystemSettings = getPreference(getString(R.string.pref_tts_open_system_settings));
    mTtsOpenSystemSettings.setOnPreferenceClickListener(pref -> {
      try
      {
        final Intent intent =
                new Intent().setAction("com.android.settings.TTS_SETTINGS").setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        startActivity(intent);
        return true;
      }
      catch (ActivityNotFoundException e)
      {
        CharSequence noTtsSettingString = getString(R.string.pref_tts_no_system_tts);
        Toast.makeText(super.getSettingsActivity(), noTtsSettingString, Toast.LENGTH_LONG).show();
        return false;
      }
    });

    mTtsVoiceTest = getPreference(getString(R.string.pref_tts_test_voice));
    mTtsVoiceTest.setOnPreferenceClickListener(pref -> {
      if (mTtsTestStringArray == null)
        return false;

      Utils.showSnackbar(view, getString(R.string.pref_tts_playing_test_voice));
      TtsPlayer.INSTANCE.speak(mTtsTestStringArray.get(mTestStringIndex));
      mTestStringIndex++;
      if (mTestStringIndex >= mTtsTestStringArray.size())
        mTestStringIndex = 0;
      return true;
    });

    TtsPlayer.sOnReloadCallback = () ->
    {
      Toast.makeText(requireContext(), "TTS engine reloaded", Toast.LENGTH_SHORT).show();
      updateTts();
    };

    // Initialize all preferences
    initVolume();
    initTtsLangInfoLink();
    initSpeedCamerasPrefs();

    updateTts();
  }

  @Override
  public void onResume()
  {
    super.onResume();
    updateTts();
  }

  @Override
  public void onDestroyView()
  {
    TtsPlayer.sOnReloadCallback = null;
    super.onDestroyView();
  }

  private void hideVoicePreferences()
  {
    mTtsPrefLanguages.setVisible(false);
    mTtsPrefStreetNames.setVisible(false);
    mTtsVolume.setVisible(false);
    mTtsVoiceTest.setVisible(false);
  }

  private void showVoicePreferencesImmediately()
  {
    mTtsPrefLanguages.setVisible(true);
    mTtsPrefStreetNames.setVisible(true);
    mTtsVolume.setVisible(true);
    mTtsVoiceTest.setVisible(true);
  }

  private void showVoicePreferencesWithAnimation()
  {
    // This will cause the preferences to slide down when they become visible
    mTtsPrefLanguages.setVisible(true);
    mTtsPrefStreetNames.setVisible(true);
    mTtsVolume.setVisible(true);
    mTtsVoiceTest.setVisible(true);
  }

  private void onInstallDataResult()
  {
    updateTts();

    LanguageData lang = mLanguages.get(mSelectedLanguage);
    if (lang != null && lang.downloaded)
      setLanguage(lang);
  }

  private void enableListeners(boolean enable)
  {
    mTtsPrefEnabled.setOnPreferenceChangeListener(enable ? mEnabledListener : null);
    mTtsPrefLanguages.setOnPreferenceChangeListener(enable ? mLangListener : null);
    mTtsPrefStreetNames.setOnPreferenceChangeListener(enable ? mStreetNameListener : null);
  }

  private void setLanguage(@NonNull LanguageData lang)
  {
    TtsPlayer.setEnabled(true);
    TtsPlayer.INSTANCE.setLanguage(lang);
    mTtsPrefLanguages.setSummary(lang.name);

    updateTts();
  }

  private void updateTts()
  {
    enableListeners(false);

    final List<LanguageData> languages = TtsPlayer.INSTANCE.refreshLanguages();
    mLanguages.clear();
    mCurrentLanguage = null;
    mTtsTestStringArray = null;

    if (languages.isEmpty())
    {
      mTtsPrefEnabled.setChecked(false);
      mTtsPrefEnabled.setEnabled(false);
      mTtsPrefEnabled.setSummary(R.string.pref_tts_unavailable);
      hideVoicePreferences();
      mVoicePreferencesVisible = false;
      if (mTtsPrefLanguages != null) {
        mTtsPrefLanguages.setSummary(null);
      }
      mTtsLangInfo.setSummary(R.string.prefs_languages_information_off);

      enableListeners(true);
      return;
    }

    final CharSequence[] entries = new CharSequence[languages.size()];
    final CharSequence[] values = new CharSequence[languages.size()];
    for (int i = 0; i < languages.size(); i++)
    {
      LanguageData lang = languages.get(i);
      entries[i] = lang.name;
      values[i] = lang.internalCode;
      mLanguages.put(lang.internalCode, lang);
    }

    mTtsPrefLanguages.setEntries(entries);
    mTtsPrefLanguages.setEntryValues(values);

    mCurrentLanguage = TtsPlayer.getSelectedLanguage(languages);
    final boolean available = (mCurrentLanguage != null && mCurrentLanguage.downloaded);
    final boolean ttsEnabled = available && TtsPlayer.isEnabled();

    // Update the enabled checkbox
    mTtsPrefEnabled.setChecked(ttsEnabled);

    // ONLY set visibility during first setup or if TTS state actually changed
    if (mIsFirstSetup)
    {
      // First setup - preferences visibility was already set in onViewCreated if TTS was enabled
      // So we only need to handle the case where TTS is disabled
      if (!ttsEnabled && mVoicePreferencesVisible)
      {
        hideVoicePreferences();
        mVoicePreferencesVisible = false;
      }
      mIsFirstSetup = false;
    }
    // For subsequent calls, visibility changes are handled only by the user toggle listener
    // This prevents unwanted animations on resume/refresh

    // Always update summaries and values regardless of visibility
    mTtsPrefLanguages.setSummary(available ? mCurrentLanguage.name : null);
    mTtsPrefLanguages.setValue(available ? mCurrentLanguage.internalCode : null);

    mTtsLangInfo.setSummary(ttsEnabled ? R.string.prefs_languages_information : R.string.prefs_languages_information_off);

    if (available)
    {
      final Configuration config = new Configuration(getResources().getConfiguration());
      config.setLocale(mCurrentLanguage.locale);
      mTtsTestStringArray = Arrays.asList(
              requireContext().createConfigurationContext(config).getResources().getStringArray(R.array.app_tips));
      Collections.shuffle(mTtsTestStringArray);
      mTestStringIndex = 0;
    }

    enableListeners(true);
  }

  @SuppressWarnings("ConstantConditions")
  private void initVolume()
  {
    mTtsVolume = getPreference(getString(R.string.pref_tts_volume));
    mTtsVolume.setMin((int) (Config.TTS.Defaults.VOLUME_MIN * 100));
    mTtsVolume.setMax((int) (Config.TTS.Defaults.VOLUME_MAX * 100));
    mTtsVolume.setUpdatesContinuously(true);
    mTtsVolume.setOnPreferenceClickListener(preference -> {
      setTtsVolume(Config.TTS.Defaults.VOLUME);
      return true;
    });
    mTtsVolume.setOnPreferenceChangeListener((preference, newValue) -> {
      setTtsVolume(((float) ((int) newValue)) / 100);
      return true;
    });
    setTtsVolume(TtsPlayer.INSTANCE.getVolume());
  }

  private void setTtsVolume(final float volume)
  {
    final int volumeInt = (int) (volume * 100);
    mTtsVolume.setValue(volumeInt);
    mTtsVolume.setSummary(Integer.toString(volumeInt));
    TtsPlayer.INSTANCE.setVolume(volume);
  }

  private void initTtsLangInfoLink()
  {
    final Preference ttsLangInfoLink = getPreference(getString(R.string.pref_tts_info_link));
    final String ttsLinkText = getString(R.string.prefs_languages_information_off_link);
    final Spannable link = new SpannableString(ttsLinkText + "↗");
    // Set link color.
    link.setSpan(
            new ForegroundColorSpan(ContextCompat.getColor(
                    requireContext(), UiUtils.getStyledResourceId(requireContext(), androidx.appcompat.R.attr.colorAccent))),
            0, ttsLinkText.length(), 0);
    ttsLangInfoLink.setSummary(link);

    final String ttsInfoUrl = requireActivity().getString(R.string.tts_info_link);
    ttsLangInfoLink.setOnPreferenceClickListener(preference -> {
      Utils.openUrl(requireContext(), ttsInfoUrl);
      return false;
    });
  }

  private void initSpeedCamerasPrefs()
  {
    final ListPreference pref = getPreference(getString(R.string.pref_tts_speed_cameras));
    pref.setSummary(pref.getEntry());
    pref.setOnPreferenceChangeListener((preference, newValue) -> {
      final String speedCamModeValue = (String) newValue;
      final SpeedCameraMode newCamMode = SpeedCameraMode.valueOf(speedCamModeValue);
      final CharSequence summary = pref.getEntries()[newCamMode.ordinal()];
      pref.setSummary(summary);
      if (pref.getValue().equals(newValue))
        return true;

      onSpeedCamerasPrefChanged(newCamMode);
      return true;
    });
  }

  private void onSpeedCamerasPrefChanged(@NonNull SpeedCameraMode newCamMode)
  {
    Framework.setSpeedCamerasMode(newCamMode);
  }
}