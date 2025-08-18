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
import androidx.lifecycle.LifecycleOwner;
import app.organicmaps.R;
import app.organicmaps.car.screens.ErrorScreen;
import app.organicmaps.car.screens.base.BaseScreen;
import app.organicmaps.car.util.Colors;
import app.organicmaps.car.util.UserActionRequired;
import app.organicmaps.sdk.util.LocationUtils;
import java.util.Arrays;
import java.util.List;

public class RequestPermissionsScreenWithApi extends BaseScreen implements UserActionRequired
{
  private static final List<String> LOCATION_PERMISSIONS = Arrays.asList(ACCESS_COARSE_LOCATION, ACCESS_FINE_LOCATION);

  @NonNull
  private final Runnable mPermissionsGrantedCallback;

  public RequestPermissionsScreenWithApi(@NonNull CarContext carContext, @NonNull Runnable permissionsGrantedCallback)
  {
    super(carContext);
    mPermissionsGrantedCallback = permissionsGrantedCallback;
  }

  @NonNull
  @Override
  public Template onGetTemplate()
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
    headerBuilder.setTitle(getCarContext().getString(R.string.app_name));

    builder.setHeader(headerBuilder.build());
    builder.setIcon(
        new CarIcon.Builder(IconCompat.createWithResource(getCarContext(), R.drawable.ic_location_off)).build());
    builder.addAction(grantPermissions);
    return builder.build();
  }

  @Override
  public void onResume(@NonNull LifecycleOwner owner)
  {
    // Let's review the permissions once more, as we might enter this function following an ErrorScreen situation
    // where the user manually enabled location permissions.
    if (LocationUtils.checkFineLocationPermission(getCarContext()))
    {
      mPermissionsGrantedCallback.run();
      finish();
    }
  }

  private void onRequestPermissionsResult(@NonNull List<String> grantedPermissions,
                                          @NonNull List<String> rejectedPermissions)
  {
    if (grantedPermissions.isEmpty())
    {
      getScreenManager().push(new ErrorScreen.Builder(getCarContext())
                                  .setErrorMessage(R.string.location_is_disabled_long_text)
                                  .setNegativeButton(R.string.close, null)
                                  .build());
      return;
    }

    mPermissionsGrantedCallback.run();
    finish();
  }
}
