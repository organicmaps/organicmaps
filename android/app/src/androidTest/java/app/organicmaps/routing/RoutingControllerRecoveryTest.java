package app.organicmaps.routing;

import static android.Manifest.permission.ACCESS_COARSE_LOCATION;
import static android.Manifest.permission.ACCESS_FINE_LOCATION;
import static android.Manifest.permission.POST_NOTIFICATIONS;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import android.app.Instrumentation;
import android.location.Location;
import android.location.LocationManager;
import android.os.Build;
import android.os.SystemClock;
import android.view.View;
import androidx.core.view.ViewCompat;
import androidx.lifecycle.Lifecycle;
import androidx.lifecycle.ViewModelProvider;
import androidx.test.core.app.ActivityScenario;
import androidx.test.platform.app.InstrumentationRegistry;
import androidx.test.runner.permission.PermissionRequester;
import app.organicmaps.MwmActivity;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.maplayer.MapButtonsController;
import app.organicmaps.maplayer.MapButtonsViewModel;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.Map;
import app.organicmaps.sdk.Router;
import app.organicmaps.sdk.bookmarks.data.BookmarkCategory;
import app.organicmaps.sdk.bookmarks.data.BookmarkManager;
import app.organicmaps.sdk.bookmarks.data.DataChangedListener;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.routing.FollowTrackResult;
import app.organicmaps.sdk.routing.ResultCodes;
import app.organicmaps.sdk.routing.RouteMarkData;
import app.organicmaps.sdk.routing.RouteMarkType;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.routing.RoutingListener;
import app.organicmaps.widget.placepage.ElevationProfileChart;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.io.UncheckedIOException;
import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.FutureTask;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;
import java.util.function.BooleanSupplier;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

public class RoutingControllerRecoveryTest
{
  private final Instrumentation mInstrumentation = InstrumentationRegistry.getInstrumentation();
  private final MwmApplication mApplication = MwmApplication.from(mInstrumentation.getTargetContext());
  private RoutingController mController;
  private RoutingController.Container mContainer;
  private Router mOriginalRouter;

  @Before
  public void initializeCore() throws Exception
  {
    final CountDownLatch initialized = new CountDownLatch(1);
    onMain(() -> {
      try
      {
        mApplication.initOrganicMaps();
        mApplication.getMapsReadiness().runWhenReady(initialized::countDown);
      }
      catch (IOException e)
      {
        throw new UncheckedIOException(e);
      }
    });
    assertTrue("Core initialization did not complete", initialized.await(30, TimeUnit.SECONDS));
    final DataChangedListener singletonBookmarksListener = getBookmarksListener(RoutingController.get());
    onMain(() -> {
      BookmarkManager.INSTANCE.removeCategoriesUpdatesListener(singletonBookmarksListener);
      mOriginalRouter = Router.getLastUsed();
      Framework.nativeCloseRouting();
      Framework.nativeDeleteSavedRoutePoints();
      mController = new RoutingController();
      mController.initialize(mApplication.getLocationHelper());
      mController.setRouterType(Router.Ruler);
    });
  }

  @After
  public void clearRoute() throws Exception
  {
    if (mController == null)
      return;
    final DataChangedListener bookmarksListener = getBookmarksListener(mController);
    onMain(() -> {
      if (mContainer != null)
        mController.detach(mContainer);
      mController.cancel();
      Framework.nativeCloseRouting();
      Framework.nativeDeleteSavedRoutePoints();
      Router.set(mOriginalRouter);
      BookmarkManager.INSTANCE.removeCategoriesUpdatesListener(bookmarksListener);
      RoutingController.get().initialize(mApplication.getLocationHelper());
    });
  }

  @Test
  public void idlePreparationReplacesOldPointsAndDiscardsRecovery() throws Exception
  {
    onMain(() -> {
      saveRouteWithStop();
      assertTrue(Framework.nativeHasSavedRoutePoints());
      mController.prepare(point("New start", 3), point("New finish", 4), Router.Ruler);
      assertTrue(mController.isPlanning());
      assertTrue(mController.isBuilding());
      assertEquals(Router.Ruler, mController.getLastRouterType());
      final RouteMarkData[] points = Framework.nativeGetRoutePoints();
      assertEquals(2, points.length);
      assertEquals("New start", points[0].mTitle);
      assertEquals("New finish", points[1].mTitle);
      assertFalse(Framework.nativeHasSavedRoutePoints());
    });
  }

