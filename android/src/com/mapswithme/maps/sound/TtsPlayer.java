package com.mapswithme.maps.sound;

import android.content.Context;
import android.content.res.Resources;
import android.speech.tts.TextToSpeech;
import android.support.annotation.NonNull;
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

/**
 * {@code TtsPlayer} class manages available TTS voice languages.
 * Single TTS language is described by {@link LanguageData} item.
 * <p>
 * We support a set of languages listed in {@code strings-tts.xml} file.
 * During loading each item in this list is marked as {@code downloaded} or {@code not downloaded},
 * unsupported voices are excluded.
 * <p>
 * At startup we check whether currently selected language is in our list of supported voices and its data is downloaded.
 * If not, we check system default locale. If failed, the same check is made for English language.
 * Finally, if mentioned checks fail we manually disable TTS, so the user must go to the settings and select
 * preferred voice language by hand.
 * <p>
 * If no core supported languages can be used by the system, TTS is locked down and can not be enabled and used.
 */
public enum TtsPlayer
{
  INSTANCE;

  private static final Locale DEFAULT_LOCALE = Locale.US;
  private static final float SPEECH_RATE = 1.2f;

  private TextToSpeech mTts;
  private boolean mInitializing;

  // TTS is locked down due to absence of supported languages
  private boolean mUnavailable;

  TtsPlayer() {}

  private static @Nullable LanguageData findSupportedLanguage(String internalCode, List<LanguageData> langs)
  {
    if (TextUtils.isEmpty(internalCode))
      return null;

    for (LanguageData lang : langs)
      if (lang.matchesInternalCode(internalCode))
        return lang;

    return null;
  }

  private static @Nullable LanguageData findSupportedLanguage(Locale locale, List<LanguageData> langs)
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

  private static @Nullable LanguageData getDefaultLanguage(List<LanguageData> langs)
  {
    LanguageData res;

    Locale defLocale = Locale.getDefault();
    if (defLocale != null)
    {
      res = findSupportedLanguage(defLocale, langs);
      if (res != null && res.downloaded)
        return res;
    }

    res = findSupportedLanguage(DEFAULT_LOCALE, langs);
    if (res != null && res.downloaded)
      return res;

    return null;
  }

  public static @Nullable LanguageData getSelectedLanguage(List<LanguageData> langs)
  {
    return findSupportedLanguage(Config.getTtsLanguage(), langs);
  }

  private void lockDown()
  {
    mUnavailable = true;
    setEnabled(false);
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
          Log.e("TtsPlayer", "Failed to initialize TextToSpeach");
          lockDown();
          mInitializing = false;
          return;
        }

        refreshLanguages();
        mTts.setSpeechRate(SPEECH_RATE);
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
    if (Config.isTtsEnabled())
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
    return (isReady() && nativeAreTurnNotificationsEnabled());
  }

  public static void setEnabled(boolean enabled)
  {
    Config.setTtsEnabled(enabled);
    nativeEnableTurnNotifications(enabled);
  }

  private void getUsableLanguages(List<LanguageData> outList)
  {
    Resources resources = MwmApplication.get().getResources();
    String[] codes = resources.getStringArray(R.array.tts_languages_supported);
    String[] names = resources.getStringArray(R.array.tts_language_names);

    for (int i = 0; i < codes.length; i++)
    {
      try
      {
        outList.add(new LanguageData(codes[i], names[i], mTts));
      } catch (LanguageData.NotAvailableException ignored)
      {}
    }
  }

  private @Nullable LanguageData refreshLanguagesInternal(List<LanguageData> outList)
  {
    getUsableLanguages(outList);

    if (outList.isEmpty())
    {
      // No supported languages found, lock down TTS :(
      lockDown();
      return null;
    }

    LanguageData res = getSelectedLanguage(outList);
    if (res == null || !res.downloaded)
      // Selected locale is not available or not downloaded
      res = getDefaultLanguage(outList);

    if (res == null || !res.downloaded)
    {
      // Default locale can not be used too
      Config.setTtsEnabled(false);
      return null;
    }

    return res;
  }

  public @NonNull List<LanguageData> refreshLanguages()
  {
    List<LanguageData> res = new ArrayList<>();
    if (mUnavailable || mTts == null)
      return res;

    LanguageData lang = refreshLanguagesInternal(res);
    setLanguage(lang);

    setEnabled(Config.isTtsEnabled());
    return res;
  }

  private native static void nativeEnableTurnNotifications(boolean enable);
  private native static boolean nativeAreTurnNotificationsEnabled();
  private native static void nativeSetTurnNotificationsLocale(String code);
  private native static String nativeGetTurnNotificationsLocale();
}
