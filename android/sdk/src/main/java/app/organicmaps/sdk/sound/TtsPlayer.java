package app.organicmaps.sdk.sound;

import android.content.Context;
import android.database.ContentObserver;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.provider.Settings;
import android.speech.tts.TextToSpeech;
import android.speech.tts.UtteranceProgressListener;
import android.text.TextUtils;
import android.util.Pair;
import androidx.annotation.AnyThread;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.VisibleForTesting;
import app.organicmaps.sdk.util.Assert;
import app.organicmaps.sdk.util.Config;
import app.organicmaps.sdk.util.concurrency.ThreadPool;
import app.organicmaps.sdk.util.concurrency.UiThread;
import app.organicmaps.sdk.util.log.Logger;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.Executor;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * {@code TtsPlayer} class manages available TTS voice languages.
 * Single TTS language is described by {@link LanguageData} item.
 * <p>
 * We support a set of languages listed in {@code libs/platform/languages.hpp} and passed to Java by
 * {@link #nativeGetSupportedLanguages()}.
 * During loading each item in this list is marked as {@code downloaded} or {@code not downloaded},
 * unsupported voices are excluded.
 * <p>
 * At startup we check whether currently selected language is in our list of supported voices and its data is
 * downloaded. If not, we check system default locale. If failed, the same check is made for English language. Finally,
 * if mentioned checks fail we manually disable TTS, so the user must go to the settings and select preferred voice
 * language by hand. <p> If no core supported languages can be used by the system, TTS is locked down and can not be
 * enabled and used.
 */
public enum TtsPlayer
{
  INSTANCE;

  private static final String TAG = TtsPlayer.class.getSimpleName();
  private static final Locale DEFAULT_LOCALE = Locale.US;
  private static final float SPEECH_RATE = 1.0f;
  private static final int TTS_SPEAK_DELAY_MILLIS = 50;
  private static final String TTS_SILENT_UTTERANCE_ID = "SILENT_DELAY";

  @Nullable
  private static List<Pair<String, String>> sSupportedLanguages = null;

  public static Runnable sOnReloadCallback = null;

  private static final CopyOnWriteArrayList<Runnable> sStateChangedListeners = new CopyOnWriteArrayList<>();

  // Checking every supported language makes ~60 synchronous calls into the engine's process, ~0.1 s in total,
  // so the check at startup runs here instead of on the UI thread.
  @VisibleForTesting
  static Executor sLanguageCheckExecutor = ThreadPool.getWorker();

  private ContentObserver mTtsEngineObserver;
  private TextToSpeech mTts;
  private final AtomicInteger mTtsQueueSize = new AtomicInteger(0);
  private final UtteranceProgressListener mUtteranceProgressListener = new UtteranceProgressListener() {
    @Override
    public void onStart(@NonNull String utteranceId)
    {
      Logger.d(TAG, "TTS Utterance started: " + utteranceId);
    }

    @Override
    public void onDone(@NonNull String utteranceId)
    {
      handleStop(utteranceId);
    }

    @Override
    @SuppressWarnings("deprecated") // abstract method must be implemented
    public void onError(@NonNull String utteranceId)
    {
      handleError(utteranceId, -1);
    }

    @Override
    public void onError(@NonNull String utteranceId, int errorCode)
    {
      handleError(utteranceId, errorCode);
    }

    private void handleError(@NonNull String utteranceId, int errorCode)
    {
      Logger.w(TAG, "TTS Utterance error: " + utteranceId + ", code: " + errorCode);
      handleStop(utteranceId);
    }

    private void handleStop(@NonNull String utteranceId)
    {
      Logger.d(TAG, "TTS Utterance stopped: " + utteranceId);
      if (mTtsQueueSize.decrementAndGet() <= 0)
        mAudioFocusManager.releaseAudioFocus();
      if (mTtsQueueSize.get() < 0)
        mTtsQueueSize.set(0);
    }
  };

  private boolean mInitializing;
  // Bumped on every initialize() start; the captured value lets the init callback
  // detect that it belongs to a TextToSpeech we've already shutdown and bail out.
  private int mInitGeneration;
  private boolean mReloadTriggered = false;
  private AudioFocusManager mAudioFocusManager;

  private final Bundle mParams = new Bundle();

  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private Context mContext;

  // Lockdown reasons: init ERROR, no OM-supported languages, engine IllegalArgumentException.
  // Reset on ContentObserver.onChange so a switched engine can re-initialize.
  private boolean mUnavailable;
  // Engine is ready and a downloaded, OM-supported voice language is selected.
  // Reflects the result of the latest refreshLanguages() pass.
  private boolean mHasUsableLanguage;

  public enum State
  {
    INITIALIZING,
    UNAVAILABLE,
    NEEDS_LANGUAGE,
    READY_ON,
    READY_OFF;

    public boolean isReady()
    {
      return this == READY_ON || this == READY_OFF;
    }
  }

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

  private boolean setLanguageInternal(@NonNull LanguageData lang)
  {
    mTts.setLanguage(lang.locale);
    nativeSetTurnNotificationsLocale(lang.internalCode);
    Config.TTS.setLanguage(lang.internalCode);

    return true;
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
    return findSupportedLanguage(Config.TTS.getLanguage(), langs);
  }

  private void lockDown()
  {
    mUnavailable = true;
    setEnabled(false);
  }

  public void initialize(@NonNull Context context)
  {
    mContext = context;

    if (mTts != null || mInitializing || mUnavailable)
      return;

    mInitializing = true;
    final int generation = ++mInitGeneration;
    // TextToSpeech.OnInitListener() can be called from a non-main thread
    // on LineageOS '20.0-20231127-RELEASE-thyme' 'Xiaomi/thyme/thyme'.
    // https://github.com/organicmaps/organicmaps/issues/6903
    mTts = new TextToSpeech(context, status -> UiThread.run(() -> {
      // Stale callback from a TextToSpeech we've already shut down via onChange();
      // mTts now points to the new engine, so running this body would clobber its
      // in-flight initialization.
      if (generation != mInitGeneration)
        return;
      if (status == TextToSpeech.ERROR)
      {
        Logger.e(TAG, "Failed to initialize TextToSpeech");
        lockDown();
        mInitializing = false;
        notifyStateChanged();
        return;
      }
      mTts.setSpeechRate(SPEECH_RATE);
      mTts.setAudioAttributes(AudioFocusManager.AUDIO_ATTRIBUTES);
      mTts.setOnUtteranceProgressListener(mUtteranceProgressListener);
      mAudioFocusManager = new AudioFocusManager(context, this::stop);
      mParams.putFloat(TextToSpeech.Engine.KEY_PARAM_VOLUME, Config.TTS.getVolume());
      final TextToSpeech tts = mTts;
      final List<Pair<String, String>> supported = getSupportedLanguages();
      final String selectedLanguage = Config.TTS.getLanguage();
      sLanguageCheckExecutor.execute(() -> {
        final LanguageCheck check = checkLanguages(tts, supported, selectedLanguage);
        UiThread.run(() -> {
          // The engine was switched while its languages were being checked.
          if (generation != mInitGeneration)
            return;
          applyLanguageCheck(check);
          mInitializing = false;
          if (mReloadTriggered && sOnReloadCallback != null)
          {
            sOnReloadCallback.run();
            mReloadTriggered = false;
          }
          notifyStateChanged();
        });
      });
    }));

    if (mTtsEngineObserver == null)
    {
      mTtsEngineObserver = new ContentObserver(new Handler(Looper.getMainLooper())) {
        @Override
        public void onChange(boolean selfChange)
        {
          Logger.d(TAG, "System TTS engine changed – reloading TTS engine");
          mReloadTriggered = true;
          if (mTts != null)
          {
            mTts.shutdown();
            mTts = null;
          }
          // Clear lockdown for the new engine; clear mInitializing because a pending
          // old-engine callback will detect its stale generation and bail without resetting it.
          mUnavailable = false;
          mInitializing = false;
          initialize(mContext);
        }
      };
      mContext.getContentResolver().registerContentObserver(Settings.Secure.getUriFor("tts_default_synth"), false,
                                                            mTtsEngineObserver);
    }
  }

  private static boolean isReady()
  {
    return INSTANCE.mTts != null && !INSTANCE.mUnavailable && !INSTANCE.mInitializing;
  }

  /**
   * Registers a listener invoked on the UI thread whenever the TTS lifecycle state may have
   * changed (init success / init failure / lockdown / enable toggle). Callers must remove
   * the listener (via {@link #removeStateChangedListener}) when their host Activity is
   * destroyed to avoid leaking its context.
   */
  public static void addStateChangedListener(@NonNull Runnable listener)
  {
    sStateChangedListeners.add(listener);
  }

  public static void removeStateChangedListener(@NonNull Runnable listener)
  {
    sStateChangedListeners.remove(listener);
  }

  private static void notifyStateChanged()
  {
    // CopyOnWriteArrayList iteration is safe under concurrent add/remove.
    for (Runnable listener : sStateChangedListeners)
      UiThread.run(listener);
  }

  @NonNull
  public static State getState()
  {
    if (INSTANCE.mUnavailable)
      return State.UNAVAILABLE;
    if (INSTANCE.mTts == null || INSTANCE.mInitializing)
      return State.INITIALIZING;
    if (!INSTANCE.mHasUsableLanguage)
      return State.NEEDS_LANGUAGE;
    return Config.TTS.isEnabled() ? State.READY_ON : State.READY_OFF;
  }

  public void speak(@NonNull String textToSpeak)
  {
    if (!isReady())
      return;

    if (!speakFirstString(textToSpeak))
      stop();
  }

  public void playTurnNotifications(@NonNull String[] turnNotifications)
  {
    if (!isReady())
      return;

    for (int i = 0; i < turnNotifications.length; i++)
    {
      final String text = turnNotifications[i];
      final boolean result = i == 0 ? speakFirstString(text) : addToQueue(text);
      if (!result)
      {
        stop();
        return;
      }
    }
  }

  private boolean speakFirstString(@NonNull String text)
  {
    if (!Config.TTS.isEnabled())
      return false;
    if (!mAudioFocusManager.requestAudioFocus())
      return false;

    mTtsQueueSize.set(0);

    final boolean isMusicActive = mAudioFocusManager.isMusicActive();
    boolean result = true;
    if (isMusicActive)
      result = mTts.playSilentUtterance(TTS_SPEAK_DELAY_MILLIS, TextToSpeech.QUEUE_FLUSH, TTS_SILENT_UTTERANCE_ID)
            == TextToSpeech.SUCCESS;
    if (result)
      mTtsQueueSize.incrementAndGet();
    else
    {
      Logger.d(TAG, "Failed to play silent utterance for music active delay");
      return false;
    }

    result = mTts.speak(text, isMusicActive ? TextToSpeech.QUEUE_ADD : TextToSpeech.QUEUE_FLUSH, mParams, text)
          == TextToSpeech.SUCCESS;
    if (result)
      mTtsQueueSize.incrementAndGet();
    else
      Logger.d(TAG, "Failed to speak text: " + text);
    return result;
  }

  private boolean addToQueue(@NonNull String text)
  {
    final boolean result = mTts.speak(text, TextToSpeech.QUEUE_ADD, mParams, text) == TextToSpeech.SUCCESS;
    if (result)
      mTtsQueueSize.incrementAndGet();
    else
      Logger.d(TAG, "Failed to add text to TTS queue: " + text);
    return result;
  }

  public void stop()
  {
    if (!isReady())
      return;

    mAudioFocusManager.releaseAudioFocus();
    mTts.stop();
    mTtsQueueSize.set(0);
  }

  public static boolean isEnabled()
  {
    return (isReady() && nativeAreTurnNotificationsEnabled());
  }

  public static void setEnabled(boolean enabled)
  {
    final boolean wasEnabled = Config.TTS.isEnabled();
    Config.TTS.setEnabled(enabled);
    nativeEnableTurnNotifications(enabled);
    if (wasEnabled != enabled)
      notifyStateChanged();
  }

  public float getVolume()
  {
    return Config.TTS.getVolume();
  }

  public void setVolume(final float volume)
  {
    mParams.putFloat(TextToSpeech.Engine.KEY_PARAM_VOLUME, volume);
    Config.TTS.setVolume(volume);
  }

  private static final class LanguageCheck
  {
    final List<LanguageData> languages = new ArrayList<>();
    // The engine threw IllegalArgumentException: it is corrupted internally.
    boolean engineFailed;
    // The downloaded language to speak, already set in the engine; null if there is none.
    @Nullable
    LanguageData selected;
  }

  /**
   * Asks the engine about every supported language and picks the one to speak: the selected language, else the
   * device's, else English. It only talks to the engine, so it may run on any thread.
   */
  @AnyThread
  private static @NonNull LanguageCheck checkLanguages(@NonNull TextToSpeech tts,
                                                       @NonNull List<Pair<String, String>> supported,
                                                       @Nullable String selectedLanguage)
  {
    final LanguageCheck check = new LanguageCheck();
    for (final Pair<String, String> langNamePair : supported)
    {
      try
      {
        check.languages.add(new LanguageData(langNamePair.first, langNamePair.second, tts));
      }
      catch (LanguageData.NotAvailableException ex)
      {
        Logger.w(TAG, "Failed to get usable languages " + ex.getMessage());
      }
      catch (IllegalArgumentException e)
      {
        Logger.e(TAG, "Failed to get usable languages", e);
        check.engineFailed = true;
        return check;
      }
    }

    LanguageData lang = findSupportedLanguage(selectedLanguage, check.languages);
    if (lang == null || !lang.downloaded)
      lang = getDefaultLanguage(check.languages);
    if (lang != null && lang.downloaded)
    {
      tts.setLanguage(lang.locale);
      check.selected = lang;
    }
    return check;
  }

  @androidx.annotation.UiThread
  private void applyLanguageCheck(@NonNull LanguageCheck check)
  {
    Assert.debug(UiThread.isUiThread(), "TTS state must be changed on the UI thread");
    // No supported language at all: lock down TTS.
    if (check.engineFailed || check.languages.isEmpty())
      lockDown();
    // Neither the selected language nor a default one is downloaded.
    else if (check.selected == null)
      Config.TTS.setEnabled(false);
    else
    {
      nativeSetTurnNotificationsLocale(check.selected.internalCode);
      Config.TTS.setLanguage(check.selected.internalCode);
    }
    mHasUsableLanguage = check.selected != null;
    setEnabled(Config.TTS.isEnabled());
    notifyStateChanged();
  }

  public @NonNull List<LanguageData> refreshLanguages()
  {
    if (mUnavailable || mTts == null)
      return new ArrayList<>();

    final LanguageCheck check = checkLanguages(mTts, getSupportedLanguages(), Config.TTS.getLanguage());
    applyLanguageCheck(check);
    return check.languages;
  }

  @VisibleForTesting
  void resetForTesting()
  {
    if (mTts != null)
      mTts.shutdown();
    if (mTtsEngineObserver != null)
      mContext.getContentResolver().unregisterContentObserver(mTtsEngineObserver);
    mTts = null;
    mTtsEngineObserver = null;
    ++mInitGeneration;
    mInitializing = false;
    mUnavailable = false;
    mHasUsableLanguage = false;
    mReloadTriggered = false;
  }

  @NonNull
  private List<Pair<String, String>> getSupportedLanguages()
  {
    if (sSupportedLanguages == null)
    {
      sSupportedLanguages = nativeGetSupportedLanguages();
    }
    return sSupportedLanguages;
  }

  private native static void nativeEnableTurnNotifications(boolean enable);
  private native static boolean nativeAreTurnNotificationsEnabled();
  private native static void nativeSetTurnNotificationsLocale(String code);
  private native static String nativeGetTurnNotificationsLocale();
  @NonNull
  private native static List<Pair<String, String>> nativeGetSupportedLanguages();
}
