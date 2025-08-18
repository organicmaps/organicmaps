package app.organicmaps.car.screens.permissions;

import static android.Manifest.permission.POST_NOTIFICATIONS;
import static android.content.pm.PackageManager.PERMISSION_GRANTED;

import android.os.Build;
import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import androidx.car.app.Screen;
import androidx.core.content.ContextCompat;
import app.organicmaps.sdk.util.log.Logger;

public class RequestPermissionsScreenBuilder
{
  private static final String TAG = RequestPermissionsScreenBuilder.class.getSimpleName();

  public static Screen build(@NonNull CarContext carContext, @NonNull Runnable permissionsGrantedCallback)
  {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU
        && ContextCompat.checkSelfPermission(carContext, POST_NOTIFICATIONS) != PERMISSION_GRANTED)
    {
      Logger.w(TAG, "Permission POST_NOTIFICATIONS is not granted, using API-based permissions request");
      return new RequestPermissionsScreenWithApi(carContext, permissionsGrantedCallback);
    }
    return new RequestPermissionsScreenWithNotification(carContext, permissionsGrantedCallback);
  }
}
