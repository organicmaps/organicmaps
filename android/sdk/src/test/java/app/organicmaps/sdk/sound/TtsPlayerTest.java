package app.organicmaps.sdk.sound;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotSame;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mockStatic;
import static org.robolectric.Shadows.shadowOf;

import android.os.Looper;
import android.provider.Settings;
import android.speech.tts.TextToSpeech;
import android.util.Pair;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.util.log.Logger;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.Executor;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.MockedStatic;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;
import org.robolectric.shadow.api.Shadow;
import org.robolectric.shadows.ShadowTextToSpeech;

@RunWith(RobolectricTestRunner.class)
@Config(manifest = Config.NONE, sdk = 28,
        shadows = {TtsPlayerTest.TtsPlayerShadow.class, TtsPlayerTest.TtsConfigShadow.class,
                   TtsPlayerTest.EngineShadow.class})
public class TtsPlayerTest
{
  @Implements(value = TtsPlayer.class, isInAndroidSdk = false)
  public static class TtsPlayerShadow
  {
    static boolean sTurnNotificationsEnabled;
    @Nullable
    static String sTurnNotificationsLocale;

    @Implementation
    protected static List<Pair<String, String>> nativeGetSupportedLanguages()
    {
      return List.of(new Pair<>("en", "English"), new Pair<>("de", "Deutsch"), new Pair<>("ru", "Russian"));
    }

    @Implementation
    protected static void nativeEnableTurnNotifications(boolean enable)
    {
      sTurnNotificationsEnabled = enable;
    }

    @Implementation
    protected static boolean nativeAreTurnNotificationsEnabled()
    {
      return sTurnNotificationsEnabled;
    }

    @Implementation
    protected static void nativeSetTurnNotificationsLocale(String code)
    {
      sTurnNotificationsLocale = code;
    }
  }

  @Implements(value = app.organicmaps.sdk.util.Config.TTS.class, isInAndroidSdk = false)
  public static class TtsConfigShadow
  {
    static boolean sEnabled;
    static String sLanguage;

    @Implementation
    protected static boolean isEnabled()
    {
      return sEnabled;
    }

    @Implementation
    protected static void setEnabled(boolean enabled)
    {
      sEnabled = enabled;
    }

    @Implementation
    protected static String getLanguage()
    {
      return sLanguage;
    }

    @Implementation
    protected static void setLanguage(String language)
    {
      sLanguage = language;
    }

    @Implementation
    protected static float getVolume()
    {
      return 1.0f;
    }
  }

  // An engine that answers isLanguageAvailable() from a table and records which threads asked.
  @Implements(TextToSpeech.class)
  public static class EngineShadow extends ShadowTextToSpeech
  {
    static final Map<Locale, Integer> sAvailability = new ConcurrentHashMap<>();
    static final Set<Thread> sCallingThreads = ConcurrentHashMap.newKeySet();
    static volatile boolean sCorrupted;

    @Implementation
    @Override
    protected int isLanguageAvailable(Locale locale)
    {
      sCallingThreads.add(Thread.currentThread());
      if (sCorrupted)
        throw new IllegalArgumentException("Corrupted engine");
      final Integer status = sAvailability.get(locale);
      return status == null ? TextToSpeech.LANG_NOT_SUPPORTED : status;
    }
  }

  private final List<Runnable> mPendingChecks = new ArrayList<>();
  private Executor mDefaultExecutor;
  private Locale mDefaultLocale;
  private MockedStatic<Logger> mLogger;

  @Before
  public void setUp()
  {
    mLogger = mockStatic(Logger.class);
    mDefaultLocale = Locale.getDefault();
    Locale.setDefault(Locale.US);
    mDefaultExecutor = TtsPlayer.sLanguageCheckExecutor;
    TtsPlayer.sLanguageCheckExecutor = mPendingChecks::add;
    TtsConfigShadow.sEnabled = true;
    TtsConfigShadow.sLanguage = "";
    TtsPlayerShadow.sTurnNotificationsEnabled = false;
    TtsPlayerShadow.sTurnNotificationsLocale = null;
    EngineShadow.sAvailability.clear();
    EngineShadow.sCallingThreads.clear();
    EngineShadow.sCorrupted = false;
  }

  @After
  public void tearDown()
  {
    TtsPlayer.INSTANCE.resetForTesting();
    TtsPlayer.sLanguageCheckExecutor = mDefaultExecutor;
    Locale.setDefault(mDefaultLocale);
    mLogger.close();
  }

  private static void setAvailability(int status, String... languages)
  {
    for (String language : languages)
      EngineShadow.sAvailability.put(new Locale(language), status);
  }

  private static TextToSpeech lastEngine()
  {
    return ShadowTextToSpeech.getLastTextToSpeechInstance();
  }

  private static void finishEngineInit(int status)
  {
    ((ShadowTextToSpeech) Shadow.extract(lastEngine())).getOnInitListener().onInit(status);
  }

  private static void startPlayer(int initStatus)
  {
    TtsPlayer.INSTANCE.initialize(RuntimeEnvironment.getApplication());
    finishEngineInit(initStatus);
  }

  private void runPendingChecks()
  {
    final List<Runnable> checks = new ArrayList<>(mPendingChecks);
    mPendingChecks.clear();
    for (Runnable check : checks)
      check.run();
    shadowOf(Looper.getMainLooper()).idle();
  }