  @Test
  public void restorationStartsBuildingWithoutAnAttachedView() throws Exception
  {
    onMain(() -> {
      saveRouteWithStop();
      Framework.nativeRemoveRoutePoints();
      mController.restoreRoute();
      assertTrue(mController.isPlanning());
      assertTrue(mController.isBuilding());
      assertEquals(3, Framework.nativeGetRoutePoints().length);
      assertFalse(Framework.nativeHasSavedRoutePoints());
      Framework.nativeSaveRoutePoints();
      mController.restoreRoute();
      assertTrue(Framework.nativeHasSavedRoutePoints());
      assertTrue(mController.isBuilding());
      final int[] shown = {0};
      mContainer = new RoutingController.Container() {
        @Override
        public void showRoutePlan(boolean show)
        {
          assertTrue(show);
          ++shown[0];
        }
      };
      mController.attach(mContainer);
      mController.restore();
      assertEquals(1, shown[0]);
      assertEquals(Router.Ruler, mController.getLastRouterType());
    });
  }

  @Test
  public void cancellationDiscardsRestoredRecoveryImmediately() throws Exception
  {
    onMain(() -> {
      saveRouteWithStop();
      Framework.nativeRemoveRoutePoints();
      mController.restoreRoute();
      assertTrue(mController.cancel());
      assertFalse(Framework.nativeHasSavedRoutePoints());
      assertFalse(Framework.nativeLoadRoutePoints());
      assertEquals(0, Framework.nativeGetRoutePoints().length);
    });
    mInstrumentation.waitForIdleSync();
    onMain(() -> {
      assertFalse(mController.isPlanning());
      assertEquals(0, Framework.nativeGetRoutePoints().length);
    });
  }

  @Test
  public void missingRecoveryLeavesTheControllerIdle() throws Exception
  {
    onMain(() -> {
      mController.restoreRoute();
      assertFalse(mController.isPlanning());
      assertFalse(mController.isNavigating());
      assertEquals(RoutingController.BuildState.NONE, mController.getBuildState());
      assertEquals(0, Framework.nativeGetRoutePoints().length);
    });
  }

  @Test
  public void emptyPlanningHasNoActiveNativeItinerary() throws Exception
  {
    onMain(() -> {
      mController.prepare(null, null, Router.Ruler);
      assertTrue(mController.isPlanning());
      assertFalse(Framework.nativeIsRoutingActive());
      assertEquals(0, Framework.nativeGetRoutePoints().length);
    });
  }

  @Test
  public void validationFailureKeepsTheNewItineraryInPlanning() throws Exception
  {
    onMain(() -> {
      saveRouteWithStop();
      mContainer = new RoutingController.Container() {};
      mController.attach(mContainer);
      mController.prepare(point("New start", 3), point("New finish", 3), Router.Ruler);
      assertTrue(mController.isPlanning());
      assertEquals(RoutingController.BuildState.NONE, mController.getBuildState());
      assertEquals(Router.Ruler, mController.getLastRouterType());
      assertEquals(2, Framework.nativeGetRoutePoints().length);
      assertFalse(Framework.nativeHasSavedRoutePoints());
    });
  }

  @Test
  public void detachedValidationResultDoesNotReplaceANewBuild() throws Exception
  {
    onMain(() -> {
      mController.prepare(point("Invalid start", 3), point("Invalid finish", 3), Router.Ruler);
      mController.setEndPoint(point("New finish", 4));
      mContainer = new RoutingController.Container() {};
      mController.attach(mContainer);
      mController.restore();
      assertTrue(mController.isBuilding());
    });
  }

  @Test
  public void cancelledErrorDoesNotBecomeTheResultOfANewEmptyPlan() throws Exception
  {
    final Field listener = RoutingController.class.getDeclaredField("mRoutingListener");
    listener.setAccessible(true);
    final RoutingListener events = (RoutingListener) listener.get(mController);
    onMain(() -> {
      mController.prepare(point("Start", 1), point("Finish", 2), Router.Ruler);
      events.onRoutingEvent(ResultCodes.ROUTE_NOT_FOUND, new String[0]);
      assertTrue(mController.cancel());
      mController.prepare(null, null, Router.Ruler);
      mContainer = new RoutingController.Container() {};
      mController.attach(mContainer);
      mController.restore();
      assertEquals(RoutingController.BuildState.NONE, mController.getBuildState());
    });
  }

