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
  private Locale mTtsLocale; // TTS locale. If mTtsLocale == null than mTts cannot be used.
  private final Locale mDefaultTtsLocale = Locale.US;
  private boolean mIsLocaleChanging = false;

  private final static String TAG = "TtsPlayer";

  TtsPlayer()
  {
    mContext = MwmApplication.get().getApplicationContext();
  }

  public void init()
  {
    Locale systemLanguage = Locale.getDefault();
    if (systemLanguage == null)
      systemLanguage = mDefaultTtsLocale;

    if (INSTANCE.mTtsLocale == null || !INSTANCE.isLocaleEqual(systemLanguage))
      INSTANCE.setLocaleIfAvailable(systemLanguage);
  }

  private boolean isLocaleEqual(Locale locale)
  {
    return locale.getLanguage().equals(mTtsLocale.getLanguage()) &&
        locale.getCountry().equals(mTtsLocale.getCountry());
  }

  private boolean isLocaleAvailable(Locale locale)
  {
    final int avail = mTts.isLanguageAvailable(locale);
    return avail == TextToSpeech.LANG_AVAILABLE || avail == TextToSpeech.LANG_COUNTRY_AVAILABLE
        || avail == TextToSpeech.LANG_COUNTRY_VAR_AVAILABLE;
  }

  private void setLocaleIfAvailable(final Locale locale)
  {
    if (mTts != null && mTtsLocale != null && mTtsLocale.equals(locale))
      return;

    mIsLocaleChanging = true;

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

        if (isLocaleAvailable(locale))
        {
          Log.i(TAG, "The locale " + locale.getLanguage() + " " + locale.getCountry() + " will be used for TTS.");
          mTtsLocale = locale;
        }
        else if (isLocaleAvailable(mDefaultTtsLocale))
        {
          Log.w(TAG, "TTS is not available for locale " + locale.getLanguage() + " " + locale.getCountry() +
              ". The default locale " + mDefaultTtsLocale.getLanguage() + " " + mDefaultTtsLocale.getCountry() + " will be used.");
          mTtsLocale = mDefaultTtsLocale;
        }
        else
        {
          Log.w(TAG, "TTS is not available for locale " + locale.getLanguage() + " " + locale.getCountry() +
              " and for the default locale " +  mDefaultTtsLocale.getLanguage() + " " + mDefaultTtsLocale.getCountry() +
              ". TTS will be switched off.");
          mTtsLocale = null;
          mIsLocaleChanging = false;
          return;
        }

        mTts.setLanguage(mTtsLocale);
        // @TODO(vbykoianko) In case of mTtsLocale.getLanguage() returns zh. But the core is needed zh-Hant or zh-Hans.
        // It should be fixed.
        nativeSetTurnNotificationsLocale(mTtsLocale.getLanguage());
        Log.i(TAG, "setLocaleIfAvailable() onInit nativeSetTurnNotificationsLocale(" + mTtsLocale.getLanguage() + ")");
        mIsLocaleChanging = false;
      }
    });
  }

  private boolean readyToPlay()
  {
    return !mIsLocaleChanging && mTts != null && mTtsLocale != null;
  }

  private void speak(String textToSpeak)
  {
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
    if (!readyToPlay())
      return; // speakNotifications() is called while TTS is not ready or could not be initialized.

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
