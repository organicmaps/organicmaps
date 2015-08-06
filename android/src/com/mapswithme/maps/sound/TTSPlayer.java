package com.mapswithme.maps.sound;

import android.content.Context;
import android.speech.tts.TextToSpeech;
import android.util.Log;
import android.widget.Toast;

import com.mapswithme.maps.MWMApplication;

import java.util.Locale;


public class TTSPlayer
{
  private static TTSPlayer ourInstance = null;

  private Context mContext = null;
  private TextToSpeech mTts = null;

  private final static String TAG = "TTSPlayer";

  private TTSPlayer()
  {
    mContext = MWMApplication.get().getApplicationContext();
    setLocaleIfAvailable(Locale.getDefault());
  }

  @Override
  protected void finalize() throws Throwable
  {
    if(mTts != null)
      mTts.shutdown();
    super.finalize();
  }

  public static TTSPlayer get()
  {
    if (ourInstance == null || !ourInstance.isLocaleEquals(Locale.getDefault()))
      ourInstance = new TTSPlayer();

    return ourInstance;
  }

  private boolean isLocaleEquals(Locale locale)
  {
    return locale.equals(mTts.getLanguage());
  }

  private void setLocaleIfAvailable(final Locale locale)
  {
    if (mTts != null && mTts.getLanguage().equals(locale))
      return;

    // @TODO Consider move TextToSpeech to a service:
    // http://stackoverflow.com/questions/24712639/android-texttospeech-initialization-blocks-freezes-ui-thread
    mTts = new TextToSpeech(mContext, new TextToSpeech.OnInitListener()
    {
      @Override
      public void onInit(int status)
      {
        if (status == TextToSpeech.ERROR)
        {
          Log.w(TAG, "Can't initialize TextToSpeech for locale " + locale.getLanguage() + " " + locale.getCountry());
          return;
        }

        final int avail = mTts.isLanguageAvailable(locale);
        Locale loc = locale;
        if (avail != TextToSpeech.LANG_AVAILABLE && avail != TextToSpeech.LANG_COUNTRY_AVAILABLE
                && avail != TextToSpeech.LANG_COUNTRY_VAR_AVAILABLE)
        {
          loc = Locale.UK; // No translation for TTS for Locale.getDefault() language.
        }

        mTts.setLanguage(locale);
        nativeSetTurnNotificationsLocale(locale.getLanguage());
        Log.i(TAG, "setLocaleIfAvailable() nativeSetTurnNotificationsLocale(" + locale.getLanguage() + ")");
      }
    });
  }

  public void speak(String textToSpeak)
  {
    if (mTts == null)
    {
      Log.w(TAG, "TTSPlayer.speak() is called while mTts == null.");
      return;
    }

    Toast.makeText(mContext, textToSpeak, Toast.LENGTH_SHORT).show();
    mTts.speak(textToSpeak, TextToSpeech.QUEUE_ADD, null);
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

  public native static void nativeEnableTurnNotifications(boolean enable);
  public native static boolean nativeAreTurnNotificationsEnabled();
  public native static void nativeSetTurnNotificationsLocale(String locale);
  public native static String nativeGetTurnNotificationsLocale();
}
