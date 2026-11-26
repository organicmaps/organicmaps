package app.organicmaps.car.screens.permissions;

import static android.Manifest.permission.ACCESS_COARSE_LOCATION;
import static android.Manifest.permission.ACCESS_FINE_LOCATION;

import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import androidx.car.app.model.Action;
import androidx.car.app.model.CarIcon;
import androidx.car.app.model.Header;
import androidx.car.app.model.MessageTemplate;
import androidx.car.app.model.ParkedOnlyOnClickListener;
import androidx.car.app.model.Template;
import androidx.core.graphics.drawable.IconCompat;
import androidx.lifecycle.Lifecycle;
import androidx.lifecycle.LifecycleOwner;
import app.organicmaps.car.R;
import app.organicmaps.car.screens.ErrorScreen;
import app.organicmaps.car.util.Colors;
import app.organicmaps.sdk.OrganicMaps;
import app.organicmaps.sdk.location.LocationUtils;
import java.util.Arrays;
import java.util.List;

public class RequestPermissionsScreenWithApi extends RequestPermissionsScreen
{
  private static final List<String> LOCATION_PERMISSIONS = Arrays.asList(ACCESS_COARSE_LOCATION, ACCESS_FINE_LOCATION);

  private boolean mPermissionResultPending;

  public RequestPermissionsScreenWithApi(@NonNull CarContext carContext, @NonNull OrganicMaps organicMapsContext,
                                         @NonNull Runnable permissionsGrantedCallback)
  {
    super(carContext, organicMapsContext, permissionsGrantedCallback);
  }

  @NonNull
  @Override
  protected Template onGetTemplateImpl()
  {
    final MessageTemplate.Builder builder =
        new MessageTemplate.Builder(getCarContext().getString(R.string.aa_request_permission_activity_text));
    final Action grantPermissions =
        new Action.Builder()
            .setTitle(getCarContext().getString(R.string.aa_grant_permissions))
            .setBackgroundColor(Colors.BUTTON_ACCEPT)
            .setOnClickListener(ParkedOnlyOnClickListener.create(
                () -> getCarContext().requestPermissions(LOCATION_PERMISSIONS, this::onRequestPermissionsResult)))
            .build();

    final Header.Builder headerBuilder = new Header.Builder();
    headerBuilder.setStartHeaderAction(Action.APP_ICON);
    headerBuilder.setTitle(getCarContext().getString(app.organicmaps.branding.R.string.app_name));

    builder.setHeader(headerBuilder.build());
    builder.setIcon(
        new CarIcon.Builder(IconCompat.createWithResource(getCarContext(), R.drawable.ic_location_off)).build());
    builder.addAction(grantPermissions);
    builder.addAction(closeAction());
    return builder.build();
  }

  @Override
  public void onResume(@NonNull LifecycleOwner owner)
  {
    super.onResume(owner);
    completePermissionRequest();
  }

  private void onRequestPermissionsResult(@NonNull List<String> grantedPermissions,
                                          @NonNull List<String> rejectedPermissions)
  {
    if (getLifecycle().getCurrentState() == Lifecycle.State.DESTROYED)
      return;
    mPermissionResultPending = true;
    // Automotive can return the result before its permission activity uncovers the car screen.
    if (getLifecycle().getCurrentState().isAtLeast(Lifecycle.State.RESUMED))
      completePermissionRequest();
  }

  private void completePermissionRequest()
  {
    if (finishIfGranted() || !mPermissionResultPending)
      return;
    mPermissionResultPending = false;
    final int message = LocationUtils.checkLocationPermission(getCarContext())
                          ? R.string.precise_location_is_disabled_long_text
                          : R.string.location_is_disabled_long_text;
    getScreenManager().push(new ErrorScreen.Builder(getCarContext(), getOrganicMapsContext())
                                .setErrorMessage(message)
                                .setNegativeButton(R.string.close, null)
                                .build());
  }
}