  @Test
  public void savedWaterTrackIsViewOnlyAndOrdinaryTrackUsesItsFollowProfile() throws Exception
  {
    try (ImportedTrack water = new ImportedTrack("water", ImportedTrack.DEFAULT_COORDINATES))
    {
      onMain(() -> {
        assertEquals(FollowTrackResult.UnsupportedProfile, mController.navigateTrack(water.id));
        assertFalse(mController.isFollowingTrack());
      });
    }
    try (ImportedTrack ordinary = new ImportedTrack("pedestrian", ImportedTrack.DEFAULT_COORDINATES))
    {
      onMain(() -> {
        mController.setRouterType(Router.Water);
        assertEquals(FollowTrackResult.Ok, mController.navigateTrack(ordinary.id));
        assertEquals(Router.Pedestrian, mController.getLastRouterType());
        assertFalse(mController.isWaterRouterType());
      });
    }
  }

  @Test
  public void acceptedTrackBuildReplacesTheSavedHostsCancelledResult() throws Exception
  {
    try (ImportedTrack track = new ImportedTrack())
    {
      onMain(() -> {
        mContainer = new RoutingController.Container() {};
        mController.attach(mContainer);
        mController.onSaveState(mContainer);
        mController.prepare(point("Start", 3), point("Finish", 3), Router.Ruler);
        mController.detach(mContainer);
        assertEquals(FollowTrackResult.Ok, mController.navigateTrack(track.id));
        mController.attach(mContainer);
        mController.restore();
        assertTrue(mController.isPlanning());
        assertTrue(mController.isBuilding());
        assertEquals("Trail 🏔", mController.getFollowedTrackInfo().getName());
        assertEquals(0.0, mController.getFollowedTrackInfo().getEntryDistanceMeters(), 0.0);
      });
      await("Track preview did not finish building", mController::isBuilt);
      onMain(() -> assertTrue(Framework.nativeIsRouteBuilt()));
    }
  }

  @Test
  public void movingTheEntryToEitherEndpointUsesItsOnlyDirection() throws Exception
  {
    try (ImportedTrack track = new ImportedTrack())
    {
      onMain(() -> assertEquals(FollowTrackResult.Ok, mController.navigateTrack(track.id)));
      await("Track preview did not finish building", mController::isBuilt);
      onMain(() -> assertEquals(FollowTrackResult.Ok, mController.updateFollowedTrackEntryAt(400.0)));
      await("Entry update did not finish building", mController::isBuilt);
      onMain(() -> {
        BookmarkManager.INSTANCE.setElevationActivePoint(track.id, 800.0);
        assertEquals(FollowTrackResult.Ok, mController.setFollowTrackProfile(Router.Vehicle));
      });
      await("Profile update did not finish building", mController::isBuilt);
      onMain(() -> {
        assertEquals(400.0, mController.getFollowedTrackInfo().getEntryDistanceMeters(), 0.0);
        assertEquals(FollowTrackResult.Ok, mController.updateFollowedTrackEntryAt(2000.0));
        assertTrue(mController.isBuilding());
      });
      await("Backward preview did not finish building", mController::isBuilt);
      onMain(() -> {
        assertTrue(mController.getFollowedTrackInfo().isReversed());
        assertFalse(mController.getFollowedTrackInfo().canReverse());
        assertEquals(FollowTrackResult.Ok, mController.updateFollowedTrackEntryAt(0.0));
      });
      await("Forward preview did not finish building", mController::isBuilt);
      onMain(() -> {
        assertFalse(mController.getFollowedTrackInfo().isReversed());
        assertEquals(0.0, mController.getFollowedTrackInfo().getEntryDistanceMeters(), 0.0);
      });
    }
  }

  @Test
  public void openingATrackPreviewAnnouncesItOnce() throws Exception
  {
    try (ImportedTrack track = new ImportedTrack())
    {
      onMain(() -> {
        final List<String> events = new ArrayList<>();
        mContainer = new RoutingController.Container() {
          @Override
          public void showRoutePlan(boolean show)
          {
            events.add("plan " + show);
          }

          @Override
          public void updateBuildProgress(int progress, Router router)
          {
            events.add("progress " + progress);
          }

          @Override
          public void onPlanningStarted()
          {
            events.add("planning started");
          }
        };
        mController.attach(mContainer);
        assertEquals(FollowTrackResult.Ok, mController.navigateTrack(track.id));
        assertEquals(List.of("plan true", "progress 0", "planning started"), events);
      });
    }
  }

