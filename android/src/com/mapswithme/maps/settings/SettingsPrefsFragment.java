package com.mapswithme.maps.settings;

import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.speech.tts.TextToSpeech;
import android.text.Spannable;
import android.text.SpannableString;
import android.text.style.ForegroundColorSpan;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.appcompat.app.AlertDialog;
import androidx.core.content.ContextCompat;
import androidx.fragment.app.Fragment;
import androidx.preference.ListPreference;
import androidx.preference.Preference;
import androidx.preference.PreferenceCategory;
import androidx.preference.PreferenceScreen;
import androidx.preference.TwoStatePreference;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.maps.downloader.MapManager;
import com.mapswithme.maps.downloader.OnmapDownloader;
import com.mapswithme.maps.editor.ProfileActivity;
import com.mapswithme.maps.help.HelpActivity;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.location.LocationProviderFactory;
import com.mapswithme.maps.sound.LanguageData;
import com.mapswithme.maps.sound.TtsPlayer;
import com.mapswithme.util.Config;
import com.mapswithme.util.CrashlyticsUtils;
import com.mapswithme.util.NetworkPolicy;
import com.mapswithme.util.PowerManagment;
import com.mapswithme.util.SharedPropertiesUtils;
import com.mapswithme.util.ThemeSwitcher;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.log.LogsManager;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class SettingsPrefsFragment extends BaseXmlSettingsFragment
{
  private static final int REQUEST_INSTALL_DATA = 1;

  @Nullable
  private Preference mStoragePref;

  @Nullable
  private TwoStatePreference mPrefEnabled;
  @Nullable
  private ListPreference mPrefLanguages;
  @Nullable
  private Preference mLangInfo;
  @Nullable
  private Preference mLangInfoLink;
  private PreferenceScreen mPreferenceScreen;

  @NonNull
  private final Map<String, LanguageData> mLanguages = new HashMap<>();
  private LanguageData mCurrentLanguage;
  private String mSelectedLanguage;

  private final Preference.OnPreferenceChangeListener mEnabledListener = new Preference.OnPreferenceChangeListener()
  {
    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue)
    {
      Preference root = findPreference(getString(R.string.pref_tts_screen));
      boolean set = (Boolean)newValue;
      if (!set)
      {
        TtsPlayer.setEnabled(false);
        if (mPrefLanguages != null)
          mPrefLanguages.setEnabled(false);
        if (mLangInfo != null)
          mLangInfo.setSummary(R.string.prefs_languages_information_off);
        if (mLangInfoLink != null && isOnTtsScreen())
          getPreferenceScreen().addPreference(mLangInfoLink);

        if (root != null)
          root.setSummary(R.string.off);

        if (mPrefEnabled != null)
          mPrefEnabled.setTitle(R.string.off);
        return true;
      }

      if (mLangInfo != null)
        mLangInfo.setSummary(R.string.prefs_languages_information);
      if (root != null)
        root.setSummary(R.string.on);
      if (mPrefEnabled != null)
        mPrefEnabled.setTitle(R.string.on);
      if (mLangInfoLink != null)
        removePreference(getString(R.string.pref_navigation), mLangInfoLink);

      if (mCurrentLanguage != null && mCurrentLanguage.downloaded)
      {
        setLanguage(mCurrentLanguage);
        return true;
      }

      return false;
    }
  };

  private final Preference.OnPreferenceChangeListener mLangListener = new Preference.OnPreferenceChangeListener()
  {
    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue)
    {
      if (newValue == null)
        return false;

      mSelectedLanguage = (String)newValue;
      LanguageData lang = mLanguages.get(mSelectedLanguage);
      if (lang == null)
        return false;

      if (lang.downloaded)
        setLanguage(lang);
      else
        startActivityForResult(new Intent(TextToSpeech.Engine.ACTION_INSTALL_TTS_DATA), REQUEST_INSTALL_DATA);

      return false;
    }
  };

  private void enableListeners(boolean enable)
  {
    if (mPrefEnabled != null)
      mPrefEnabled.setOnPreferenceChangeListener(enable ? mEnabledListener : null);
    if (mPrefLanguages != null)
      mPrefLanguages.setOnPreferenceChangeListener(enable ? mLangListener : null);
  }

  private void setLanguage(@NonNull LanguageData lang)
  {
    Config.setTtsEnabled(true);
    TtsPlayer.INSTANCE.setLanguage(lang);
    if (mPrefLanguages != null)
      mPrefLanguages.setSummary(lang.name);

    updateTts();
  }

  private void updateTts()
  {
    if (mPrefEnabled == null || mPrefLanguages == null || mLangInfo == null || mLangInfoLink == null)
      return;

    enableListeners(false);

    List<LanguageData> languages = TtsPlayer.INSTANCE.refreshLanguages();
    mLanguages.clear();
    mCurrentLanguage = null;

    Preference root = findPreference(getString(R.string.pref_tts_screen));

    if (languages.isEmpty())
    {
      mPrefEnabled.setChecked(false);
      mPrefEnabled.setEnabled(false);
      mPrefEnabled.setSummary(R.string.pref_tts_unavailable);
      mPrefEnabled.setTitle(R.string.off);
      mPrefLanguages.setEnabled(false);
      mPrefLanguages.setSummary(null);
      mLangInfo.setSummary(R.string.prefs_languages_information_off);
      if (isOnTtsScreen())
        getPreferenceScreen().addPreference(mLangInfoLink);
      if (root != null)
        root.setSummary(R.string.off);

      enableListeners(true);
      return;
    }

    boolean enabled = TtsPlayer.isEnabled();
    mPrefEnabled.setChecked(enabled);
    mPrefEnabled.setSummary(null);
    mPrefEnabled.setTitle(enabled ? R.string.on : R.string.off);
    mLangInfo.setSummary(enabled ? R.string.prefs_languages_information
                                 : R.string.prefs_languages_information_off);
    if (enabled)
      removePreference(getString(R.string.pref_navigation), mLangInfoLink);
    else if (isOnTtsScreen())
      getPreferenceScreen().addPreference(mLangInfoLink);

    if (root != null)
      root.setSummary(enabled ? R.string.on : R.string.off);

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

    mCurrentLanguage = TtsPlayer.getSelectedLanguage(languages);
    boolean available = (mCurrentLanguage != null && mCurrentLanguage.downloaded);
    mPrefLanguages.setEnabled(available && TtsPlayer.isEnabled());
    mPrefLanguages.setSummary(available ? mCurrentLanguage.name : null);
    mPrefLanguages.setValue(available ? mCurrentLanguage.internalCode : null);
    mPrefEnabled.setChecked(available && TtsPlayer.isEnabled());

    enableListeners(true);
  }

  private boolean isOnTtsScreen()
  {
    String ttsScreenKey = requireActivity().getString(R.string.pref_tts_screen);
    return mPreferenceScreen.getKey() != null && mPreferenceScreen.getKey().equals(ttsScreenKey);
  }

  @Override
  protected int getXmlResources()
  {
    return R.xml.prefs_main;
  }

  private boolean onToggleCrashReports(Object newValue)
  {
    boolean isEnabled = (boolean) newValue;
    CrashlyticsUtils.INSTANCE.setEnabled(isEnabled);
    return true;
  }

  @Override
  public void onViewCreated(View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    mPreferenceScreen = getPreferenceScreen();
    mStoragePref = findPreference(getString(R.string.pref_storage));
    mPrefEnabled = findPreference(getString(R.string.pref_tts_enabled));
    mPrefLanguages = findPreference(getString(R.string.pref_tts_language));
    mLangInfo = findPreference(getString(R.string.pref_tts_info));
    mLangInfoLink = findPreference(getString(R.string.pref_tts_info_link));
    initLangInfoLink();
    initStoragePrefCallbacks();
    initMeasureUnitsPrefsCallbacks();
    initZoomPrefsCallbacks();
    initMapStylePrefsCallbacks();
    initSpeedCamerasPrefs();
    initAutoDownloadPrefsCallbacks();
    initLargeFontSizePrefsCallbacks();
    initTransliterationPrefsCallbacks();
    init3dModePrefsCallbacks();
    initPerspectivePrefsCallbacks();
    initAutoZoomPrefsCallbacks();
    initLoggingEnabledPrefsCallbacks();
    initEmulationBadStorage();
    initUseMobileDataPrefsCallbacks();
    initPowerManagementPrefsCallbacks();
    boolean playServices = initPlayServicesPrefsCallbacks();
    boolean crashReports = initCrashReports();
    if (!playServices && !crashReports)
    {
      // Remove "Tracking" section completely.
      PreferenceCategory tracking = findPreference(getString(R.string.pref_subtittle_opt_out));
      if (tracking != null)
        mPreferenceScreen.removePreference(tracking);
    }
    initScreenSleepEnabledPrefsCallbacks();
    initShowOnLockScreenPrefsCallbacks();
    updateTts();
  }

  private void initSpeedCamerasPrefs()
  {
    String key = getString(R.string.pref_speed_cameras);
    final ListPreference pref = findPreference(key);
    // TODO: check whether it's needed #2049
    if (pref == null)
      return;
    pref.setSummary(pref.getEntry());
    pref.setOnPreferenceChangeListener(this::onSpeedCamerasPrefSelected);
  }

  private boolean onSpeedCamerasPrefSelected(@NonNull Preference preference,
                                             @NonNull Object newValue)
  {
    String speedCamModeValue = (String) newValue;
    ListPreference speedCamModeList = (ListPreference) preference;
    SpeedCameraMode newCamMode = SpeedCameraMode.valueOf(speedCamModeValue);
    CharSequence summary = speedCamModeList.getEntries()[newCamMode.ordinal()];
    speedCamModeList.setSummary(summary);
    if (speedCamModeList.getValue().equals(newValue))
      return true;

    SpeedCameraMode oldCamMode = SpeedCameraMode.valueOf(speedCamModeList.getValue());
    onSpeedCamerasPrefChanged(oldCamMode, newCamMode);
    return true;
  }

  private void onSpeedCamerasPrefChanged(@NonNull SpeedCameraMode oldCamMode,
                                         @NonNull SpeedCameraMode newCamMode)
  {
    Framework.setSpeedCamerasMode(newCamMode);
  }

  @Override
  public void onResume()
  {
    super.onResume();

    updateTts();
  }

  @Override
  public void onActivityResult(int requestCode, int resultCode, Intent data)
  {
    // Do not check resultCode here as it is always RESULT_CANCELED
    super.onActivityResult(requestCode, resultCode, data);

    if (requestCode == REQUEST_INSTALL_DATA)
    {
      updateTts();

      LanguageData lang = mLanguages.get(mSelectedLanguage);
      if (lang != null && lang.downloaded)
        setLanguage(lang);
    }
  }

  @Override
  public boolean onPreferenceTreeClick(Preference preference)
  {
    if (preference.getKey() != null && preference.getKey().equals(getString(R.string.pref_osm_profile)))
    {
      startActivity(new Intent(requireActivity(), ProfileActivity.class));
    }
    else if (preference.getKey() != null && preference.getKey().equals(getString(R.string.pref_help)))
    {
      startActivity(new Intent(requireActivity(), HelpActivity.class));
    }
    return super.onPreferenceTreeClick(preference);
  }

  private void initLangInfoLink()
  {
    if (mLangInfoLink != null)
    {
      Spannable link = new SpannableString(getString(R.string.prefs_languages_information_off_link));
      link.setSpan(new ForegroundColorSpan(ContextCompat.getColor(requireContext(),
                                                                  UiUtils.getStyledResourceId(requireContext(), R.attr.colorAccent))),
                   0, link.length(), 0);
      mLangInfoLink.setSummary(link);
      String TTS_INFO_LINK = requireActivity().getString(R.string.tts_info_link);
      mLangInfoLink.setOnPreferenceClickListener(preference -> {
        final Intent intent = new Intent(Intent.ACTION_VIEW);
        intent.setData(Uri.parse(TTS_INFO_LINK));
        requireContext().startActivity(intent);
        return false;
      });
      removePreference(getString(R.string.pref_navigation), mLangInfoLink);
    }
  }

  private void initLargeFontSizePrefsCallbacks()
  {
    Preference pref = findPreference(getString(R.string.pref_large_fonts_size));
    // TODO: check whether it's needed #2049
    if (pref == null)
      return;

    ((TwoStatePreference)pref).setChecked(Config.isLargeFontsSize());
    pref.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener()
    {
      @Override
      public boolean onPreferenceChange(Preference preference, Object newValue)
      {
        boolean oldVal = Config.isLargeFontsSize();
        boolean newVal = (Boolean) newValue;
        if (oldVal != newVal)
          Config.setLargeFontsSize(newVal);

        return true;
      }
    });
  }

  private void initTransliterationPrefsCallbacks()
  {
    Preference pref = findPreference(getString(R.string.pref_transliteration));
    // TODO: check whether it's needed #2049
    if (pref == null)
      return;

    ((TwoStatePreference)pref).setChecked(Config.isTransliteration());
    pref.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener()
    {
      @Override
      public boolean onPreferenceChange(Preference preference, Object newValue)
      {
        boolean oldVal = Config.isTransliteration();
        boolean newVal = (Boolean) newValue;
        if (oldVal != newVal)
          Config.setTransliteration(newVal);

        return true;
      }
    });
  }

  private void initUseMobileDataPrefsCallbacks()
  {
    final ListPreference mobilePref = findPreference(
        getString(R.string.pref_use_mobile_data));
    if (mobilePref == null)
      return;

    NetworkPolicy.Type curValue = Config.getUseMobileDataSettings();
    if (curValue == NetworkPolicy.Type.NOT_TODAY || curValue == NetworkPolicy.Type.TODAY)
        curValue = NetworkPolicy.Type.ASK;
    mobilePref.setValue(curValue.name());
    mobilePref.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener()
    {
      @Override
      public boolean onPreferenceChange(Preference preference, Object newValue)
      {
        String valueStr = (String)newValue;
        NetworkPolicy.Type type = NetworkPolicy.Type.valueOf(valueStr);
        Config.setUseMobileDataSettings(type);
        return true;
      }
    });
  }

  private void initPowerManagementPrefsCallbacks()
  {
    final ListPreference powerManagementPref = findPreference(
      getString(R.string.pref_power_management));
    if (powerManagementPref == null)
      return;

    @PowerManagment.SchemeType
    int curValue = PowerManagment.getScheme();
    powerManagementPref.setValue(String.valueOf(curValue));

    powerManagementPref.setOnPreferenceChangeListener((preference, newValue) ->
    {
      @PowerManagment.SchemeType
      int scheme = Integer.parseInt((String) newValue);

      PowerManagment.setScheme(scheme);

      return true;
    });
  }

  private void initLoggingEnabledPrefsCallbacks()
  {
    Preference pref = findPreference(getString(R.string.pref_enable_logging));
    // TODO: check whether it's needed #2049
    if (pref == null)
      return;

    ((TwoStatePreference) pref).setChecked(LogsManager.INSTANCE.isFileLoggingEnabled());
    pref.setOnPreferenceChangeListener((preference, newValue) -> {
      if (!LogsManager.INSTANCE.setFileLoggingEnabled((Boolean) newValue))
      {
        // It's a very rare condition when debugging, so we can do without translation.
        Utils.showSnackbar(getView(), "ERROR: Can't create a logs folder!");
        return false;
      }
      return true;
    });
  }

  private void initEmulationBadStorage()
  {
    Preference pref = findPreference(getString(R.string.pref_emulate_bad_external_storage));
    // TODO: check whether it's needed #2049
    if (pref == null)
      return;

    if (!SharedPropertiesUtils.shouldShowEmulateBadStorageSetting(requireContext()))
      removePreference(getString(R.string.pref_settings_general), pref);
  }

  private void initAutoZoomPrefsCallbacks()
  {
    final TwoStatePreference pref = findPreference(getString(R.string.pref_auto_zoom));
    // TODO: check whether it's needed #2049
    if (pref == null)
      return;

    boolean autozoomEnabled = Framework.nativeGetAutoZoomEnabled();
    pref.setChecked(autozoomEnabled);
    pref.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener()
    {
      @Override
      public boolean onPreferenceChange(Preference preference, Object newValue)
      {
        Framework.nativeSetAutoZoomEnabled((Boolean)newValue);
        return true;
      }
    });
  }

  private boolean initPlayServicesPrefsCallbacks()
  {
    Preference pref = findPreference(getString(R.string.pref_play_services));
    // TODO: check whether it's needed #2049
    if (pref == null)
      return false;

    if (!LocationProviderFactory.isGoogleLocationAvailable(requireActivity().getApplicationContext()))
    {
      removePreference(getString(R.string.pref_subtittle_opt_out), pref);
      return false;
    }
    else
    {
      ((TwoStatePreference) pref).setChecked(Config.useGoogleServices());
      pref.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener()
      {
        @Override
        public boolean onPreferenceChange(Preference preference, Object newValue)
        {
          boolean oldVal = Config.useGoogleServices();
          boolean newVal = (Boolean) newValue;
          if (oldVal != newVal)
          {
            Config.setUseGoogleService(newVal);
            LocationHelper.INSTANCE.restart();
          }
          return true;
        }
      });
      return true;
    }
  }

  private void init3dModePrefsCallbacks()
  {
    final TwoStatePreference pref = findPreference(getString(R.string.pref_3d_buildings));
    // TODO: check whether it's needed #2049
    if (pref == null)
      return;

    final Framework.Params3dMode _3d = new Framework.Params3dMode();
    Framework.nativeGet3dMode(_3d);

    pref.setChecked(_3d.buildings);

    pref.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener()
    {
      @Override
      public boolean onPreferenceChange(Preference preference, Object newValue)
      {
        Framework.nativeSet3dMode(_3d.enabled, (Boolean)newValue);
        return true;
      }
    });
  }

  private void initPerspectivePrefsCallbacks()
  {
    final TwoStatePreference pref = findPreference(getString(R.string.pref_3d));
    // TODO: check whether it's needed #2049
    if (pref == null)
      return;

    final Framework.Params3dMode _3d = new Framework.Params3dMode();
    Framework.nativeGet3dMode(_3d);

    pref.setChecked(_3d.enabled);

    pref.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener()
    {
      @Override
      public boolean onPreferenceChange(Preference preference, Object newValue)
      {
        Framework.nativeSet3dMode((Boolean) newValue, _3d.buildings);
        return true;
      }
    });
  }

  private void initAutoDownloadPrefsCallbacks()
  {
    TwoStatePreference pref = findPreference(getString(R.string.pref_autodownload));
    // TODO: check whether it's needed #2049
    if (pref == null)
      return;

    pref.setChecked(Config.isAutodownloadEnabled());
    pref.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener()
    {
      @Override
      public boolean onPreferenceChange(Preference preference, Object newValue)
      {
        boolean value = (Boolean)newValue;
        Config.setAutodownloadEnabled(value);

        if (value)
          OnmapDownloader.setAutodownloadLocked(false);

        return true;
      }
    });
  }

  private void initMapStylePrefsCallbacks()
  {
    final ListPreference pref = findPreference(getString(R.string.pref_map_style));
    // TODO: check whether it's needed #2049
    if (pref == null)
      return;

    String curTheme = Config.getUiThemeSettings(requireContext());
    pref.setValue(curTheme);
    pref.setSummary(pref.getEntry());
    pref.setOnPreferenceChangeListener(this::onMapStylePrefChanged);
  }

  private boolean onMapStylePrefChanged(@NonNull Preference pref, @NonNull Object newValue)
  {
    String themeName = (String) newValue;
    if (!Config.setUiThemeSettings(requireContext(), themeName))
      return true;

    ThemeSwitcher.INSTANCE.restart(false);
    ListPreference mapStyleModeList = (ListPreference) pref;

    ThemeMode mode = ThemeMode.getInstance(requireContext().getApplicationContext(), themeName);
    CharSequence summary = mapStyleModeList.getEntries()[mode.ordinal()];
    mapStyleModeList.setSummary(summary);
    return true;
  }

  private void initZoomPrefsCallbacks()
  {
    Preference pref = findPreference(getString(R.string.pref_show_zoom_buttons));
    // TODO: check whether it's needed #2049
    if (pref == null)
      return;

    ((TwoStatePreference)pref).setChecked(Config.showZoomButtons());
    pref.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener()
    {
      @Override
      public boolean onPreferenceChange(Preference preference, Object newValue)
      {
        Config.setShowZoomButtons((Boolean) newValue);
        return true;
      }
    });
  }

  private void initMeasureUnitsPrefsCallbacks()
  {
    Preference pref = findPreference(getString(R.string.pref_munits));
    // TODO: check whether it's needed #2049
    if (pref == null)
      return;

    ((ListPreference)pref).setValue(String.valueOf(UnitLocale.getUnits()));
    pref.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener()
    {
      @Override
      public boolean onPreferenceChange(Preference preference, Object newValue)
      {
        UnitLocale.setUnits(Integer.parseInt((String) newValue));
        return true;
      }
    });
  }

  private void initStoragePrefCallbacks()
  {
    if (mStoragePref == null)
      return;

    mStoragePref.setOnPreferenceClickListener(new Preference.OnPreferenceClickListener()
    {
      @Override
      public boolean onPreferenceClick(Preference preference)
      {
        if (MapManager.nativeIsDownloading())
          new AlertDialog.Builder(requireActivity())
              .setTitle(getString(R.string.downloading_is_active))
              .setMessage(getString(R.string.cant_change_this_setting))
              .setPositiveButton(getString(R.string.ok), null)
              .show();
        else
//          getSettingsActivity().switchToFragment(StoragePathFragment.class, R.string.maps_storage);
          getSettingsActivity().stackFragment(StoragePathFragment.class,
                                                getString(R.string.maps_storage), null);

        return true;
      }
    });
  }

  private boolean initCrashReports()
  {
    String key = getString(R.string.pref_crash_reports);
    Preference pref = findPreference(key);
    // TODO: check whether it's needed #2049
    if (pref == null)
      return false;

    if (!CrashlyticsUtils.INSTANCE.isAvailable())
    {
      removePreference(getString(R.string.pref_subtittle_opt_out), pref);
      return false;
    }

    ((TwoStatePreference)pref).setChecked(CrashlyticsUtils.INSTANCE.isEnabled());
    pref.setOnPreferenceChangeListener((preference, newValue) -> onToggleCrashReports(newValue));
    return true;
  }

  private void initScreenSleepEnabledPrefsCallbacks()
  {
    Preference pref = findPreference(getString(R.string.pref_screen_sleep));
    // TODO: check whether it's needed #2049
    if (pref == null)
      return;

    final boolean isScreenSleepEnabled = Config.isScreenSleepEnabled();
    ((TwoStatePreference) pref).setChecked(isScreenSleepEnabled);
    pref.setOnPreferenceChangeListener(
        (preference, newValue) ->
        {
          boolean newVal = (Boolean) newValue;
          if (isScreenSleepEnabled != newVal)
          {
            Config.setScreenSleepEnabled(newVal);
            Utils.keepScreenOn(!newVal, requireActivity().getWindow());
          }
          return true;
        });
  }

  private void initShowOnLockScreenPrefsCallbacks()
  {
    Preference pref = findPreference(getString(R.string.pref_show_on_lock_screen));
    // TODO: check whether it's needed #2049
    if (pref == null)
      return;

    final boolean isShowOnLockScreenEnabled = Config.isShowOnLockScreenEnabled();
    ((TwoStatePreference) pref).setChecked(isShowOnLockScreenEnabled);
    pref.setOnPreferenceChangeListener(
            (preference, newValue) ->
            {
              boolean newVal = (Boolean) newValue;
              if (isShowOnLockScreenEnabled != newVal)
              {
                Config.setShowOnLockScreenEnabled(newVal);
                Utils.showOnLockScreen(newVal, requireActivity());
              }
              return true;
            });
  }

  private void removePreference(@NonNull String categoryKey, @NonNull Preference preference)
  {
    PreferenceCategory category = findPreference(categoryKey);
    // TODO: check whether it's needed #2049
    if (category == null)
      return;

    category.removePreference(preference);
  }

  enum ThemeMode
  {
    DEFAULT(R.string.theme_default),
    NIGHT(R.string.theme_night),
    AUTO(R.string.theme_auto);

    private final int mModeStringId;

    ThemeMode(@StringRes int modeStringId)
    {
      mModeStringId = modeStringId;
    }

    @NonNull
    public static ThemeMode getInstance(@NonNull Context context, @NonNull String src)
    {
      for (ThemeMode each : values())
      {
        if (context.getResources().getString(each.mModeStringId).equals(src))
          return each;
      }
      return AUTO;
    }
  }

  public enum SpeedCameraMode
  {
    AUTO,
    ALWAYS,
    NEVER
  }
}
