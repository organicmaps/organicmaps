package app.organicmaps.car;

import android.util.Log;

import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import androidx.car.app.CarToast;
import androidx.car.app.Screen;
import androidx.car.app.model.Action;
import androidx.car.app.model.ActionStrip;
import androidx.car.app.model.CarIcon;
import androidx.car.app.model.Template;
import androidx.car.app.navigation.model.NavigationTemplate;
import androidx.core.graphics.drawable.IconCompat;

import app.organicmaps.R;
import app.organicmaps.location.LocationHelper;
import app.organicmaps.util.LocationUtils;

public class NavigationScreen extends Screen
{
  private static final String TAG = NavigationScreen.class.getSimpleName();

  @NonNull
  private final SurfaceRenderer mSurfaceRenderer;

  protected NavigationScreen(@NonNull CarContext carContext, @NonNull SurfaceRenderer surfaceRenderer)
  {
    super(carContext);
    mSurfaceRenderer = surfaceRenderer;
  }

  @NonNull
  @Override
  public Template onGetTemplate()
  {
    Log.d(TAG, "onGetTemplate");
    NavigationTemplate.Builder builder = new NavigationTemplate.Builder();
    ActionStrip.Builder actionStripBuilder = new ActionStrip.Builder();
    actionStripBuilder.addAction(new Action.Builder().setIcon(new CarIcon.Builder(IconCompat.createWithResource(getCarContext(), R.drawable.ic_settings)).build())
        .setOnClickListener(this::settings)
        .build());

    Action panAction = new Action.Builder(Action.PAN).build();
    Action location = new Action.Builder().setIcon(new CarIcon.Builder(IconCompat.createWithResource(getCarContext(), R.drawable.ic_not_follow)).build())
        .setOnClickListener(this::location)
        .build();
    Action zoomIn = new Action.Builder().setIcon(new CarIcon.Builder(IconCompat.createWithResource(getCarContext(), R.drawable.ic_plus)).build())
        .setOnClickListener(this::zoomIn)
        .build();
    Action zoomOut = new Action.Builder().setIcon(new CarIcon.Builder(IconCompat.createWithResource(getCarContext(), R.drawable.ic_minus)).build())
        .setOnClickListener(this::zoomOut)
        .build();

    ActionStrip mapActionStrip = new ActionStrip.Builder().addAction(location)
        .addAction(zoomIn)
        .addAction(zoomOut)
        .addAction(panAction)
        .build();
    builder.setMapActionStrip(mapActionStrip);
    builder.setActionStrip(actionStripBuilder.build());
    return builder.build();
  }

  private void location()
  {
    CarToast.makeText(getCarContext(), "Location", CarToast.LENGTH_LONG).show();
  }

  private void zoomOut()
  {
    mSurfaceRenderer.onZoomOut();
  }

  private void zoomIn()
  {
    mSurfaceRenderer.onZoomIn();
  }

  private void settings()
  {
    CarToast.makeText(getCarContext(), "Settings", CarToast.LENGTH_LONG).show();
  }
}
