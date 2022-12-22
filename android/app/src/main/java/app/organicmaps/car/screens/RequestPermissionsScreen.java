package app.organicmaps.car.screens;

import static android.Manifest.permission.ACCESS_COARSE_LOCATION;
import static android.Manifest.permission.ACCESS_FINE_LOCATION;

import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import androidx.car.app.model.Action;
import androidx.car.app.model.CarIcon;
import androidx.car.app.model.MessageTemplate;
import androidx.car.app.model.ParkedOnlyOnClickListener;
import androidx.car.app.model.Template;
import androidx.core.graphics.drawable.IconCompat;
import androidx.lifecycle.LifecycleOwner;

import app.organicmaps.R;
import app.organicmaps.car.screens.base.BaseScreen;
import app.organicmaps.car.util.Colors;
import app.organicmaps.util.LocationUtils;

import java.util.Arrays;
import java.util.List;

public class RequestPermissionsScreen extends BaseScreen
{
  public interface PermissionsGrantedCallback
  {
    void onPermissionsGranted();
  }

  private static final List<String> LOCATION_PERMISSIONS = Arrays.asList(ACCESS_COARSE_LOCATION, ACCESS_FINE_LOCATION);

  @NonNull
  private final PermissionsGrantedCallback mPermissionsGrantedCallback;

  public RequestPermissionsScreen(@NonNull CarContext carContext, @NonNull PermissionsGrantedCallback permissionsGrantedCallback)
  {
    super(carContext);
    mPermissionsGrantedCallback = permissionsGrantedCallback;
  }

  @NonNull
  @Override
  public Template onGetTemplate()
  {
    final MessageTemplate.Builder builder = new MessageTemplate.Builder(getCarContext().getString(R.string.aa_location_permissions_request));
    final Action grantPermissions = new Action.Builder()
        .setTitle(getCarContext().getString(R.string.aa_grant_permissions))
        .setBackgroundColor(Colors.BUTTON_ACCEPT)
        .setOnClickListener(ParkedOnlyOnClickListener.create(() -> getCarContext().requestPermissions(LOCATION_PERMISSIONS, this::onRequestPermissionsResult)))
        .build();

    builder.setHeaderAction(Action.APP_ICON);
    builder.setTitle(getCarContext().getString(R.string.app_name));
    builder.setIcon(new CarIcon.Builder(IconCompat.createWithResource(getCarContext(), R.drawable.ic_location_off)).build());
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
      mPermissionsGrantedCallback.onPermissionsGranted();
      finish();
    }
  }

  private void onRequestPermissionsResult(@NonNull List<String> grantedPermissions, @NonNull List<String> rejectedPermissions)
  {
    if (grantedPermissions.isEmpty())
    {
      getScreenManager().push(new ErrorScreen.Builder(getCarContext())
          .setErrorMessage(R.string.location_is_disabled_long_text)
          .setCloseable(true)
          .build()
      );
      return;
    }

    mPermissionsGrantedCallback.onPermissionsGranted();
    finish();
  }
}