  @Test
  public void plannerReversesOrdinaryRoutePoints() throws Exception
  {
    try (ActivityScenario<MwmActivity> scenario = launchMap())
    {
      onMain(() -> mController.prepare(point("Start", 1), point("Finish", 2), Router.Ruler));
      await("Route did not finish building", mController::isBuilt);
      scenario.onActivity(activity -> {
        final View reverse = activity.findViewById(R.id.reverse_route_btn);
        assertTrue(reverse.isShown());
        assertTrue(reverse.isEnabled());
        assertTrue(reverse.performClick());
        final RouteMarkData[] points = Framework.nativeGetRoutePoints();
        assertEquals("Finish", points[0].mTitle);
        assertEquals("Start", points[1].mTitle);
      });
    }
  }

  @Test
  public void plannerReversesAReadyTrackAtItsSelectedEntry() throws Exception
  {
    try (ImportedTrack track = new ImportedTrack(); ActivityScenario<MwmActivity> scenario = launchMap())
    {
      onMain(() -> assertEquals(FollowTrackResult.Ok, mController.navigateTrack(track.id)));
      await("Track preview did not finish building", mController::isBuilt);
      scenario.onActivity(activity -> {
        final View reverse = activity.findViewById(R.id.reverse_route_btn);
        assertTrue(reverse.isShown());
        assertFalse(reverse.isEnabled());
        assertEquals(activity.getString(R.string.navigate_track_finish), ViewCompat.getStateDescription(reverse));
        assertEquals(FollowTrackResult.NoRemainingGeometry, mController.reverseFollowedTrack());
        assertTrue(mController.isPlanning());
        assertTrue(mController.isBuilt());
        assertFalse(mController.getFollowedTrackInfo().isReversed());
        assertEquals(FollowTrackResult.Ok, mController.updateFollowedTrackEntryAt(400.0));
      });
      await("Entry update did not finish building", mController::isBuilt);
      scenario.onActivity(activity -> {
        final View reverse = activity.findViewById(R.id.reverse_route_btn);
        assertTrue(reverse.isShown());
        assertTrue(reverse.isEnabled());
        assertTrue(reverse.performClick());
        assertTrue(mController.isBuilding());
        assertFalse(reverse.isEnabled());
        assertEquals(FollowTrackResult.Busy, mController.reverseFollowedTrack());
      });
      await("Reversed preview did not finish building", mController::isBuilt);
      scenario.onActivity(activity -> {
        final View reverse = activity.findViewById(R.id.reverse_route_btn);
        assertTrue(reverse.isEnabled());
        assertEquals(activity.getString(R.string.navigate_track_start), ViewCompat.getStateDescription(reverse));
        assertEquals(400.0, mController.getFollowedTrackInfo().getEntryDistanceMeters(), 0.0);
      });
    }
  }

  @Test
  public void reversingAClosedTrackMovesTheChartMarkerToItsFinish() throws Exception
  {
    try (ImportedTrack track = new ImportedTrack("", "0,51.47,100 0,51.48,120 0.01,51.48,150 0,51.470049,100");
         ActivityScenario<MwmActivity> scenario = launchMap())
    {
      onMain(() -> {
        BookmarkManager.INSTANCE.setElevationActivePoint(track.id, 3.0);
        assertEquals(FollowTrackResult.Ok, mController.navigateTrack(track.id));
      });
      await("Loop preview did not finish building", mController::isBuilt);
      scenario.onActivity(activity -> {
        final View reverse = activity.findViewById(R.id.reverse_route_btn);
        assertTrue(reverse.isEnabled());
        assertTrue(reverse.performClick());
      });
      await("Reversed loop did not finish building", mController::isBuilt);
      scenario.onActivity(activity -> {
        final ElevationProfileChart chart = activity.findViewById(R.id.elevation_profile_chart);
        assertTrue(mController.getFollowedTrackInfo().isReversed());
        assertEquals(chart.getXChartMax(), chart.getHighlightX(), 0f);
      });
    }
  }

  @Test
  public void deletingTheTrackCategoryEndsItsPreview() throws Exception
  {
    try (ImportedTrack track = new ImportedTrack())
    {
      onMain(() -> assertEquals(FollowTrackResult.Ok, mController.navigateTrack(track.id)));
      await("Track preview did not finish building", mController::isBuilt);
      onMain(track::deleteCategory);
      await("Deleting the track did not end its preview", () -> !mController.isPlanning());
    }
  }

