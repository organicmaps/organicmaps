package app.organicmaps.intent;

import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.CALLS_REAL_METHODS;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.mockito.Mockito.withSettings;

import android.content.Intent;
import android.net.Uri;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.util.log.Logger;
import java.lang.reflect.Field;
import org.junit.Test;
import org.mockito.MockedStatic;

public class GoogleAssistantIntentHandlerTest
{
  @Test
  public void resumeDoesNotStartBuiltPreviewWithFixedStart() throws ReflectiveOperationException
  {
    checkResume(false, RoutingController.BuildState.BUILT, false);
  }

  @Test
  public void resumeStartsBuiltPreviewFromMyPosition() throws ReflectiveOperationException
  {
    checkResume(true, RoutingController.BuildState.BUILT, true);
  }

  @Test
  public void resumeDoesNotStartWhileRebuilding() throws ReflectiveOperationException
  {
    checkResume(true, RoutingController.BuildState.BUILDING, false);
  }

  @SuppressWarnings({"rawtypes", "unchecked"})
  private static void checkResume(boolean myPosition, RoutingController.BuildState buildState, boolean shouldStart)
      throws ReflectiveOperationException
  {
    final RoutingController controller =
        mock(RoutingController.class, withSettings().useConstructor().defaultAnswer(CALLS_REAL_METHODS));
    final Field state = RoutingController.class.getDeclaredField("mState");
    state.setAccessible(true);
    state.set(controller, Enum.valueOf((Class) state.getType(), "PREPARE"));
    final Field build = RoutingController.class.getDeclaredField("mBuildState");
    build.setAccessible(true);
    build.set(controller, buildState);
    final MapObject start = mock(MapObject.class);
    when(start.isMyPosition()).thenReturn(myPosition);
    doReturn(start).when(controller).getStartPoint();
    if (shouldStart)
      doNothing().when(controller).start();

    final Uri uri = mock(Uri.class);
    when(uri.getScheme()).thenReturn("geo.action");
    when(uri.isHierarchical()).thenReturn(true);
    when(uri.getQueryParameter("act")).thenReturn("resume_navigation");
    final Intent intent = mock(Intent.class);
    when(intent.getData()).thenReturn(uri);
    when(intent.getAction()).thenReturn(Intent.ACTION_VIEW);

    try (MockedStatic<RoutingController> routing = mockStatic(RoutingController.class);
         MockedStatic<Logger> ignored = mockStatic(Logger.class))
    {
      routing.when(RoutingController::get).thenReturn(controller);
      assertTrue(new GoogleAssistantIntentHandler() {}.handleIntent(
          intent, mock(GoogleAssistantIntentHandler.SearchHandler.class)));
      if (shouldStart)
        verify(controller).start();
      else
        verify(controller, never()).start();
    }
  }
}
