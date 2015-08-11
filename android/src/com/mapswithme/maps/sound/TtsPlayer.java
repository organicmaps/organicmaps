package com.mapswithme.maps.sound;

import android.content.Context;
import android.speech.tts.TextToSpeech;
import android.util.Log;
import android.widget.Toast;

import com.mapswithme.maps.MwmApplication;

import java.util.Locale;


public enum TtsPlayer
{
  INSTANCE;

  private Context mContext;
  private TextToSpeech mTts;
  private Locale mTtsLocale;

  private final static String TAG = "TtsPlayer";

  TtsPlayer()
  {
    mContext = MwmApplication.get().getApplicationContext();
  }

  public void init()
  {
    Locale systemLanguage = Locale.getDefault();
    if (INSTANCE.mTtsLocale == null || !INSTANCE.isLocaleEqual(systemLanguage))
      INSTANCE.setLocaleIfAvailable(systemLanguage);
  }

  private boolean isLocaleEqual(Locale locale)
  {
    return locale.getLanguage().equals(mTtsLocale.getLanguage()) &&
        locale.getCountry().equals(mTtsLocale.getCountry());
  }

  private void setLocaleIfAvailable(final Locale locale)
  {
    if (mTts != null && mTtsLocale.equals(locale))
      return;

    if (mTts != null)
    {
      mTts.stop();
      mTts.shutdown();
    }

    mTts = new TextToSpeech(mContext, new TextToSpeech.OnInitListener()
    {
      @Override
      public void onInit(int status)
      {
        // This method is called anisochronously.
        if (status == TextToSpeech.ERROR)
        {
          Log.w(TAG, "Can't initialize TextToSpeech for locale " + locale.getLanguage() + " " + locale.getCountry());
          return;
        }

        final int avail = mTts.isLanguageAvailable(locale);
        mTtsLocale = locale;
        if (avail != TextToSpeech.LANG_AVAILABLE && avail != TextToSpeech.LANG_COUNTRY_AVAILABLE
                && avail != TextToSpeech.LANG_COUNTRY_VAR_AVAILABLE)
        {
          mTtsLocale = Locale.UK; // No translation for TTS for Locale.getDefault() language.
        }

        mTts.setLanguage(mTtsLocale);
        nativeSetTurnNotificationsLocale(mTtsLocale.getLanguage());
        Log.i(TAG, "setLocaleIfAvailable() nativeSetTurnNotificationsLocale(" + mTtsLocale.getLanguage() + ")");
      }
    });
  }

  private void speak(String textToSpeak)
  {
    if (mTts == null)
    {
      Log.w(TAG, "TtsPlayer.speak() is called while mTts == null.");
      return;
    }
    // @TODO(vbykoianko) removes these two toasts below when the test period is finished.
    Toast.makeText(mContext, textToSpeak, Toast.LENGTH_SHORT).show();
    if (mTts.speak(textToSpeak, TextToSpeech.QUEUE_ADD, null) == TextToSpeech.ERROR)
    {
      Log.e(TAG, "TextToSpeech returns TextToSpeech.ERROR.");
      Toast.makeText(mContext, "TTS error", Toast.LENGTH_SHORT).show();
    }
  }

  public void speakNotifications(String[] turnNotifications)
  {
    if (turnNotifications == null)
      return;

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

  public void enable(boolean enabled)
  {
    nativeEnableTurnNotifications(enabled);
  }

  private native static void nativeEnableTurnNotifications(boolean enable);
  private native static boolean nativeAreTurnNotificationsEnabled();
  private native static void nativeSetTurnNotificationsLocale(String locale);
  private native static String nativeGetTurnNotificationsLocale();
}
