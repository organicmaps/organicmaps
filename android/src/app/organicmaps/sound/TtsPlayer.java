package app.organicmaps.sound;

import android.content.Context;
import android.content.res.Resources;
import android.os.Handler;
import android.os.Looper;
import android.speech.tts.TextToSpeech;
import android.speech.tts.UtteranceProgressListener;
import android.text.TextUtils;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.Framework;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.base.Initializable;
import app.organicmaps.base.MediaPlayerWrapper;
import app.organicmaps.util.Config;
import app.organicmaps.util.log.Logger;

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
public enum TtsPlayer implements Initializable<Context>
{
  INSTANCE;

  private static final String TAG = TtsPlayer.class.getSimpleName();
  private static final Locale DEFAULT_LOCALE = Locale.US;
  private static final float SPEECH_RATE = 1.2f;
  private static final int TTS_SPEAK_DELAY_MILLIS = 50;

  private TextToSpeech mTts;
  private boolean mInitializing;
  private AudioFocusManager mAudioFocusManager;

  private final Handler delayHandler = new Handler(Looper.getMainLooper());

  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private Context mContext;

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

  private boolean setLanguageInternal(LanguageData lang)
  {
    try
    {
      mTts.setLanguage(lang.locale);
      nativeSetTurnNotificationsLocale(lang.internalCode);
      Config.setTtsLanguage(lang.internalCode);

      return true;
    }
    catch (IllegalArgumentException e)
    {
      lockDown();
      return false;
    }
  }

  public boolean setLanguage(LanguageData lang)
  {
    return (lang != null && setLanguageInternal(lang));
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

  @Override
  public void initialize(@Nullable Context context)
  {
    mContext = context;

    if (mTts != null || mInitializing || mUnavailable)
      return;

    mInitializing = true;
    mTts = new TextToSpeech(context, status -> {
      if (status == TextToSpeech.ERROR)
      {
        Logger.e(TAG, "Failed to initialize TextToSpeach");
        lockDown();
        mInitializing = false;
        return;
      }
      refreshLanguages();
      mTts.setSpeechRate(SPEECH_RATE);
      mTts.setOnUtteranceProgressListener(new UtteranceProgressListener() {
        @Override
        public void onStart(String utteranceId) {
          mAudioFocusManager.requestAudioFocus();
        }

        @Override
        public void onDone(String utteranceId) {
          mAudioFocusManager.releaseAudioFocus();
        }

        @Override
        public void onError(String utteranceId) {
          mAudioFocusManager.releaseAudioFocus();
        }
      });
      mAudioFocusManager = new AudioFocusManager(context);
      mInitializing = false;
    });
  }

  @Override
  public void destroy()
  {
    // No op.
  }

  public boolean isSpeaking()
  {
    return mTts != null && mTts.isSpeaking();
  }

  private static boolean isReady()
  {
    return (INSTANCE.mTts != null && !INSTANCE.mUnavailable && !INSTANCE.mInitializing);
  }

  private void speak(String textToSpeak)
  {
    if (Config.isTtsEnabled())
      try
      {
        boolean isMusicActive = mAudioFocusManager.requestAudioFocus();
        if (isMusicActive)
          delayHandler.postDelayed(() -> mTts.speak(textToSpeak, TextToSpeech.QUEUE_ADD, null, textToSpeak), TTS_SPEAK_DELAY_MILLIS);
        else
          mTts.speak(textToSpeak, TextToSpeech.QUEUE_ADD, null, textToSpeak);
      }
      catch (IllegalArgumentException e)
      {
        lockDown();
      }
  }

  public void playTurnNotifications(@NonNull Context context)
  {
    if (MediaPlayerWrapper.from(context).isPlaying())
      return;
    // It's necessary to call Framework.nativeGenerateTurnNotifications() even if TtsPlayer is invalid.
    final String[] turnNotifications = Framework.nativeGenerateNotifications();

    if (turnNotifications != null && isReady())
      for (String textToSpeak : turnNotifications)
        speak(textToSpeak);
  }

  public void playTurnNotifications(@NonNull Context context, @NonNull String[] turnNotifications)
  {
    if (MediaPlayerWrapper.from(context).isPlaying())
      return;

    if (isReady())
      for (String textToSpeak : turnNotifications)
        speak(textToSpeak);
  }

  public void stop()
  {
    if (isReady())
      try
      {
        mAudioFocusManager.releaseAudioFocus();
        mTts.stop();
      }
      catch (IllegalArgumentException e)
      {
        lockDown();
      }
  }

  public static boolean isEnabled()
  {
    return (isReady() && nativeAreTurnNotificationsEnabled());
  }

  public static void setEnabled(boolean enabled)
  {
    Config.setTtsEnabled(enabled);
    nativeEnableTurnNotifications(enabled);
  }

  private boolean getUsableLanguages(List<LanguageData> outList)
  {
    Resources resources = MwmApplication.from(mContext).getResources();
    String[] codes = resources.getStringArray(R.array.tts_languages_supported);
    String[] names = resources.getStringArray(R.array.tts_language_names);

    for (int i = 0; i < codes.length; i++)
    {
      try
      {
        outList.add(new LanguageData(codes[i], names[i], mTts));
      }
      catch (LanguageData.NotAvailableException ignored) {
        Logger.w(TAG, "Failed to get usable languages " + ignored.getMessage());
      }
      catch (IllegalArgumentException e)
      {
        Logger.e(TAG, "Failed to get usable languages", e);
        lockDown();
        return false;
      }
    }

    return true;
  }

  private @Nullable LanguageData refreshLanguagesInternal(List<LanguageData> outList)
  {
    if (!getUsableLanguages(outList))
      return null;

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