  @Test
  public void deletingTheTrackEndsItsNavigation() throws Exception
  {
    try (ImportedTrack track = new ImportedTrack(); ActivityScenario<MwmActivity> ignored = launchMap())
    {
      startTrackNavigation(track);
      onMain(() -> BookmarkManager.INSTANCE.deleteTrack(track.id));
      await("Deleting the track did not end its navigation", () -> !mController.isNavigating());
      onMain(() -> {
        assertFalse(mController.isPlanning());
        assertFalse(mController.isFollowingTrack());
        assertFalse(Framework.nativeIsRoutingFollowing());
      });
    }
  }

  @Test
  public void deletingTheTrackListWhileTheMapIsStoppedEndsItsNavigation() throws Exception
  {
    try (ImportedTrack track = new ImportedTrack(); ActivityScenario<MwmActivity> scenario = launchMap())
    {
      startTrackNavigation(track);
      scenario.moveToState(Lifecycle.State.CREATED);
      onMain(track::deleteCategory);
      await("Deleting the track list did not end its navigation", () -> !mController.isNavigating());
      onMain(() -> assertFalse(Framework.nativeIsRoutingFollowing()));

      scenario.moveToState(Lifecycle.State.RESUMED);
      scenario.onActivity(activity -> {
        final MapButtonsViewModel buttons = new ViewModelProvider(activity).get(MapButtonsViewModel.class);
        assertEquals(MapButtonsController.LayoutMode.regular, buttons.getLayoutMode().getValue());
        assertNull(activity.getSupportFragmentManager().findFragmentByTag(RoutingPlanFragment.TAG));
      });
    }
  }

  @Test
  public void configurationRecreationKeepsRouteRecovery() throws Exception
  {
    // A location permission dialog would pause the Activity during recreation. Below API 23 the permissions are
    // granted at install time; above, the requester grants them with pm grant.
    final PermissionRequester permissions = new PermissionRequester();
    permissions.addPermissions(ACCESS_COARSE_LOCATION, ACCESS_FINE_LOCATION);
    permissions.requestPermissions();
    useTheAppController();
    try (ActivityScenario<MwmActivity> scenario = ActivityScenario.launch(MwmActivity.class))
    {
      final AtomicReference<MwmActivity> original = new AtomicReference<>();
      scenario.onActivity(activity -> {
        original.set(activity);
        mController.prepare(point("Start", 1), point("Finish", 2), Router.Ruler);
      });
      await("Route did not finish building", mController::isBuilt);
      onMain(() -> {
        mController.saveRoute();
        assertTrue(Framework.nativeHasSavedRoutePoints());
      });

      scenario.recreate();

      onMain(() -> {
        assertTrue(original.get().isChangingConfigurations());
        assertTrue(mController.isPlanning());
        assertTrue(Framework.nativeHasSavedRoutePoints());
      });
    }
  }

  private static void saveRouteWithStop()
  {
    assertTrue(Framework.nativeAddRoutePoint("Saved start", "", RouteMarkType.Start, false, 1, 1, false));
    assertTrue(Framework.nativeAddRoutePoint("Saved stop", "", RouteMarkType.Intermediate, false, 1.5, 1.5, false));
    assertTrue(Framework.nativeAddRoutePoint("Saved finish", "", RouteMarkType.Finish, false, 2, 2, false));
    Framework.nativeSaveRoutePoints();
  }

  private static MapObject point(String name, double coordinate)
  {
    return MapObject.createMapObject(MapObject.API_POINT, name, "", coordinate, coordinate);
  }

  // The map activity attaches to the app's controller.
  private void useTheAppController() throws Exception
  {
    final DataChangedListener bookmarksListener = getBookmarksListener(mController);
    onMain(() -> {
      BookmarkManager.INSTANCE.removeCategoriesUpdatesListener(bookmarksListener);
      mController = RoutingController.get();
      mController.initialize(mApplication.getLocationHelper());
    });
  }

