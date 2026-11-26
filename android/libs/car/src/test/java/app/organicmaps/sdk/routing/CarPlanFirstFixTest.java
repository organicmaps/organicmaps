package app.organicmaps.sdk.routing;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.location.Location;
import androidx.car.app.testing.TestCarContext;
import app.organicmaps.car.screens.PlaceScreen;
import app.organicmaps.sdk.OrganicMaps;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.car.renderer.Renderer;
import app.organicmaps.sdk.location.LocationHelper;
import app.organicmaps.sdk.location.LocationListener;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.MockedStatic;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;

@RunWith(RobolectricTestRunner.class)
@Config(shadows = CarPlanFirstFixTest.Sdk.class)
public class CarPlanFirstFixTest
{
  private final LocationHelper mLocation = mock(LocationHelper.class);
  private final RoutingController mRouting = mock(RoutingController.class);
  private final MapObject mDestination = MapObject.createMapObject(MapObject.POI, "Finish", "", 1, 2);
  private final MapObject mOrigin = MapObject.createMapObject(MapObject.MY_POSITION, "", "", 3, 4);
  private PlaceScreen mScreen;
  private MockedStatic<RoutingController> mController;

  @Before
  public void adoptPhonePlan()
  {
    mController = mockStatic(RoutingController.class);
    mController.when(RoutingController::get).thenReturn(mRouting);
    final OrganicMaps sdk = mock(OrganicMaps.class);
    when(sdk.getLocationHelper()).thenReturn(mLocation);
    when(mRouting.getLastRouterType()).thenReturn(PlaceScreen.ROUTER);
    when(mRouting.isPlanning()).thenReturn(true);
    when(mRouting.getEndPoint()).thenReturn(mDestination);
    mScreen =
        new PlaceScreen
            .Builder(TestCarContext.createCarContext(RuntimeEnvironment.getApplication()), sdk, mock(Renderer.class))
            .setMapObject(mDestination)
            .build();
    mScreen.onCreate(mScreen);
    verify(mRouting, never()).prepare(any(), any());
  }

  @After
  public void close()
  {
    mController.close();
  }

  private LocationListener resume()
  {
    mScreen.onResume(mScreen);
    ArgumentCaptor<LocationListener> capture = ArgumentCaptor.forClass(LocationListener.class);
    verify(mLocation).addListener(capture.capture());
    when(mLocation.getMyPosition()).thenReturn(mOrigin);
    return capture.getValue();
  }

  @Test
  public void firstFixCompletesAdoptedPlan()
  {
    final LocationListener listener = resume();
    listener.onLocationUpdated(new Location("gps"));
    verify(mRouting).setStartPoint(mOrigin);
    verify(mLocation).removeListener(listener);
  }

  @Test
  public void firstFixPreservesAnExplicitOrigin()
  {
    final LocationListener listener = resume();
    when(mRouting.getStartPoint()).thenReturn(mDestination);
    listener.onLocationUpdated(new Location("gps"));
    verify(mRouting, never()).setStartPoint(any());
  }

  @Test
  public void cancelledPlanIsNotRestarted()
  {
    final LocationListener listener = resume();
    when(mRouting.isPlanning()).thenReturn(false);
    listener.onLocationUpdated(new Location("gps"));
    verify(mRouting, never()).setStartPoint(any());
  }

  @Test
  public void pausedScreenReleasesItsListener()
  {
    final LocationListener listener = resume();
    mScreen.onPause(mScreen);
    verify(mLocation).removeListener(listener);
  }

  @Implements(OrganicMaps.class)
  public static class Sdk
  {
    @Implementation
    protected static void __staticInitializer__()
    {}
  }
}
