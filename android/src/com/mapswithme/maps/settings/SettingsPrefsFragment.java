package com.mapswithme.maps.settings;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.speech.tts.TextToSpeech;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AlertDialog;
import android.support.v7.preference.ListPreference;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceScreen;
import android.support.v7.preference.TwoStatePreference;
import android.text.Spannable;
import android.text.SpannableString;
import android.text.style.ForegroundColorSpan;

import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.GoogleApiAvailability;
import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.downloader.MapManager;
import com.mapswithme.maps.downloader.OnmapDownloader;
import com.mapswithme.maps.editor.ProfileActivity;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.location.TrackRecorder;
import com.mapswithme.maps.search.SearchFragment;
import com.mapswithme.maps.sound.LanguageData;
import com.mapswithme.maps.sound.TtsPlayer;
import com.mapswithme.util.Config;
import com.mapswithme.util.NetworkPolicy;
import com.mapswithme.util.ThemeSwitcher;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.concurrency.UiThread;
import com.mapswithme.util.log.LoggerFactory;
import com.mapswithme.util.statistics.AlohaHelper;
import com.mapswithme.util.statistics.MytargetHelper;
import com.mapswithme.util.statistics.Statistics;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class SettingsPrefsFragment extends BaseXmlSettingsFragment
{
  private static final int REQUEST_INSTALL_DATA = 1;
  private static final String TTS_SCREEN_KEY = MwmApplication.get()
                                                             .getString(R.string.pref_tts_screen);
  private static final String TTS_INFO_LINK = MwmApplication.get()
                                                            .getString(R.string.tts_info_link);

  @NonNull
  private final StoragePathManager mPathManager = new StoragePathManager();
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

  private boolean singleStorageOnly()
  {
    return !mPathManager.hasMoreThanOneStorage();
  }

  private void updateStoragePrefs()
  {
    Preference old = findPreference(getString(R.string.pref_storage));

    if (singleStorageOnly())
    {
      if (old != null)
        getPreferenceScreen().removePreference(old);
    }
    else
    {
      if (old == null && mStoragePref != null)
        getPreferenceScreen().addPreference(mStoragePref);
    }
  }

  private final Preference.OnPreferenceChangeListener mEnabledListener = new Preference.OnPreferenceChangeListener()
  {
    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue)
    {
      Statistics.INSTANCE.trackEvent(Statistics.EventName.Settings.VOICE_ENABLED, Statistics.params().add(Statistics.EventParam.ENABLED, newValue.toString()));
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
        getPreferenceScreen().removePreference(mLangInfoLink);

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
      Statistics.INSTANCE.trackEvent(Statistics.EventName.Settings.VOICE_LANGUAGE, Statistics.params().add(Statistics.EventParam.LANGUAGE, mSelectedLanguage));
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

    boolean enabled = TtsPlayer.INSTANCE.isEnabled();
    mPrefEnabled.setChecked(enabled);
    mPrefEnabled.setSummary(null);
    mPrefEnabled.setTitle(enabled ? R.string.on : R.string.off);
    mLangInfo.setSummary(enabled ? R.string.prefs_languages_information
                                 : R.string.prefs_languages_information_off);
    if (enabled)
      getPreferenceScreen().removePreference(mLangInfoLink);
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
    mPrefLanguages.setEnabled(available && TtsPlayer.INSTANCE.isEnabled());
    mPrefLanguages.setSummary(available ? mCurrentLanguage.name : null);
    mPrefLanguages.setValue(available ? mCurrentLanguage.internalCode : null);
    mPrefEnabled.setChecked(available && TtsPlayer.INSTANCE.isEnabled());

    enableListeners(true);
  }

  private boolean isOnTtsScreen()
  {
    return mPreferenceScreen.getKey() != null && mPreferenceScreen.getKey().equals(TTS_SCREEN_KEY);
  }

  @Override
  public Fragment getCallbackFragment()
  {
    return this;
  }

  @Override
  protected int getXmlResources()
  {
    return R.xml.prefs_main;
  }

  @Override
  public void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    mPreferenceScreen = getPreferenceScreen();
    mStoragePref = findPreference(getString(R.string.pref_storage));
    mPrefEnabled = (TwoStatePreference) findPreference(getString(R.string.pref_tts_enabled));
    mPrefLanguages = (ListPreference) findPreference(getString(R.string.pref_tts_language));
    mLangInfo = findPreference(getString(R.string.pref_tts_info));
    mLangInfoLink = findPreference(getString(R.string.pref_tts_info_link));
    initLangInfoLink();
    updateStoragePrefs();
    initStoragePrefCallbacks();
    initMeasureUnitsPrefsCallbacks();
    initZoomPrefsCallbacks();
    initMapStylePrefsCallbacks();
    initAutoDownloadPrefsCallbacks();
    initLargeFontSizePrefsCallbacks();
    init3dModePrefsCallbacks();
    initPerspectivePrefsCallbacks();
    initTrackRecordPrefsCallbacks();
    initStatisticsPrefsCallback();
    initPlayServicesPrefsCallbacks();
    initAutoZoomPrefsCallbacks();
    initSimplifiedTrafficColorsPrefsCallbacks();

    if (!MytargetHelper.isShowcaseSwitchedOnServer())
      getPreferenceScreen().removePreference(findPreference(getString(R.string.pref_showcase_switched_on)));

    initLoggingEnabledPrefsCallbacks();
    initUseMobileDataPrefsCallbacks();

    updateTts();
  }

  @Override
  public void onResume()
  {
    super.onResume();

    initTrackRecordPrefsCallbacks();
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
    if (preference.getKey() != null && preference.getKey().equals(getString(R.string.pref_help)))
    {
      Statistics.INSTANCE.trackEvent(Statistics.EventName.Settings.HELP);
      AlohaHelper.logClick(AlohaHelper.Settings.HELP);
    }
    else if (preference.getKey() != null && preference.getKey().equals(getString(R.string.pref_about)))
    {
      Statistics.INSTANCE.trackEvent(Statistics.EventName.Settings.ABOUT);
      AlohaHelper.logClick(AlohaHelper.Settings.ABOUT);
    }
    else if (preference.getKey() != null && preference.getKey().equals(getString(R.string.pref_osm_profile)))
    {
      Statistics.INSTANCE.trackEvent(Statistics.EventName.Settings.OSM_PROFILE);
      startActivity(new Intent(getActivity(), ProfileActivity.class));
    }
    return super.onPreferenceTreeClick(preference);
  }

  private void initLangInfoLink()
  {
    if (mLangInfoLink != null)
    {
      Spannable link = new SpannableString(getString(R.string.prefs_languages_information_off_link));
      link.setSpan(new ForegroundColorSpan(ContextCompat.getColor(getContext(),
                                                                  UiUtils.getStyledResourceId(getContext(), R.attr.colorAccent))),
                   0, link.length(), 0);
      mLangInfoLink.setSummary(link);
      mLangInfoLink.setOnPreferenceClickListener(new Preference.OnPreferenceClickListener()
      {
        @Override
        public boolean onPreferenceClick(Preference preference)
        {
          final Intent intent = new Intent(Intent.ACTION_VIEW);
          intent.setData(Uri.parse(TTS_INFO_LINK));
          getContext().startActivity(intent);
          return false;
        }
      });
      mPreferenceScreen.removePreference(mLangInfoLink);
    }
  }

  private void initSimplifiedTrafficColorsPrefsCallbacks()
  {
    final TwoStatePreference prefSimplifiedColors = (TwoStatePreference)findPreference(
        getString(R.string.pref_traffic_simplified_colors));
    if (prefSimplifiedColors == null)
      return;

    boolean simplifiedColorsEnabled = Framework.nativeGetSimplifiedTrafficColorsEnabled();
    prefSimplifiedColors.setChecked(simplifiedColorsEnabled);
    prefSimplifiedColors.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener()
    {
      @Override
      public boolean onPreferenceChange(Preference preference, Object newValue)
      {
        Framework.nativeSetSimplifiedTrafficColorsEnabled((Boolean)newValue);
        return true;
      }
    });
  }

  private void initLargeFontSizePrefsCallbacks()
  {
    Preference pref = findPreference(getString(R.string.pref_large_fonts_size));
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

  private void initUseMobileDataPrefsCallbacks()
  {
    final ListPreference mobilePref = (ListPreference)findPreference(
        getString(R.string.pref_use_mobile_data));
    if (mobilePref == null)
      return;

    int curValue = Config.getUseMobileDataSettings();

    if (curValue != NetworkPolicy.NOT_TODAY && curValue != NetworkPolicy.TODAY)
    {
      mobilePref.setValue(String.valueOf(curValue));
      mobilePref.setSummary(mobilePref.getEntry());
    }
    else
    {
      mobilePref.setSummary(getString(R.string.mobile_data_description));
    }
    mobilePref.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener()
    {
      @Override
      public boolean onPreferenceChange(Preference preference, Object newValue)
      {
        String valueStr = (String)newValue;
        switch (Integer.parseInt(valueStr))
        {
          case NetworkPolicy.ASK:
            Config.setUseMobileDataSettings(NetworkPolicy.ASK);
            break;
          case NetworkPolicy.ALWAYS:
            Config.setUseMobileDataSettings(NetworkPolicy.ALWAYS);
            break;
          case NetworkPolicy.NEVER:
            Config.setUseMobileDataSettings(NetworkPolicy.NEVER);
            break;
          default:
            throw new AssertionError("Wrong NetworkPolicy type!");
        }

        UiThread.runLater(new Runnable()
        {
          @Override
          public void run()
          {
            mobilePref.setSummary(mobilePref.getEntry());
          }
        });

        return true;
      }
    });
  }

  private void initLoggingEnabledPrefsCallbacks()
  {
    Preference pref = findPreference(getString(R.string.pref_enable_logging));
    if (pref == null)
      return;

    if (!MwmApplication.prefs().getBoolean(SearchFragment.PREFS_SHOW_ENABLE_LOGGING_SETTING,
                                           BuildConfig.BUILD_TYPE.equals("beta")))
    {
      getPreferenceScreen().removePreference(pref);
    }
    else
    {
      final boolean isLoggingEnabled = LoggerFactory.INSTANCE.isFileLoggingEnabled();
      ((TwoStatePreference) pref).setChecked(isLoggingEnabled);
      pref.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener()
      {
        @Override
        public boolean onPreferenceChange(Preference preference, Object newValue)
        {
          boolean oldVal = isLoggingEnabled;
          boolean newVal = (Boolean) newValue;
          if (oldVal != newVal)
          {
            LoggerFactory.INSTANCE.setFileLoggingEnabled(newVal);
          }
          return true;
        }
      });
    }
  }

  private void initAutoZoomPrefsCallbacks()
  {
    final TwoStatePreference pref = (TwoStatePreference)findPreference(getString(R.string.pref_auto_zoom));
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

  private void initPlayServicesPrefsCallbacks()
  {
    Preference pref = findPreference(getString(R.string.pref_play_services));
    if (pref == null)
      return;

    if (GoogleApiAvailability.getInstance().isGooglePlayServicesAvailable(MwmApplication.get()) != ConnectionResult.SUCCESS)
      getPreferenceScreen().removePreference(pref);
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
    }
  }

  private void initStatisticsPrefsCallback()
  {
    Preference pref = findPreference(getString(R.string.pref_send_statistics));
    if (pref == null)
      return;

    ((TwoStatePreference)pref).setChecked(Config.isStatisticsEnabled());
    pref.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener()
    {
      @Override
      public boolean onPreferenceChange(Preference preference, Object newValue)
      {
        Statistics.INSTANCE.setStatEnabled((Boolean) newValue);
        return true;
      }
    });
  }

  private void initTrackRecordPrefsCallbacks()
  {
    final ListPreference trackPref = (ListPreference)findPreference(getString(R.string.pref_track_record));
    final Preference pref = findPreference(getString(R.string.pref_track_record_time));
    final Preference root = findPreference(getString(R.string.pref_track_screen));
    if (trackPref == null || pref == null)
      return;

    boolean enabled = TrackRecorder.isEnabled();
    ((TwoStatePreference)pref).setChecked(enabled);
    trackPref.setEnabled(enabled);
    if (root != null)
      root.setSummary(enabled ? R.string.on : R.string.off);
    pref.setTitle(enabled ? R.string.on : R.string.off);
    pref.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener()
    {
      @Override
      public boolean onPreferenceChange(Preference preference, Object newValue)
      {
        boolean enabled = (Boolean) newValue;
        TrackRecorder.setEnabled(enabled);
        Statistics.INSTANCE.setStatEnabled(enabled);
        trackPref.setEnabled(enabled);
        if (root != null)
          root.setSummary(enabled ? R.string.on : R.string.off);
        pref.setTitle(enabled ? R.string.on : R.string.off);
        trackPref.performClick();
        return true;
      }
    });

    String value = (enabled ? String.valueOf(TrackRecorder.getDuration()) : "0");
    trackPref.setValue(value);
    trackPref.setSummary(trackPref.getEntry());
    trackPref.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener()
    {
      @Override
      public boolean onPreferenceChange(final Preference preference, Object newValue)
      {
        int value = Integer.valueOf((String)newValue);
        boolean enabled = value != 0;
        if (enabled)
          TrackRecorder.setDuration(value);
        TrackRecorder.setEnabled(enabled);
        ((TwoStatePreference) pref).setChecked(enabled);
        trackPref.setEnabled(enabled);
        if (root != null)
          root.setSummary(enabled ? R.string.on : R.string.off);
        pref.setTitle(enabled ? R.string.on : R.string.off);

        UiThread.runLater(new Runnable()
        {
          @Override
          public void run()
          {
            trackPref.setSummary(trackPref.getEntry());
          }
        });
        return true;
      }
    });
  }

  private void init3dModePrefsCallbacks()
  {
    final TwoStatePreference pref = (TwoStatePreference)findPreference(getString(R.string.pref_3d_buildings));
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
    final TwoStatePreference pref = (TwoStatePreference)findPreference(getString(R.string.pref_3d));
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
    TwoStatePreference pref = (TwoStatePreference)findPreference(getString(R.string.pref_autodownload));
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
    final ListPreference pref = (ListPreference)findPreference(getString(R.string.pref_map_style));
    if (pref == null)
      return;

    String curTheme = Config.getUiThemeSettings();
    pref.setValue(curTheme);
    pref.setSummary(pref.getEntry());
    pref.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener()
    {
      @Override
      public boolean onPreferenceChange(Preference preference, Object newValue)
      {
        String themeName = (String)newValue;
        if (!Config.setUiThemeSettings(themeName))
          return true;

        ThemeSwitcher.restart(false);
        Statistics.INSTANCE.trackEvent(Statistics.EventName.Settings.MAP_STYLE,
                                       Statistics.params().add(Statistics.EventParam.NAME, themeName));

        UiThread.runLater(new Runnable()
        {
          @Override
          public void run()
          {
            pref.setSummary(pref.getEntry());
          }
        });

        return true;
      }
    });
  }

  private void initZoomPrefsCallbacks()
  {
    Preference pref = findPreference(getString(R.string.pref_show_zoom_buttons));
    if (pref == null)
      return;

    ((TwoStatePreference)pref).setChecked(Config.showZoomButtons());
    pref.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener()
    {
      @Override
      public boolean onPreferenceChange(Preference preference, Object newValue)
      {
        Statistics.INSTANCE.trackEvent(Statistics.EventName.Settings.ZOOM);
        Config.setShowZoomButtons((Boolean) newValue);
        return true;
      }
    });
  }

  private void initMeasureUnitsPrefsCallbacks()
  {
    Preference pref = findPreference(getString(R.string.pref_munits));
    if (pref == null)
      return;

    ((ListPreference)pref).setValue(String.valueOf(UnitLocale.getUnits()));
    pref.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener()
    {
      @Override
      public boolean onPreferenceChange(Preference preference, Object newValue)
      {
        UnitLocale.setUnits(Integer.parseInt((String) newValue));
        Statistics.INSTANCE.trackEvent(Statistics.EventName.Settings.UNITS);
        AlohaHelper.logClick(AlohaHelper.Settings.CHANGE_UNITS);
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
          new AlertDialog.Builder(getActivity())
              .setTitle(getString(R.string.downloading_is_active))
              .setMessage(getString(R.string.cant_change_this_setting))
              .setPositiveButton(getString(R.string.ok), null)
              .show();
        else
//          getSettingsActivity().switchToFragment(StoragePathFragment.class, R.string.maps_storage);
          getSettingsActivity().replaceFragment(StoragePathFragment.class,
                                                getString(R.string.maps_storage), null);

        return true;
      }
    });
  }

  @Override
  public void onAttach(Context context)
  {
    super.onAttach(context);

    if (!(context instanceof Activity))
      return;

    mPathManager.startExternalStorageWatching((Activity) context, new StoragePathManager.OnStorageListChangedListener()
    {
      @Override
      public void onStorageListChanged(List<StorageItem> storageItems, int currentStorageIndex)
      {
        updateStoragePrefs();
      }
    }, null);
  }

  @Override
  public void onDetach()
  {
    super.onDetach();
    mPathManager.stopExternalStorageWatching();
  }
}