  // Following a route needs the map engine and, without a permission dialog, keeps the activity resumed.
  private ActivityScenario<MwmActivity> launchMap() throws Exception
  {
    final PermissionRequester permissions = new PermissionRequester();
    permissions.addPermissions(ACCESS_COARSE_LOCATION, ACCESS_FINE_LOCATION);
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU)
      permissions.addPermissions(POST_NOTIFICATIONS);
    permissions.requestPermissions();
    useTheAppController();
    final ActivityScenario<MwmActivity> scenario = ActivityScenario.launch(MwmActivity.class);
    await("The map engine was not created", Map::isEngineCreated);
    await("Location did not start", () -> mApplication.getLocationHelper().isActive());
    return scenario;
  }

  // The core starts following from its latest fix: put it at the track start to follow at once. The track lies on
  // land, where navigation can read the speed limit signs of a country.
  private void startTrackNavigation(ImportedTrack track) throws Exception
  {
    onMain(() -> {
      final Location fix = new Location(LocationManager.GPS_PROVIDER);
      fix.setLatitude(51.47);
      fix.setLongitude(0);
      fix.setAccuracy(1);
      fix.setTime(System.currentTimeMillis());
      fix.setElapsedRealtimeNanos(SystemClock.elapsedRealtimeNanos());
      mApplication.getLocationHelper().onLocationChanged(fix);
      assertEquals(FollowTrackResult.Ok, mController.navigateTrack(track.id));
    });
    await("Track preview did not finish building", mController::isBuilt);
    onMain(mController::start);
    await("Track navigation did not start", mController::isNavigating);
    onMain(() -> assertTrue(Framework.nativeIsRoutingFollowing()));
  }

  private static DataChangedListener getBookmarksListener(RoutingController controller)
      throws ReflectiveOperationException
  {
    final Field listener = RoutingController.class.getDeclaredField("mBookmarksListener");
    listener.setAccessible(true);
    return (DataChangedListener) listener.get(controller);
  }

  private void onMain(Runnable action) throws Exception
  {
    final FutureTask<Void> task = new FutureTask<>(action, null);
    mInstrumentation.runOnMainSync(task);
    task.get();
  }

  private void await(String message, BooleanSupplier condition) throws Exception
  {
    final long deadline = SystemClock.uptimeMillis() + TimeUnit.SECONDS.toMillis(30);
    while (true)
    {
      final FutureTask<Boolean> task = new FutureTask<>(condition::getAsBoolean);
      mInstrumentation.runOnMainSync(task);
      if (task.get())
        return;
      assertTrue(message, SystemClock.uptimeMillis() < deadline);
      Thread.sleep(10);
    }
  }

  private final class ImportedTrack implements AutoCloseable
  {
    private final File source = File.createTempFile("track-preview-", ".kml", mApplication.getCacheDir());
    final long id;

    private static final String DEFAULT_COORDINATES = "0,51.47,100 0,51.48,200";

    ImportedTrack() throws Exception
    {
      this("", DEFAULT_COORDINATES);
    }

    ImportedTrack(String profile, String coordinates) throws Exception
    {
      try (FileWriter writer = new FileWriter(source))
      {
        writer.write(String.format("""
            <kml xmlns="http://www.opengis.net/kml/2.2"><Document>
            <name>%s</name><Placemark><name>Trail 🏔</name>
            <LineString><coordinates>%s</coordinates></LineString>
            %s</Placemark></Document></kml>
            """, source.getName(), coordinates,
                                   profile.isEmpty() ? ""
                                                     : "<ExtendedData xmlns:mwm=\"https://omaps.app\"><mwm:routerType>"
                                                           + profile + "</mwm:routerType></ExtendedData>"));
      }
      onMain(() -> BookmarkManager.INSTANCE.loadBookmarksFile(source.getAbsolutePath(), false));
      await("Track was not imported", () -> category() != null && category().getTracksCount() == 1);
      final FutureTask<Long> selected = new FutureTask<>(() -> category().getTrackIdByPosition(0));
      mInstrumentation.runOnMainSync(selected);
      id = selected.get();
      onMain(() -> BookmarkManager.INSTANCE.setElevationActivePoint(id, 0.0));
    }

    private BookmarkCategory category()
    {
      return BookmarkManager.INSTANCE.getCategories()
          .stream()
          .filter(item -> item.getName().equals(source.getName()))
          .findFirst()
          .orElse(null);
    }

    // Runs on the main thread.
    void deleteCategory()
    {
      final BookmarkCategory category = category();
      if (category != null)
        BookmarkManager.INSTANCE.deleteCategory(category.getId());
    }

    @Override
    public void close() throws Exception
    {
      onMain(() -> {
        mController.cancel();
        deleteCategory();
      });
      source.delete();
    }
  }
}
