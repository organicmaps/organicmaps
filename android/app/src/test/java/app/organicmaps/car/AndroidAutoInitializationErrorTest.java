package app.organicmaps.car;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.doThrow;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.Application;
import android.content.Intent;
import androidx.car.app.ScreenManager;
import androidx.car.app.Session;
import androidx.car.app.SessionInfo;
import androidx.car.app.model.MessageTemplate;
import androidx.car.app.testing.SessionController;
import androidx.car.app.testing.TestCarContext;
import androidx.lifecycle.Lifecycle;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.car.screens.ErrorScreen;
import app.organicmaps.sdk.OrganicMaps;
import app.organicmaps.sdk.car.renderer.RendererFactory;
import app.organicmaps.sdk.display.DisplayManager;
import app.organicmaps.sdk.util.log.Logger;
import java.io.IOException;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.MockedStatic;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.android.controller.ServiceController;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;

@RunWith(RobolectricTestRunner.class)
@Config(sdk = 34, application = Application.class, shadows = AndroidAutoInitializationErrorTest.Sdk.class)
public class AndroidAutoInitializationErrorTest
{
  @Test
  public void failedSetupCreatesAnErrorSessionWithoutNativeFeatureManagers() throws IOException
  {
    final MwmApplication app = mock(MwmApplication.class);
    final DisplayManager display = new DisplayManager();
    final OrganicMaps sdk = mock(OrganicMaps.class);
    when(app.getOrganicMaps()).thenReturn(sdk);
    when(app.getDisplayManager()).thenReturn(display);
    doThrow(new IOException("Storage unavailable"))
        .doAnswer(call -> {
          when(sdk.arePlatformAndCoreInitialized()).thenReturn(true);
          return null;
        })
        .when(app)
        .initOrganicMaps();
    try (MockedStatic<MwmApplication> application = mockStatic(MwmApplication.class);
         MockedStatic<app.organicmaps.sdk.util.Config> config = mockStatic(app.organicmaps.sdk.util.Config.class);
         MockedStatic<Logger> logger = mockStatic(Logger.class);
         MockedStatic<RendererFactory> renderers = mockStatic(RendererFactory.class))
    {
      application.when(() -> MwmApplication.from(any())).thenReturn(app);
      final ServiceController<AndroidAutoService> service = Robolectric.buildService(AndroidAutoService.class).create();
      try
      {
        final Session session = service.get().onCreateSession(mock(SessionInfo.class));
        final TestCarContext context = TestCarContext.createCarContext(RuntimeEnvironment.getApplication());
        final SessionController host = new SessionController(session, context, new Intent());
        context.getLifecycleOwner().getRegistry().setCurrentState(Lifecycle.State.CREATED);
        host.moveToState(Lifecycle.State.CREATED);
        context.getLifecycleOwner().getRegistry().setCurrentState(Lifecycle.State.STARTED);
        assertTrue(context.getCarService(ScreenManager.class).getTop() instanceof ErrorScreen);
        final MessageTemplate error =
            (MessageTemplate) context.getCarService(ScreenManager.class).getTop().onGetTemplate();
        assertEquals(context.getString(R.string.dialog_error_storage_message), error.getMessage().toString());
        assertEquals(context.getString(R.string.dialog_error_storage_title), error.getHeader().getTitle().toString());
        assertFalse(display.isCarDisplayUsed());
        renderers.verifyNoInteractions();
        context.getLifecycleOwner().getRegistry().setCurrentState(Lifecycle.State.DESTROYED);
        verify(app).initOrganicMaps();
        assertTrue(service.get().onCreateSession(mock(SessionInfo.class)) instanceof AndroidAutoSession);
        assertFalse(display.isCarDisplayUsed());
        verify(app, times(2)).initOrganicMaps();
      }
      finally
      {
        service.destroy();
      }
    }
  }

  @Implements(OrganicMaps.class)
  public static class Sdk
  {
    @Implementation
    protected static void __staticInitializer__()
    {}
  }
}
