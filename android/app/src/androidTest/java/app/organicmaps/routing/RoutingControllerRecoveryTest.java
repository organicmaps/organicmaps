package app.organicmaps.routing;

import static android.Manifest.permission.ACCESS_COARSE_LOCATION;
import static android.Manifest.permission.ACCESS_FINE_LOCATION;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.app.Instrumentation;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.SystemClock;
import androidx.test.core.app.ActivityScenario;
import androidx.test.platform.app.InstrumentationRegistry;
import app.organicmaps.MwmActivity;
import app.organicmaps.MwmApplication;
import app.organicmaps.sdk.Framework;
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
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.io.UncheckedIOException;
import java.lang.reflect.Field;
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
        if (!mApplication.initOrganicMaps(initialized::countDown))
          initialized.countDown();
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
  public void inspectedAndRefusedSelectionsPreserveTheCapturedEntry() throws Exception
  {
    try (ImportedTrack track = new ImportedTrack())
    {
      onMain(() -> assertEquals(FollowTrackResult.Ok, mController.navigateTrack(track.id)));
      await("Track preview did not finish building", mController::isBuilt);
      onMain(() -> assertEquals(FollowTrackResult.Ok, mController.updateFollowedTrackEntryAt(400.0)));
      await("Entry update did not finish building", mController::isBuilt);
      onMain(() -> {
        BookmarkManager.INSTANCE.setElevationActivePoint(track.id, 800.0);
        assertEquals(FollowTrackResult.NoRemainingGeometry, mController.updateFollowedTrackEntryAt(2000.0));
        assertEquals(400.0, mController.getFollowedTrackInfo().getEntryDistanceMeters(), 0.0);
        assertTrue(mController.isBuilt());
        assertEquals(FollowTrackResult.Ok, mController.setFollowTrackProfile(Router.Vehicle));
      });
      await("Profile update did not finish building", mController::isBuilt);
      onMain(() -> {
        assertEquals(400.0, mController.getFollowedTrackInfo().getEntryDistanceMeters(), 0.0);
        assertEquals(FollowTrackResult.Ok, mController.updateFollowedTrackEntryAt(-100.0));
        assertEquals(0.0, mController.getFollowedTrackInfo().getEntryDistanceMeters(), 0.0);
      });
    }
  }

  @Test
  public void trackReversalKeepsTheControllerInSyncWithCore() throws Exception
  {
    try (ImportedTrack track = new ImportedTrack())
    {
      onMain(() -> assertEquals(FollowTrackResult.Ok, mController.navigateTrack(track.id)));
      await("Track preview did not finish building", mController::isBuilt);
      onMain(() -> {
        assertEquals(FollowTrackResult.NoRemainingGeometry, mController.reverseFollowedTrack());
        assertTrue(mController.isPlanning());
        assertTrue(mController.isBuilt());
        assertFalse(mController.getFollowedTrackInfo().isReversed());
        assertEquals(FollowTrackResult.Ok, mController.updateFollowedTrackEntryAt(400.0));
      });
      await("Entry update did not finish building", mController::isBuilt);
      onMain(() -> {
        assertEquals(FollowTrackResult.Ok, mController.reverseFollowedTrack());
        assertTrue(mController.isBuilding());
        assertEquals(FollowTrackResult.Busy, mController.reverseFollowedTrack());
      });
      await("Reversed preview did not finish building", mController::isBuilt);
      onMain(() -> {
        assertTrue(mController.getFollowedTrackInfo().isReversed());
        assertEquals(400.0, mController.getFollowedTrackInfo().getEntryDistanceMeters(), 0.0);
      });
    }
  }

  @Test
  public void configurationRecreationKeepsRouteRecovery() throws Exception
  {
    // A permission dialog pauses the activity and prevents ActivityScenario from recreating it.
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M)
      for (String permission : new String[] {ACCESS_COARSE_LOCATION, ACCESS_FINE_LOCATION})
        if (mApplication.checkSelfPermission(permission) != PackageManager.PERMISSION_GRANTED)
          mInstrumentation.getUiAutomation().grantRuntimePermission(mApplication.getPackageName(), permission);

    final DataChangedListener bookmarksListener = getBookmarksListener(mController);
    onMain(() -> {
      BookmarkManager.INSTANCE.removeCategoriesUpdatesListener(bookmarksListener);
      mController = RoutingController.get();
      mController.initialize(mApplication.getLocationHelper());
    });
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

    ImportedTrack() throws Exception
    {
      try (FileWriter writer = new FileWriter(source))
      {
        writer.write(String.format("""
            <kml xmlns="http://www.opengis.net/kml/2.2"><Document>
            <name>%s</name><Placemark><name>Trail 🏔</name>
            <LineString><coordinates>0,0,100 0.01,0,200</coordinates></LineString>
            </Placemark></Document></kml>
            """, source.getName()));
      }
      onMain(() -> BookmarkManager.INSTANCE.loadBookmarksFile(source.getAbsolutePath(), false));
      await("Track was not imported", () -> category() != null);
      final FutureTask<Long> selected = new FutureTask<>(() -> category().getTrackIdByPosition(0));
      mInstrumentation.runOnMainSync(selected);
      id = selected.get();
      onMain(() -> BookmarkManager.INSTANCE.setElevationActivePoint(id, 0.0));
    }

    private BookmarkCategory category()
    {
      return BookmarkManager.INSTANCE.getCategories()
          .stream()
          .filter(item -> item.getName().equals(source.getName()) && item.getTracksCount() == 1)
          .findFirst()
          .orElse(null);
    }

    @Override
    public void close() throws Exception
    {
      onMain(() -> {
        mController.cancel();
        final BookmarkCategory category = category();
        if (category != null)
          BookmarkManager.INSTANCE.deleteCategory(category.getId());
      });
      source.delete();
    }
  }
}
