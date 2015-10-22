package com.mapswithme.maps.sound;

import android.content.Context;
import android.content.res.Resources;
import android.speech.tts.TextToSpeech;
import android.support.annotation.Nullable;
import android.text.TextUtils;
import android.util.Log;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.util.Config;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;


public enum TtsPlayer
{
  INSTANCE;

  private static final String TAG = "TtsPlayer";
  private static final Locale DEFAULT_LOCALE = Locale.US;
  private static final float SPEECH_RATE = 1.2f;

  private TextToSpeech mTts;
  private boolean mInitializing;

  // TTS is locked down due to lack of supported languages
  private boolean mUnavailable;

  TtsPlayer() {}

  private @Nullable LanguageData findSupportedLanguage(String internalCode, List<LanguageData> langs)
  {
    if (TextUtils.isEmpty(internalCode))
      return null;

    for (LanguageData lang : langs)
      if (lang.matchesInternalCode(internalCode))
        return lang;

    return null;
  }

  private @Nullable LanguageData findSupportedLanguage(Locale locale, List<LanguageData> langs)
  {
    if (locale == null)
      return null;

    for (LanguageData lang : langs)
      if (lang.matchesLocale(locale))
        return lang;

    return null;
  }

  private void setLanguageInternal(LanguageData lang)
  {
    mTts.setLanguage(lang.locale);
    nativeSetTurnNotificationsLocale(lang.internalCode);
    Config.setTtsLanguage(lang.internalCode);
  }

  public void setLanguage(LanguageData lang)
  {
    if (lang != null)
      setLanguageInternal(lang);
  }

  private LanguageData getDefaultLanguage(List<LanguageData> langs)
  {
    Locale defLocale = Locale.getDefault();
    if (defLocale == null)
      defLocale = DEFAULT_LOCALE;

    LanguageData lang = findSupportedLanguage(defLocale, langs);
    if (lang == null)
    {
      // No supported languages found, lock down TTS :(
      mUnavailable = true;
      Config.setTtsEnabled(false);
    }

    return lang;
  }

  public LanguageData getSelectedLanguage(List<LanguageData> langs)
  {
    return findSupportedLanguage(Config.getTtsLanguage(), langs);
  }

  public void init(Context context)
  {
    if (mTts != null || mInitializing || mUnavailable)
      return;

    mInitializing = true;
    mTts = new TextToSpeech(context, new TextToSpeech.OnInitListener()
    {
      @Override
      public void onInit(int status)
      {
        if (status == TextToSpeech.ERROR)
        {
          Log.e(TAG, "Failed to initialize TextToSpeach");
          mUnavailable = true;
          mInitializing = false;
          return;
        }

        List<LanguageData> langs = getAvailableLanguages(false);
        LanguageData defLang = getDefaultLanguage(langs);
        LanguageData lang = getSelectedLanguage(langs);
        if (lang == null)
          lang = defLang;

        if (lang != null)
        {
          String curLangCode = Config.getTtsLanguage();
          if (defLang != null && !defLang.matchesInternalCode(curLangCode))
          {
            // Selected TTS locale does not match current defaults. And it was NOT set by user.
            // Assume that the current locale was equal to old default one.
            // So, let the new default locale be current.
            if (!Config.isTtsLanguageSetByUser())
              lang = findSupportedLanguage(defLang.internalCode, langs);
          }

          setLanguage(lang);
        }

        mTts.setSpeechRate(SPEECH_RATE);
        setEnabled(Config.isTtsEnabled());
        mInitializing = false;
      }
    });
  }

  public boolean isReady()
  {
    return (mTts != null && !mUnavailable && !mInitializing);
  }

  private void speak(String textToSpeak)
  {
    //noinspection deprecation
    mTts.speak(textToSpeak, TextToSpeech.QUEUE_ADD, null);
  }

  public void playTurnNotifications()
  {
    // It's necessary to call Framework.nativeGenerateTurnNotifications() even if TtsPlayer is invalid.
    final String[] turnNotifications = Framework.nativeGenerateTurnNotifications();

    if (turnNotifications != null && isReady())
      for (String textToSpeak : turnNotifications)
        speak(textToSpeak);
  }

  public void stop()
  {
    if (isReady())
      mTts.stop();
  }

  public boolean isEnabled()
  {
    return nativeAreTurnNotificationsEnabled();
  }

  public void setEnabled(boolean enabled)
  {
    nativeEnableTurnNotifications(enabled);
  }

  public List<LanguageData> getAvailableLanguages(boolean includeDownloadable)
  {
    List<LanguageData> res = new ArrayList<>();
    if (mUnavailable || mTts == null)
      return res;

    Resources resources = MwmApplication.get().getResources();
    String[] codes = resources.getStringArray(R.array.tts_languages_supported);
    String[] names = resources.getStringArray(R.array.tts_language_names);

    for (int i = 0; i < codes.length; i++)
    {
      LanguageData lang = LanguageData.parse(codes[i], names[i]);
      int status = mTts.isLanguageAvailable(lang.locale);

      int requiredStatus = (includeDownloadable ? TextToSpeech.LANG_MISSING_DATA
                                                : TextToSpeech.LANG_AVAILABLE);
      if (status >= requiredStatus)
      {
        lang.setStatus(status);
        res.add(lang);
      }
    }

    return res;
  }

  private native static void nativeEnableTurnNotifications(boolean enable);
  private native static boolean nativeAreTurnNotificationsEnabled();
  private native static void nativeSetTurnNotificationsLocale(String code);
  private native static String nativeGetTurnNotificationsLocale();
}
