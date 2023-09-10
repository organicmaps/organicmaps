package app.organicmaps.settings;

import android.annotation.SuppressLint;
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
import androidx.core.content.ContextCompat;
import androidx.preference.ListPreference;
import androidx.preference.Preference;
import androidx.preference.PreferenceCategory;
import androidx.preference.PreferenceScreen;
import androidx.preference.TwoStatePreference;
import app.organicmaps.Framework;
import app.organicmaps.R;
import app.organicmaps.downloader.MapManager;
import app.organicmaps.downloader.OnmapDownloader;
import app.organicmaps.editor.ProfileActivity;
import app.organicmaps.help.HelpActivity;
import app.organicmaps.location.LocationHelper;
import app.organicmaps.location.LocationProviderFactory;
import app.organicmaps.sound.LanguageData;
import app.organicmaps.sound.TtsPlayer;
import app.organicmaps.util.Config;
import app.organicmaps.util.NetworkPolicy;
import app.organicmaps.util.PowerManagment;
import app.organicmaps.util.SharedPropertiesUtils;
import app.organicmaps.util.ThemeSwitcher;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.Utils;
import app.organicmaps.util.log.LogsManager;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class SettingsPrefsFragment extends BaseXmlSettingsFragment
{
  private static final int REQUEST_INSTALL_DATA = 1;

  @Nullable
  private Preference mStoragePref;

  @Nullable
  private TwoStatePreference mTtsPrefEnabled;
  @Nullable
  private ListPreference mTtsPrefLanguages;
  @Nullable
  private Preference mTtsLangInfo;

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
      final Preference root = getPreference(getString(R.string.pref_tts_screen));
      boolean set = (Boolean)newValue;
      if (!set)
      {
        TtsPlayer.setEnabled(false);
        if (mTtsPrefLanguages != null)
          mTtsPrefLanguages.setEnabled(false);
        if (mTtsLangInfo != null)
          mTtsLangInfo.setSummary(R.string.prefs_languages_information_off);

        root.setSummary(R.string.off);

        if (mTtsPrefEnabled != null)
          mTtsPrefEnabled.setTitle(R.string.off);
        return true;
      }

      if (mTtsLangInfo != null)
        mTtsLangInfo.setSummary(R.string.prefs_languages_information);
      root.setSummary(R.string.on);
      if (mTtsPrefEnabled != null)
        mTtsPrefEnabled.setTitle(R.string.on);

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
        UiUtils.startActivityForResult(SettingsPrefsFragment.this,
            new Intent(TextToSpeech.Engine.ACTION_INSTALL_TTS_DATA), REQUEST_INSTALL_DATA);

      return false;
    }
  };

  private void enableListeners(boolean enable)
  {
    if (mTtsPrefEnabled != null)
      mTtsPrefEnabled.setOnPreferenceChangeListener(enable ? mEnabledListener : null);
    if (mTtsPrefLanguages != null)
      mTtsPrefLanguages.setOnPreferenceChangeListener(enable ? mLangListener : null);
  }

  private void setLanguage(@NonNull LanguageData lang)
  {
    Config.setTtsEnabled(true);
    TtsPlayer.INSTANCE.setLanguage(lang);
    if (mTtsPrefLanguages != null)
      mTtsPrefLanguages.setSummary(lang.name);

    updateTts();
  }

  // Use this method only on TTS screen. Prerequisites: mTtsPrefEnabled, mTtsPrefLanguages and mTtsLangInfo are not null.
  private void updateTts()
  {
    enableListeners(false);

    List<LanguageData> languages = TtsPlayer.INSTANCE.refreshLanguages();
    mLanguages.clear();
    mCurrentLanguage = null;

    final Preference root = getPreference(getString(R.string.pref_tts_screen));

    if (languages.isEmpty())
    {
      mTtsPrefEnabled.setChecked(false);
      mTtsPrefEnabled.setEnabled(false);
      mTtsPrefEnabled.setSummary(R.string.pref_tts_unavailable);
      mTtsPrefEnabled.setTitle(R.string.off);
      mTtsPrefLanguages.setEnabled(false);
      mTtsPrefLanguages.setSummary(null);
      mTtsLangInfo.setSummary(R.string.prefs_languages_information_off);
      root.setSummary(R.string.off);

      enableListeners(true);
      return;
    }

    boolean enabled = TtsPlayer.isEnabled();
    mTtsPrefEnabled.setChecked(enabled);
    mTtsPrefEnabled.setSummary(null);
    mTtsPrefEnabled.setTitle(enabled ? R.string.on : R.string.off);
    mTtsLangInfo.setSummary(enabled ? R.string.prefs_languages_information
                                    : R.string.prefs_languages_information_off);

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

    mTtsPrefLanguages.setEntries(entries);
    mTtsPrefLanguages.setEntryValues(values);

    mCurrentLanguage = TtsPlayer.getSelectedLanguage(languages);
    boolean available = (mCurrentLanguage != null && mCurrentLanguage.downloaded);
    mTtsPrefLanguages.setEnabled(available && TtsPlayer.isEnabled());
    mTtsPrefLanguages.setSummary(available ? mCurrentLanguage.name : null);
    mTtsPrefLanguages.setValue(available ? mCurrentLanguage.internalCode : null);
    mTtsPrefEnabled.setChecked(available && TtsPlayer.isEnabled());

    enableListeners(true);
  }

  private boolean isOnMainScreen()
  {
    return mPreferenceScreen.getKey() == null;
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

  @Override
  public void onViewCreated(View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    mPreferenceScreen = getPreferenceScreen();

    if (isOnMainScreen())
    {
      // Initialize main preferences screen.
      mStoragePref = getPreference(getString(R.string.pref_storage));
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
      final boolean playServices = initPlayServicesPrefsCallbacks();
      if (!playServices)
      {
        // Remove "Tracking" section completely.
        final PreferenceCategory tracking = findPreference(getString(R.string.pref_subtittle_opt_out));
        if (tracking != null)
          mPreferenceScreen.removePreference(tracking);
      }
      initScreenSleepEnabledPrefsCallbacks();
      initShowOnLockScreenPrefsCallbacks();
    }
    else if (isOnTtsScreen())
    {
      // Initialize TTS preferences screen.
      mTtsPrefEnabled = getPreference(getString(R.string.pref_tts_enabled));
      mTtsPrefLanguages = getPreference(getString(R.string.pref_tts_language));
      mTtsLangInfo = getPreference(getString(R.string.pref_tts_info));
      initTtsLangInfoLink();
      updateTts();
    }
  }

  private void initSpeedCamerasPrefs()
  {
    String key = getString(R.string.pref_speed_cameras);
    final ListPreference pref = getPreference(key);
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

    if (isOnTtsScreen())
      updateTts();
  }

  @Override
  @SuppressWarnings("deprecation") // https://github.com/organicmaps/organicmaps/issues/3630
  public void onActivityResult(int requestCode, int resultCode, Intent data)
  {
    // Do not check resultCode here as it is always RESULT_CANCELED
    super.onActivityResult(requestCode, resultCode, data);

    if (requestCode == REQUEST_INSTALL_DATA)
    {
      if (isOnTtsScreen())
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

  // Use this method only on TTS screen.
  private void initTtsLangInfoLink()
  {
    final Preference ttsLangInfoLink = getPreference(getString(R.string.pref_tts_info_link));
    final String ttsLinkText = getString(R.string.prefs_languages_information_off_link);
    final Spannable link = new SpannableString(ttsLinkText + "↗");
    // Set link color.
    link.setSpan(new ForegroundColorSpan(ContextCompat.getColor(requireContext(),
                                                                UiUtils.getStyledResourceId(requireContext(), R.attr.colorAccent))),
                 0, ttsLinkText.length(), 0);
    ttsLangInfoLink.setSummary(link);

    final String ttsInfoUrl = requireActivity().getString(R.string.tts_info_link);
    ttsLangInfoLink.setOnPreferenceClickListener(preference -> {
      final Intent intent = new Intent(Intent.ACTION_VIEW);
      intent.setData(Uri.parse(ttsInfoUrl));
      requireContext().startActivity(intent);
      return false;
    });
  }

  private void initLargeFontSizePrefsCallbacks()
  {
    final Preference pref = getPreference(getString(R.string.pref_large_fonts_size));

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
    final Preference pref = getPreference(getString(R.string.pref_transliteration));

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
    final ListPreference mobilePref = getPreference(getString(R.string.pref_use_mobile_data));

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
    final ListPreference powerManagementPref = getPreference(getString(R.string.pref_power_management));

    @PowerManagment.SchemeType
    int curValue = PowerManagment.getScheme();
    powerManagementPref.setValue(String.valueOf(curValue));

    powerManagementPref.setOnPreferenceChangeListener((preference, newValue) ->
    {
      @PowerManagment.SchemeType
      int scheme = Integer.parseInt((String) newValue);

      PowerManagment.setScheme(scheme);

      disableOrEnable3DBuildingsForPowerMode(scheme);

      return true;
    });
  }

  private void initLoggingEnabledPrefsCallbacks()
  {
    final Preference pref = getPreference(getString(R.string.pref_enable_logging));

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
    final Preference pref = findPreference(getString(R.string.pref_emulate_bad_external_storage));
    if (pref == null)
      return;

    if (!SharedPropertiesUtils.shouldShowEmulateBadStorageSetting(requireContext()))
      removePreference(getString(R.string.pref_settings_general), pref);
  }

  private void initAutoZoomPrefsCallbacks()
  {
    final TwoStatePreference pref = getPreference(getString(R.string.pref_auto_zoom));

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
    final Preference pref = findPreference(getString(R.string.pref_play_services));
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
        @SuppressLint("MissingPermission")
        @Override
        public boolean onPreferenceChange(Preference preference, Object newValue)
        {
          final LocationHelper locationHelper = LocationHelper.from(requireContext());
          boolean oldVal = Config.useGoogleServices();
          boolean newVal = (Boolean) newValue;
          if (oldVal != newVal)
          {
            Config.setUseGoogleService(newVal);
            if (locationHelper.isActive())
            {
              locationHelper.stop();
              locationHelper.start();
            }
          }
          return true;
        }
      });
      return true;
    }
  }

  private void init3dModePrefsCallbacks()
  {
    final TwoStatePreference pref = getPreference(getString(R.string.pref_3d_buildings));

    final Framework.Params3dMode _3d = new Framework.Params3dMode();
    Framework.nativeGet3dMode(_3d);

    // Read power managements preference.
    final ListPreference powerManagementPref = getPreference(getString(R.string.pref_power_management));
    final String powerManagementValueStr = powerManagementPref.getValue();
    final Integer powerManagementValue = (powerManagementValueStr!=null) ? Integer.parseInt(powerManagementValueStr) : null;
    disableOrEnable3DBuildingsForPowerMode(powerManagementValue);

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

  // Argument `powerManagementValue` could be null on the first app run.
  private void disableOrEnable3DBuildingsForPowerMode(@Nullable Integer powerManagementValue)
  {
    final TwoStatePreference pref = getPreference(getString(R.string.pref_3d_buildings));

    if (powerManagementValue != null && powerManagementValue == PowerManagment.HIGH)
    {
      pref.setShouldDisableView(true);
      pref.setEnabled(false);
      pref.setSummary(getString(R.string.pref_map_3d_buildings_disabled_summary));
      pref.setChecked(false);
    }
    else
    {
      final Framework.Params3dMode _3d = new Framework.Params3dMode();
      Framework.nativeGet3dMode(_3d);

      pref.setShouldDisableView(false);
      pref.setEnabled(true);
      pref.setSummary("");
      pref.setChecked(_3d.buildings);
    }
  }

  private void initPerspectivePrefsCallbacks()
  {
    final TwoStatePreference pref = getPreference(getString(R.string.pref_3d));

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
    final TwoStatePreference pref = getPreference(getString(R.string.pref_autodownload));

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
    final ListPreference pref = getPreference(getString(R.string.pref_map_style));

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
    final Preference pref = getPreference(getString(R.string.pref_show_zoom_buttons));

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
    final Preference pref = getPreference(getString(R.string.pref_munits));

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
        {
          new MaterialAlertDialogBuilder(requireActivity(), R.style.MwmTheme_AlertDialog)
              .setTitle(R.string.downloading_is_active)
              .setMessage(R.string.cant_change_this_setting)
              .setPositiveButton(R.string.ok, null)
              .show();
        }
        else
        {
          getSettingsActivity().stackFragment(StoragePathFragment.class, getString(R.string.maps_storage), null);
        }

        return true;
      }
    });
  }

  private void initScreenSleepEnabledPrefsCallbacks()
  {
    final Preference pref = getPreference(getString(R.string.pref_screen_sleep));

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
    final Preference pref = getPreference(getString(R.string.pref_show_on_lock_screen));

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
    final PreferenceCategory category = getPreference(categoryKey);

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
