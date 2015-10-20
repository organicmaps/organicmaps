package com.mapswithme.maps.sound;

import android.speech.tts.TextToSpeech;
import android.text.TextUtils;
import android.util.Log;
import android.widget.Toast;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.util.Language;

import java.util.Locale;


public enum TtsPlayer
{
  INSTANCE;

  private static final Locale DEFAULT_LOCALE = Locale.US;
  private static final float SPEECH_RATE = 1.2f;

  // The both mTtts and mTtsLocale should be initialized before usage.
  private TextToSpeech mTts;
  private Locale mTtsLocale;
  private boolean mIsLocaleChanging;

  private final static String TAG = "TtsPlayer";

  TtsPlayer() {}

  public void reinitIfLocaleChanged()
  {
    if (!isValid())
      return; // TtsPlayer was not inited yet.

    final Locale locale = getDefaultLocale();
    if (!isLocaleEqual(locale))
      initTts(locale);
  }

  private Locale getDefaultLocale()
  {
    final Locale locale = Locale.getDefault();
    return locale == null ? DEFAULT_LOCALE : locale;
  }

  private boolean isLocaleEqual(Locale locale)
  {
    return locale.getLanguage().equals(mTtsLocale.getLanguage()) &&
        locale.getCountry().equals(mTtsLocale.getCountry());
  }

  private boolean isLocaleAvailable(Locale locale)
  {
    final int avail = mTts.isLanguageAvailable(locale);
    return avail == TextToSpeech.LANG_AVAILABLE || avail == TextToSpeech.LANG_COUNTRY_AVAILABLE ||
        avail == TextToSpeech.LANG_COUNTRY_VAR_AVAILABLE;
  }

  private void initTts(final Locale locale)
  {
    if (mIsLocaleChanging)
      return; // Preventing reiniting while creating TextToSpeech object. There's a small possibility a new locale is skipped.

    if (mTts != null && mTtsLocale != null && mTtsLocale.equals(locale))
      return;

    mTtsLocale = null;
    mIsLocaleChanging = true;

    if (mTts != null)
    {
      mTts.stop();
      mTts.shutdown();
    }

    mTts = new TextToSpeech(MwmApplication.get(), new TextToSpeech.OnInitListener()
    {
      @Override
      public void onInit(int status)
      {
        // This method is called asynchronously.
        mIsLocaleChanging = false;
        if (status == TextToSpeech.ERROR)
        {
          Log.w(TAG, "Can't initialize TextToSpeech for locale " + locale.getLanguage() + " " + locale.getCountry());
          return;
        }

        if (isLocaleAvailable(locale))
        {
          Log.i(TAG, "The locale " + locale.getLanguage() + " " + locale.getCountry() + " will be used for TTS.");
          mTtsLocale = locale;
        }
        else if (isLocaleAvailable(DEFAULT_LOCALE))
        {
          Log.w(TAG, "TTS is not available for locale " + locale.getLanguage() + " " + locale.getCountry() +
              ". The default locale " + DEFAULT_LOCALE.getLanguage() + " " + DEFAULT_LOCALE.getCountry() + " will be used.");
          mTtsLocale = DEFAULT_LOCALE;
        }
        else
        {
          Log.w(TAG, "TTS is not available for locale " + locale.getLanguage() + " " + locale.getCountry() +
              " and for the default locale " +  DEFAULT_LOCALE.getLanguage() + " " + DEFAULT_LOCALE.getCountry() +
              ". TTS will be switched off.");
          mTtsLocale = null;
          return;
        }

        String localeTwine = Language.localeToTwineLanguage(mTtsLocale);
        if (TextUtils.isEmpty(localeTwine))
        {
          Log.w(TAG, "Cann't get a twine language name for the locale " + locale.getLanguage() + " " + locale.getCountry() +
              ". TTS will be switched off.");
          mTtsLocale = null;
          return;
        }

        mTts.setSpeechRate(SPEECH_RATE);
        nativeSetTurnNotificationsLocale(localeTwine);
        mTts.setLanguage(mTtsLocale);
        Log.i(TAG, "setLocaleIfAvailable() onInit nativeSetTurnNotificationsLocale(" + localeTwine + ")");
      }
    });
  }

  private boolean isValid()
  {
    return !mIsLocaleChanging && mTts != null && mTtsLocale != null;
  }

  private void speak(String textToSpeak)
  {
    // @TODO(vbykoianko) removes these two toasts below when the test period is finished.
    Toast.makeText(MwmApplication.get(), textToSpeak, Toast.LENGTH_SHORT).show();
    if (mTts.speak(textToSpeak, TextToSpeech.QUEUE_ADD, null) == TextToSpeech.ERROR)
    {
      Log.e(TAG, "TextToSpeech returns TextToSpeech.ERROR.");
      Toast.makeText(MwmApplication.get(), "TTS error", Toast.LENGTH_SHORT).show();
    }
  }

  public void playTurnNotifications()
  {
    final String[] turnNotifications = Framework.nativeGenerateTurnSound();
    
    if (turnNotifications != null && isValid())
      for (String textToSpeak : turnNotifications)
        speak(textToSpeak);
  }

  public void stop()
  {
    if(mTts != null)
      mTts.stop();
  }

  public boolean isEnabled()
  {
    return nativeAreTurnNotificationsEnabled();
  }

  // Note. After a call of enable(true) the flag enabled in cpp core will be set.
  // But later in onInit callback the initialization could fail.
  // In that case isValid() returns false and every call of playTurnNotifications returns in the beginning.
  public void enable(boolean enabled)
  {
    if (enabled && !isValid())
      initTts(getDefaultLocale());
    nativeEnableTurnNotifications(enabled);
  }

  private native static void nativeEnableTurnNotifications(boolean enable);
  private native static boolean nativeAreTurnNotificationsEnabled();
  private native static void nativeSetTurnNotificationsLocale(String locale);
  private native static String nativeGetTurnNotificationsLocale();
}
