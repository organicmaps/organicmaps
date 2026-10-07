package app.organicmaps.intent;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.when;

import android.app.Application;
import android.content.Intent;
import android.net.Uri;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.util.log.Logger;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.MockedStatic;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;

@RunWith(RobolectricTestRunner.class)
@Config(sdk = 34, application = Application.class, shadows = GoogleAssistantIntentHandlerTest.NativeFramework.class)
public class GoogleAssistantIntentHandlerTest
{
  @Implements(value = Framework.class, isInAndroidSdk = false)
  public static class NativeFramework
  {
    static int sFollowCalls;

    @Implementation
    protected static void nativeSaveRoutePoints()
    {}

    @Implementation
    protected static void nativeRouteRemoveElevationActivePoint()
    {}

    @Implementation
    protected static void nativeFollowRoute()
    {
      ++sFollowCalls;
    }
  }

  @Test
  public void resumeDoesNotStartBuiltPreviewWithFixedStart()
  {
    checkResume(MapObject.POI, true, false);
  }

  @Test
  public void resumeStartsBuiltPreviewFromMyPosition()
  {
    checkResume(MapObject.MY_POSITION, true, true);
  }

  @Test
  public void resumeDoesNotStartWhileRebuilding()
  {
    checkResume(MapObject.MY_POSITION, false, false);
  }

  private static void checkResume(@MapObject.MapObjectType int originType, boolean built, boolean shouldStart)
  {
    final RoutingController controller = new RoutingController() {
      @Override
      public boolean isPlanning()
      {
        return !isNavigating();
      }

      @Override
      public boolean isBuilt()
      {
        return built;
      }

      @Override
      public MapObject getStartPoint()
      {
        return MapObject.createMapObject(originType, "Start", "", 1, 2);
      }
    };

    final Uri uri = mock(Uri.class);
    when(uri.getScheme()).thenReturn("geo.action");
    when(uri.isHierarchical()).thenReturn(true);
    when(uri.getQueryParameter("act")).thenReturn("resume_navigation");
    final Intent intent = mock(Intent.class);
    when(intent.getData()).thenReturn(uri);
    when(intent.getAction()).thenReturn(Intent.ACTION_VIEW);

    NativeFramework.sFollowCalls = 0;
    try (MockedStatic<RoutingController> routing = mockStatic(RoutingController.class);
         MockedStatic<Logger> ignored = mockStatic(Logger.class))
    {
      routing.when(RoutingController::get).thenReturn(controller);
      assertTrue(new GoogleAssistantIntentHandler() {}.handleIntent(
          intent, mock(GoogleAssistantIntentHandler.SearchHandler.class)));
      assertEquals(shouldStart, controller.isNavigating());
      assertEquals(shouldStart ? 1 : 0, NativeFramework.sFollowCalls);
    }
  }
}
