package app.organicmaps.sdk;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.when;
import static org.robolectric.Shadows.shadowOf;

import android.app.Activity;
import android.app.Application;
import android.content.Intent;
import android.os.Looper;
import android.widget.TextView;
import androidx.annotation.NonNull;
import app.organicmaps.MapPlaceholderActivity;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.sdk.display.DisplayChangedListener;
import app.organicmaps.sdk.display.DisplayManager;
import app.organicmaps.sdk.display.DisplayType;
import app.organicmaps.sdk.util.concurrency.UiThread;
import app.organicmaps.sdk.util.log.Logger;
import java.util.ArrayList;
import java.util.List;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.MockedStatic;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.android.controller.ActivityController;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;

@RunWith(RobolectricTestRunner.class)
@Config(sdk = 34, application = Application.class, shadows = MapPlaceholderActivityReadinessTest.Sdk.class)
public class MapPlaceholderActivityReadinessTest
{
  private final MapsReadiness mReadiness = new MapsReadiness();
  private final DisplayManager mDisplay = new DisplayManager();
  private final List<String> mCarHandoffs = new ArrayList<>();
  private MockedStatic<MwmApplication> mApplication;
  private MockedStatic<Logger> mLogger;

  @Before
  public void connectCarWhileMapsLoad()
  {
    mLogger = mockStatic(Logger.class);
    final OrganicMaps sdk = mock(OrganicMaps.class);
    when(sdk.arePlatformAndCoreInitialized()).thenReturn(true);
    final MwmApplication host = mock(MwmApplication.class);
    when(host.getOrganicMaps()).thenReturn(sdk);
    when(host.getMapsReadiness()).thenReturn(mReadiness);
    when(host.getDisplayManager()).thenReturn(mDisplay);
    mApplication = mockStatic(MwmApplication.class);
    mApplication.when(() -> MwmApplication.from(any())).thenReturn(host);
    mDisplay.addListener(DisplayType.Car, new DisplayChangedListener() {
      @Override
      public void onDisplayChangedToDevice(@NonNull Runnable onTaskFinishedCallback)
      {
        mCarHandoffs.add("device");
        onTaskFinishedCallback.run();
      }

      @Override
      public void onDisplayChangedToCar(@NonNull Runnable onTaskFinishedCallback)
      {
        mCarHandoffs.add("car");
        onTaskFinishedCallback.run();
      }
    });
  }

  @After
  public void closeMocks()
  {
    mApplication.close();
    mLogger.close();
  }

  @Test
  public void phoneHandoffOpensTheMapOnlyAfterMapsLoad()
  {
    try (ActivityController<MapPlaceholderActivity> placeholder = createPlaceholder())
    {
      assertHandoffStatus(placeholder.get(), false);
      continueOnPhone(placeholder.get());
      assertHandoffStatus(placeholder.get(), true);
      assertFalse(placeholder.get().isFinishing());
      assertNull(shadowOf(placeholder.get()).getNextStartedActivity());

      mReadiness.complete();

      assertPhoneChosen(placeholder.get());
    }
  }

  @Test
  public void returningToTheCarWinsOverQueuedMapCompletion()
  {
    try (ActivityController<MapPlaceholderActivity> placeholder = createPlaceholder())
    {
      continueOnPhone(placeholder.get());

      UiThread.runLater(mReadiness::complete);
      mDisplay.changeDisplay(DisplayType.Car);
      shadowOf(Looper.getMainLooper()).idle();
      assertEquals(List.of("device", "car"), mCarHandoffs);

      assertHandoffStatus(placeholder.get(), false);
      assertFalse(placeholder.get().isFinishing());
      assertNull(shadowOf(placeholder.get()).getNextStartedActivity());
    }
  }

  @Test
  public void recreatingThePlaceholderWhileMapsLoadKeepsThePhoneHandoff()
  {
    try (ActivityController<MapPlaceholderActivity> placeholder = createPlaceholder())
    {
      continueOnPhone(placeholder.get());
      placeholder.recreate();
      shadowOf(Looper.getMainLooper()).idle();
      assertEquals(List.of("device"), mCarHandoffs);

      mReadiness.complete();

      assertPhoneChosen(placeholder.get());
    }
  }

  @Test
  public void pausedPlaceholderOpensTheMapOnlyAfterResume()
  {
    try (ActivityController<MapPlaceholderActivity> placeholder = createPlaceholder())
    {
      continueOnPhone(placeholder.get());
      placeholder.pause();

      mReadiness.complete();
      assertFalse(placeholder.get().isFinishing());
      assertNull(shadowOf(placeholder.get()).getNextStartedActivity());

      placeholder.resume();
      assertPhoneChosen(placeholder.get());
    }
  }

  @Test
  public void phoneHandoffWhilePausedWaitsForResume()
  {
    try (ActivityController<MapPlaceholderActivity> placeholder = createPlaceholder())
    {
      placeholder.pause();
      mDisplay.changeDisplay(DisplayType.Device);
      shadowOf(Looper.getMainLooper()).idle();
      assertEquals(List.of("device"), mCarHandoffs);

      mReadiness.complete();
      assertFalse(placeholder.get().isFinishing());
      assertNull(shadowOf(placeholder.get()).getNextStartedActivity());

      placeholder.resume();
      assertPhoneChosen(placeholder.get());
    }
  }

  private static ActivityController<MapPlaceholderActivity> createPlaceholder()
  {
    final ActivityController<MapPlaceholderActivity> placeholder =
        Robolectric.buildActivity(MapPlaceholderActivity.class);
    placeholder.get().setTheme(R.style.MwmTheme);
    placeholder.create().start().resume();
    return placeholder;
  }

  private static void assertPhoneChosen(MapPlaceholderActivity placeholder)
  {
    assertEquals(Activity.RESULT_OK, shadowOf(placeholder).getResultCode());
    assertNull(shadowOf(placeholder).getNextStartedActivity());
    assertTrue(placeholder.isFinishing());
  }

  private static void assertHandoffStatus(MapPlaceholderActivity placeholder, boolean waiting)
  {
    final TextView status = placeholder.findViewById(R.id.map_placeholder_status);
    assertEquals(placeholder.getString(waiting ? R.string.please_wait : R.string.car_used_on_the_car_screen),
                 status.getText().toString());
    assertEquals(!waiting, placeholder.findViewById(R.id.btn_continue).isEnabled());
  }

  private void continueOnPhone(MapPlaceholderActivity placeholder)
  {
    placeholder.findViewById(R.id.btn_continue).performClick();
    shadowOf(Looper.getMainLooper()).idle();
    assertEquals(List.of("device"), mCarHandoffs);
  }

  @Implements(OrganicMaps.class)
  public static class Sdk
  {
    @Implementation
    protected static void __staticInitializer__()
    {}
  }
}
