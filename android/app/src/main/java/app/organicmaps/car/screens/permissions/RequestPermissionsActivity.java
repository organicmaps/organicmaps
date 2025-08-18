package app.organicmaps.car.screens.permissions;

import static android.Manifest.permission.ACCESS_COARSE_LOCATION;
import static android.Manifest.permission.ACCESS_FINE_LOCATION;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.provider.Settings;
import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.Nullable;
import androidx.core.app.NotificationManagerCompat;
import app.organicmaps.R;
import app.organicmaps.base.BaseMwmFragmentActivity;
import app.organicmaps.sdk.util.LocationUtils;
import java.util.Objects;

public class RequestPermissionsActivity extends BaseMwmFragmentActivity
{
  private static final String[] LOCATION_PERMISSIONS = new String[] {ACCESS_COARSE_LOCATION, ACCESS_FINE_LOCATION};

  @Nullable
  private ActivityResultLauncher<String[]> mPermissionsRequest;

  @Override
  protected void onSafeCreate(@Nullable Bundle savedInstanceState)
  {
    super.onSafeCreate(savedInstanceState);
    setContentView(R.layout.activity_request_permissions);

    findViewById(R.id.btn_grant_permissions).setOnClickListener(unused -> openAppPermissionSettings());
    mPermissionsRequest = registerForActivityResult(new ActivityResultContracts.RequestMultiplePermissions(),
                                                    (grantedPermissions) -> closeIfPermissionsGranted());
    mPermissionsRequest.launch(LOCATION_PERMISSIONS);
  }

  @Override
  protected void onResume()
  {
    super.onResume();
    closeIfPermissionsGranted();
  }

  @Override
  protected void onSafeDestroy()
  {
    super.onSafeDestroy();
    Objects.requireNonNull(mPermissionsRequest).unregister();
    mPermissionsRequest = null;
  }

  private void openAppPermissionSettings()
  {
    final Intent intent = new Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS);
    intent.setData(Uri.fromParts("package", getPackageName(), null));
    intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
    startActivity(intent);
  }

  private void closeIfPermissionsGranted()
  {
    if (!LocationUtils.checkLocationPermission(this))
      return;

    NotificationManagerCompat.from(this).cancel(RequestPermissionsScreenWithNotification.NOTIFICATION_ID);
    finish();
  }
}
