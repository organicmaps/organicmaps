package app.organicmaps.car.screens.permissions;

import androidx.activity.OnBackPressedCallback;
import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import androidx.car.app.model.Action;
import app.organicmaps.car.R;
import app.organicmaps.car.util.UserActionRequired;
import app.organicmaps.sdk.OrganicMaps;
import app.organicmaps.sdk.car.screens.BaseScreen;
import app.organicmaps.sdk.location.LocationUtils;

public abstract class RequestPermissionsScreen extends BaseScreen implements UserActionRequired
{
  @NonNull
  private final Runnable mPermissionsGranted;

  protected RequestPermissionsScreen(@NonNull CarContext context, @NonNull OrganicMaps sdk,
                                     @NonNull Runnable permissionsGranted)
  {
    super(context, sdk);
    mPermissionsGranted = permissionsGranted;
    context.getOnBackPressedDispatcher().addCallback(this, new OnBackPressedCallback(true) {
      @Override
      public void handleOnBackPressed()
      {
        // Back would expose the map without precise location. The driver can leave with Close.
      }
    });
  }

  @NonNull
  protected final Action closeAction()
  {
    return new Action.Builder()
        .setTitle(getCarContext().getString(R.string.close))
        .setOnClickListener(getCarContext()::finishCarApp)
        .build();
  }

  protected final boolean finishIfGranted()
  {
    if (!LocationUtils.checkFineLocationPermission(getCarContext()))
      return false;
    mPermissionsGranted.run();
    finish();
    return true;
  }
}
