package app.organicmaps.car.screens.permissions;

import android.Manifest;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Intent;
import android.os.Build;
import androidx.annotation.NonNull;
import androidx.annotation.RequiresPermission;
import androidx.car.app.CarContext;
import androidx.car.app.model.Action;
import androidx.car.app.model.CarIcon;
import androidx.car.app.model.Header;
import androidx.car.app.model.MessageTemplate;
import androidx.car.app.model.Template;
import androidx.core.app.NotificationCompat;
import androidx.core.app.NotificationManagerCompat;
import androidx.core.content.ContextCompat;
import androidx.core.graphics.drawable.IconCompat;
import androidx.lifecycle.LifecycleOwner;
import app.organicmaps.R;
import app.organicmaps.car.AndroidAutoService;
import app.organicmaps.sdk.OrganicMaps;
import app.organicmaps.sdk.util.concurrency.UiThread;

public class RequestPermissionsScreenWithNotification extends RequestPermissionsScreen
{
  public static final int NOTIFICATION_ID = RequestPermissionsScreenWithNotification.class.getSimpleName().hashCode();

  private final Runnable mCheckPermissions = this::checkPermissions;

  public RequestPermissionsScreenWithNotification(@NonNull CarContext carContext,
                                                  @NonNull OrganicMaps organicMapsContext,
                                                  @NonNull Runnable permissionsGrantedCallback)
  {
    super(carContext, organicMapsContext, permissionsGrantedCallback);
  }

  @NonNull
  @Override
  protected Template onGetTemplateImpl()
  {
    final MessageTemplate.Builder builder =
        new MessageTemplate.Builder(getCarContext().getString(R.string.aa_location_permissions_request));

    final Header.Builder headerBuilder = new Header.Builder();
    headerBuilder.setStartHeaderAction(Action.APP_ICON);
    headerBuilder.setTitle(getCarContext().getString(R.string.aa_grant_permissions));
    builder.setHeader(headerBuilder.build());

    builder.setIcon(
        new CarIcon.Builder(IconCompat.createWithResource(getCarContext(), R.drawable.ic_location_off)).build());
    builder.addAction(closeAction());
    return builder.build();
  }

  @Override
  @RequiresPermission(value = Manifest.permission.POST_NOTIFICATIONS)
  public void onStart(@NonNull LifecycleOwner owner)
  {
    super.onStart(owner);
    sendPermissionsRequestNotification();
    UiThread.runLater(mCheckPermissions);
  }

  @Override
  public void onStop(@NonNull LifecycleOwner owner)
  {
    super.onStop(owner);
    UiThread.cancelDelayedTasks(mCheckPermissions);
  }

  @Override
  public void onDestroy(@NonNull LifecycleOwner owner)
  {
    super.onDestroy(owner);
    NotificationManagerCompat.from(getCarContext()).cancel(NOTIFICATION_ID);
  }

  private void checkPermissions()
  {
    if (!finishIfGranted())
      UiThread.runLater(mCheckPermissions, 1000);
  }

  @RequiresPermission(value = Manifest.permission.POST_NOTIFICATIONS)
  private void sendPermissionsRequestNotification()
  {
    final int FLAG_IMMUTABLE = Build.VERSION.SDK_INT < Build.VERSION_CODES.M ? 0 : PendingIntent.FLAG_IMMUTABLE;
    final Intent contentIntent = new Intent(getCarContext(), RequestPermissionsActivity.class);
    final PendingIntent pendingIntent = PendingIntent.getActivity(getCarContext(), 0, contentIntent,
                                                                  PendingIntent.FLAG_CANCEL_CURRENT | FLAG_IMMUTABLE);

    final NotificationCompat.Builder builder =
        new NotificationCompat.Builder(getCarContext(), AndroidAutoService.ANDROID_AUTO_NOTIFICATION_CHANNEL_ID);
    builder.setCategory(NotificationCompat.CATEGORY_NAVIGATION)
        .setVisibility(NotificationCompat.VISIBILITY_PUBLIC)
        .setOngoing(true)
        .setShowWhen(false)
        .setOnlyAlertOnce(true)
        .setSmallIcon(R.drawable.ic_location_crosshair)
        .setColor(ContextCompat.getColor(getCarContext(), R.color.notification))
        .setContentTitle(getCarContext().getString(R.string.aa_request_permission_notification))
        .setContentIntent(pendingIntent);
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N)
      builder.setPriority(NotificationManager.IMPORTANCE_HIGH);
    NotificationManagerCompat.from(getCarContext()).notify(NOTIFICATION_ID, builder.build());
  }
}
