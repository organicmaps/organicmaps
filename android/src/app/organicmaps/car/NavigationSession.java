package app.organicmaps.car;

import android.content.Intent;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.car.app.CarToast;
import androidx.car.app.Screen;
import androidx.car.app.ScreenManager;
import androidx.car.app.Session;
import androidx.car.app.model.Action;
import androidx.car.app.model.ActionStrip;
import androidx.car.app.model.CarIcon;
import androidx.car.app.navigation.model.MapController;
import androidx.core.graphics.drawable.IconCompat;
import androidx.lifecycle.DefaultLifecycleObserver;
import androidx.lifecycle.LifecycleOwner;

import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.car.screens.ErrorScreen;
import app.organicmaps.car.screens.NavigationScreen;
import app.organicmaps.car.screens.settings.SettingsScreen;

import java.io.IOException;

public final class NavigationSession extends Session implements DefaultLifecycleObserver
{
  private static final String TAG = NavigationSession.class.getSimpleName();


  private OMController mMapController;

  boolean mInitFailed = false;

  public NavigationSession()
  {
    getLifecycle().addObserver(this);
  }

  @NonNull
  @Override
  public Screen onCreateScreen(@NonNull Intent intent)
  {
    Log.d(TAG, "onCreateScreen()");
    if (mInitFailed)
      return new ErrorScreen(getCarContext());

    createMapController();
    return new NavigationScreen(getCarContext(), mMapController);
  }

  @Override
  public void onCreate(@NonNull LifecycleOwner owner)
  {
    Log.d(TAG, "onCreate()");
    init();
  }

  @Override
  public void onResume(@NonNull LifecycleOwner owner)
  {
    Log.d(TAG, "onResume()");
    init();
  }

  private void init()
  {
    mInitFailed = false;
    MwmApplication app = MwmApplication.from(getCarContext());
    try
    {
      app.init();
    } catch (IOException e)
    {
      mInitFailed = true;
      Log.e(TAG, "Failed to initialize the app.");
    }
  }

  private void createMapController()
  {
    final CarIcon iconPlus = new CarIcon.Builder(IconCompat.createWithResource(getCarContext(), R.drawable.ic_plus)).build();
    final CarIcon iconMinus = new CarIcon.Builder(IconCompat.createWithResource(getCarContext(), R.drawable.ic_minus)).build();
    final CarIcon iconLocation = new CarIcon.Builder(IconCompat.createWithResource(getCarContext(), R.drawable.ic_not_follow)).build();
    final CarIcon iconSettings = new CarIcon.Builder(IconCompat.createWithResource(getCarContext(), R.drawable.ic_settings)).build();

    SurfaceRenderer surfaceRenderer = new SurfaceRenderer(getCarContext(), getLifecycle());

    Action panAction = new Action.Builder(Action.PAN).build();
    Action location = new Action.Builder().setIcon(iconLocation).setOnClickListener(this::location).build();
    Action zoomIn = new Action.Builder().setIcon(iconPlus).setOnClickListener(this::zoomIn).build();
    Action zoomOut = new Action.Builder().setIcon(iconMinus).setOnClickListener(this::zoomOut).build();
    ActionStrip mapActionStrip = new ActionStrip.Builder().addAction(location).addAction(zoomIn).addAction(zoomOut).addAction(panAction).build();
    MapController mapController = new MapController.Builder().setMapActionStrip(mapActionStrip).build();

    Action settings = new Action.Builder().setIcon(iconSettings).setOnClickListener(this::openSettings).build();
    ActionStrip actionStrip = new ActionStrip.Builder().addAction(settings).build();

    mMapController = new OMController(surfaceRenderer, mapController, actionStrip);
  }

  private void location()
  {
    CarToast.makeText(getCarContext(), "Location", CarToast.LENGTH_LONG).show();
  }

  private void zoomOut()
  {
    mMapController.getSurfaceRenderer().onZoomOut();
  }

  private void zoomIn()
  {
    mMapController.getSurfaceRenderer().onZoomIn();
  }

  private void openSettings()
  {
    getCarContext().getCarService(ScreenManager.class).push(new SettingsScreen(getCarContext(), mMapController));
  }
}
