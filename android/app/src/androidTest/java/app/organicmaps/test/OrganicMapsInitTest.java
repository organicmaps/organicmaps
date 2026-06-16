package app.organicmaps.test;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import app.organicmaps.MwmApplication;
import app.organicmaps.sdk.OrganicMaps;
import java.io.IOException;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import org.junit.BeforeClass;
import org.junit.FixMethodOrder;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.MethodSorters;

/**
 * Integration tests for the OrganicMaps initialization state machine.
 *
 * Tests are ordered because native framework init is a one-time operation
 * per process — the first test performs cold-start init, subsequent tests
 * exercise the post-READY API contract.
 *
 * Runs in the app module (not sdk) because nativeInitFramework needs
 * the full app assets (translations, map data).
 */
@RunWith(AndroidJUnit4.class)
@FixMethodOrder(MethodSorters.NAME_ASCENDING)
public class OrganicMapsInitTest
{
  private static final int INIT_TIMEOUT_SEC = 30;

  private static MwmApplication sApp;
  private static OrganicMaps sOrganicMaps;

  @BeforeClass
  public static void setUp()
  {
    sApp = (MwmApplication) InstrumentationRegistry.getInstrumentation().getTargetContext().getApplicationContext();
    sOrganicMaps = sApp.getOrganicMaps();
  }

  @Test
  public void test1_coldStartInitTransitionsToReady() throws Exception
  {
    // Skip if already initialized (e.g., by the test app's Application.onCreate).
    if (sOrganicMaps.arePlatformAndCoreInitialized())
      return;

    final CountDownLatch initLatch = new CountDownLatch(1);
    final AtomicBoolean wasInitializing = new AtomicBoolean(false);
    final AtomicBoolean wasNotReady = new AtomicBoolean(false);

    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      try
      {
        boolean async = sApp.initOrganicMaps(initLatch::countDown);
        assertTrue("First init should be async", async);
        wasInitializing.set(sOrganicMaps.isCoreInitializing());
        wasNotReady.set(!sOrganicMaps.arePlatformAndCoreInitialized());
      }
      catch (IOException e)
      {
        throw new RuntimeException(e);
      }
    });

    assertTrue("State should be INITIALIZING right after init()", wasInitializing.get());
    assertTrue("Should not be ready right after init()", wasNotReady.get());

    assertTrue("Init should complete within " + INIT_TIMEOUT_SEC + "s",
               initLatch.await(INIT_TIMEOUT_SEC, TimeUnit.SECONDS));

    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      assertTrue("Should be ready after callback", sOrganicMaps.arePlatformAndCoreInitialized());
      assertFalse("Should not be initializing after callback", sOrganicMaps.isCoreInitializing());
    });
  }

  @Test
  public void test2_warmLaunchReturnsFalseWithoutCallingCallback() throws Exception
  {
    // Ensure init has completed (may have been done by test1 or Application.onCreate).
    ensureReady();

    final AtomicBoolean callbackCalled = new AtomicBoolean(false);

    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      try
      {
        boolean async = sApp.initOrganicMaps(() -> callbackCalled.set(true));
        assertFalse("Warm launch should return false", async);
      }
      catch (IOException e)
      {
        throw new RuntimeException(e);
      }
    });

    assertFalse("Warm launch should NOT call callback", callbackCalled.get());
    assertTrue("Should still be ready", sOrganicMaps.arePlatformAndCoreInitialized());
  }

  @Test
  public void test3_runWhenReadyExecutesImmediately() throws Exception
  {
    ensureReady();

    final AtomicBoolean executed = new AtomicBoolean(false);

    InstrumentationRegistry.getInstrumentation().runOnMainSync(
        () -> sOrganicMaps.runWhenReady(() -> executed.set(true)));

    assertTrue("runWhenReady should execute immediately when READY", executed.get());
  }

  @Test
  public void test4_runWhenReadyFromCallbackDoesNotThrow() throws Exception
  {
    ensureReady();

    // Verify that calling runWhenReady() inside a runWhenReady() callback
    // does not throw (tests the drain-into-local-copy guard).
    final AtomicBoolean innerExecuted = new AtomicBoolean(false);

    InstrumentationRegistry.getInstrumentation().runOnMainSync(
        () -> sOrganicMaps.runWhenReady(() -> sOrganicMaps.runWhenReady(() -> innerExecuted.set(true))));

    assertTrue("Nested runWhenReady should execute without throwing", innerExecuted.get());
  }

  @Test
  public void test5_isCoreInitializingFalseWhenReady() throws Exception
  {
    ensureReady();

    assertFalse("isCoreInitializing should be false in READY state", sOrganicMaps.isCoreInitializing());
  }

  /// Wait for init to complete if it hasn't already.
  private void ensureReady() throws Exception
  {
    if (sOrganicMaps.arePlatformAndCoreInitialized())
      return;

    final CountDownLatch latch = new CountDownLatch(1);
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      try
      {
        sApp.initOrganicMaps(latch::countDown);
      }
      catch (IOException e)
      {
        throw new RuntimeException(e);
      }
    });
    assertTrue("Init should complete within " + INIT_TIMEOUT_SEC + "s",
               latch.await(INIT_TIMEOUT_SEC, TimeUnit.SECONDS));
  }
}