  @Test
  public void startupChecksLanguagesOffTheUiThread() throws InterruptedException
  {
    setAvailability(TextToSpeech.LANG_AVAILABLE, "en", "de");
    final ExecutorService worker = Executors.newSingleThreadExecutor();
    TtsPlayer.sLanguageCheckExecutor = worker;

    startPlayer(TextToSpeech.SUCCESS);
    worker.shutdown();
    assertTrue(worker.awaitTermination(5, TimeUnit.SECONDS));

    // The check is done, but its result waits for the UI thread.
    assertEquals(TtsPlayer.State.INITIALIZING, TtsPlayer.getState());
    shadowOf(Looper.getMainLooper()).idle();
    assertEquals(TtsPlayer.State.READY_ON, TtsPlayer.getState());
    assertFalse(EngineShadow.sCallingThreads.isEmpty());
    assertFalse(EngineShadow.sCallingThreads.contains(Looper.getMainLooper().getThread()));
  }

  @Test
  public void startupPicksTheSelectedLanguage()
  {
    setAvailability(TextToSpeech.LANG_AVAILABLE, "en", "de", "ru");
    TtsConfigShadow.sLanguage = "de";

    startPlayer(TextToSpeech.SUCCESS);
    runPendingChecks();

    assertEquals(TtsPlayer.State.READY_ON, TtsPlayer.getState());
    assertEquals("de", TtsPlayerShadow.sTurnNotificationsLocale);
    assertEquals(new Locale("de"), ((ShadowTextToSpeech) Shadow.extract(lastEngine())).getCurrentLanguage());
  }

  @Test
  public void startupFallsBackToEnglishWhenTheSelectedAndDeviceLanguagesAreNotDownloaded()
  {
    setAvailability(TextToSpeech.LANG_AVAILABLE, "en");
    setAvailability(TextToSpeech.LANG_MISSING_DATA, "de", "ru");
    TtsConfigShadow.sLanguage = "de";
    Locale.setDefault(new Locale("ru", "RU"));

    startPlayer(TextToSpeech.SUCCESS);
    runPendingChecks();

    assertEquals(TtsPlayer.State.READY_ON, TtsPlayer.getState());
    assertEquals("en", TtsConfigShadow.sLanguage);
    assertEquals("en", TtsPlayerShadow.sTurnNotificationsLocale);
  }

  @Test
  public void startupNeedsALanguageWhenNoneIsDownloaded()
  {
    setAvailability(TextToSpeech.LANG_MISSING_DATA, "en", "de");

    startPlayer(TextToSpeech.SUCCESS);
    runPendingChecks();

    assertEquals(TtsPlayer.State.NEEDS_LANGUAGE, TtsPlayer.getState());
    assertFalse(TtsConfigShadow.sEnabled);
  }

  @Test
  public void startupLocksDownWhenTheEngineSpeaksNoSupportedLanguage()
  {
    startPlayer(TextToSpeech.SUCCESS);
    runPendingChecks();

    assertEquals(TtsPlayer.State.UNAVAILABLE, TtsPlayer.getState());
  }

  @Test
  public void startupLocksDownWhenTheEngineIsCorrupted()
  {
    setAvailability(TextToSpeech.LANG_AVAILABLE, "en");
    EngineShadow.sCorrupted = true;

    startPlayer(TextToSpeech.SUCCESS);
    runPendingChecks();

    assertEquals(TtsPlayer.State.UNAVAILABLE, TtsPlayer.getState());
    assertFalse(TtsConfigShadow.sEnabled);
  }

  @Test
  public void checkOfAnEngineSwitchedMeanwhileIsDropped()
  {
    setAvailability(TextToSpeech.LANG_AVAILABLE, "en");
    startPlayer(TextToSpeech.SUCCESS);
    assertEquals(1, mPendingChecks.size());
    final Runnable staleCheck = mPendingChecks.remove(0);

    // The user picks another engine before the first one's languages are checked.
    final TextToSpeech firstEngine = lastEngine();
    RuntimeEnvironment.getApplication().getContentResolver().notifyChange(
        Settings.Secure.getUriFor("tts_default_synth"), null);
    shadowOf(Looper.getMainLooper()).idle();
    assertNotSame(firstEngine, lastEngine());

    staleCheck.run();
    shadowOf(Looper.getMainLooper()).idle();
    assertEquals(TtsPlayer.State.INITIALIZING, TtsPlayer.getState());

    finishEngineInit(TextToSpeech.SUCCESS);
    runPendingChecks();
    assertEquals(TtsPlayer.State.READY_ON, TtsPlayer.getState());
  }

  @Test
  public void settingsRefreshReturnsTheLanguagesTheEngineSpeaks()
  {
    setAvailability(TextToSpeech.LANG_AVAILABLE, "en", "ru");
    startPlayer(TextToSpeech.SUCCESS);
    runPendingChecks();

    final List<String> codes = new ArrayList<>();
    for (LanguageData language : TtsPlayer.INSTANCE.refreshLanguages())
      codes.add(language.internalCode);

    assertEquals(List.of("en", "ru"), codes);
    assertEquals(TtsPlayer.State.READY_ON, TtsPlayer.getState());
  }
}
