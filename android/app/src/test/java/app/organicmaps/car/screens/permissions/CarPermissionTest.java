package app.organicmaps.car.screens.permissions;

import static android.Manifest.permission.ACCESS_COARSE_LOCATION;
import static android.Manifest.permission.ACCESS_FINE_LOCATION;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoInteractions;
import static org.robolectric.Shadows.shadowOf;

import android.app.Application;
import android.os.Looper;
import androidx.car.app.OnDoneCallback;
import androidx.car.app.ScreenManager;
import androidx.car.app.model.MessageTemplate;
import androidx.car.app.testing.TestCarContext;
import androidx.lifecycle.Lifecycle;
import app.organicmaps.R;
import app.organicmaps.car.screens.ErrorScreen;
import app.organicmaps.sdk.OrganicMaps;
import java.time.Duration;
import java.util.List;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;

@RunWith(RobolectricTestRunner.class)
@Config(sdk = 34, application = Application.class, shadows = CarPermissionTest.Sdk.class)
public class CarPermissionTest
{
  @Test
  public void approximateGrantKeepsRequiredSetupUntilTheDriverChoosesClose()
  {
    final Application app = RuntimeEnvironment.getApplication();
    shadowOf(app).grantPermissions(ACCESS_COARSE_LOCATION);
    final TestCarContext context = TestCarContext.createCarContext(app);
    final Runnable granted = mock(Runnable.class);
    final RequestPermissionsScreenWithApi screen =
        new RequestPermissionsScreenWithApi(context, mock(OrganicMaps.class), granted);
    final ScreenManager stack = context.getCarService(ScreenManager.class);
    context.getLifecycleOwner().getRegistry().setCurrentState(Lifecycle.State.RESUMED);
    stack.push(screen);
    final MessageTemplate template = (MessageTemplate) screen.onGetTemplate();
    template.getActions().get(0).getOnClickDelegate().sendClick(mock(OnDoneCallback.class));
    context.getLifecycleOwner().getRegistry().setCurrentState(Lifecycle.State.CREATED);
    context.getLastPermissionRequestInfo().getListener().onRequestPermissionsResult(List.of(ACCESS_COARSE_LOCATION),
                                                                                    List.of(ACCESS_FINE_LOCATION));
    assertSame(screen, stack.getTop());
    context.getLifecycleOwner().getRegistry().setCurrentState(Lifecycle.State.RESUMED);
    assertTrue(stack.getTop() instanceof ErrorScreen);
    assertEquals(context.getString(R.string.precise_location_is_disabled_long_text),
                 ((MessageTemplate) stack.getTop().onGetTemplate()).getMessage().toString());
    stack.pop();
    context.getOnBackPressedDispatcher().onBackPressed();
    assertFalse(context.hasCalledFinishCarApp());
    assertSame(screen, stack.getTop());
    final MessageTemplate required = (MessageTemplate) screen.onGetTemplate();
    assertEquals(context.getString(R.string.close), required.getActions().get(1).getTitle().toString());
    required.getActions().get(1).getOnClickDelegate().sendClick(mock(OnDoneCallback.class));
    assertTrue(context.hasCalledFinishCarApp());
    verifyNoInteractions(granted);
  }

  @Test
  public void notificationPollOnlyCompletesWhileTheScreenIsStarted()
  {
    final Application app = RuntimeEnvironment.getApplication();
    shadowOf(app).grantPermissions(ACCESS_COARSE_LOCATION, android.Manifest.permission.POST_NOTIFICATIONS);
    final TestCarContext context = TestCarContext.createCarContext(app);
    final Runnable granted = mock(Runnable.class);
    final RequestPermissionsScreenWithNotification screen =
        new RequestPermissionsScreenWithNotification(context, mock(OrganicMaps.class), granted);
    context.getLifecycleOwner().getRegistry().setCurrentState(Lifecycle.State.RESUMED);
    context.getCarService(ScreenManager.class).push(screen);
    shadowOf(Looper.getMainLooper()).idleFor(Duration.ofSeconds(1));
    verifyNoInteractions(granted);
    context.getLifecycleOwner().getRegistry().setCurrentState(Lifecycle.State.CREATED);
    shadowOf(app).grantPermissions(ACCESS_FINE_LOCATION);
    shadowOf(Looper.getMainLooper()).idleFor(Duration.ofSeconds(1));
    verifyNoInteractions(granted);
    context.getLifecycleOwner().getRegistry().setCurrentState(Lifecycle.State.RESUMED);
    shadowOf(Looper.getMainLooper()).idle();
    verify(granted).run();
  }

  @Implements(OrganicMaps.class)
  public static class Sdk
  {
    @Implementation
    protected static void __staticInitializer__()
    {}
  }
}
