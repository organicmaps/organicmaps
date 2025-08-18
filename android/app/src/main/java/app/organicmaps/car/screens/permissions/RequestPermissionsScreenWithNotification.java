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
import app.organicmaps.car.CarAppService;
import app.organicmaps.car.screens.base.BaseScreen;
import app.organicmaps.car.util.UserActionRequired;
import app.organicmaps.sdk.util.LocationUtils;
import app.organicmaps.sdk.util.concurrency.ThreadPool;
import app.organicmaps.sdk.util.concurrency.UiThread;
import java.util.concurrent.ExecutorService;

public class RequestPermissionsScreenWithNotification extends BaseScreen implements UserActionRequired
{
  public static final int NOTIFICATION_ID = RequestPermissionsScreenWithNotification.class.getSimpleName().hashCode();

  @NonNull
  private final ExecutorService mBackgroundExecutor;
  private boolean mIsPermissionCheckEnabled = true;
  @NonNull
  private final Runnable mPermissionsGrantedCallback;

  public RequestPermissionsScreenWithNotification(@NonNull CarContext carContext,
                                                  @NonNull Runnable permissionsGrantedCallback)
  {
    super(carContext);
    mBackgroundExecutor = ThreadPool.getWorker();
    mPermissionsGrantedCallback = permissionsGrantedCallback;
  }

  @NonNull
  @Override
  public Template onGetTemplate()
  {
    final MessageTemplate.Builder builder =
        new MessageTemplate.Builder(getCarContext().getString(R.string.aa_location_permissions_request));

    final Header.Builder headerBuilder = new Header.Builder();
    headerBuilder.setStartHeaderAction(Action.APP_ICON);
    headerBuilder.setTitle(getCarContext().getString(R.string.aa_grant_permissions));
    builder.setHeader(headerBuilder.build());

    builder.setIcon(
        new CarIcon.Builder(IconCompat.createWithResource(getCarContext(), R.drawable.ic_location_off)).build());
    return builder.build();
  }

  @Override
  @RequiresPermission(value = Manifest.permission.POST_NOTIFICATIONS)
  public void onStart(@NonNull LifecycleOwner owner)
  {
    mIsPermissionCheckEnabled = true;
    mBackgroundExecutor.execute(this::checkPermissions);
    sendPermissionsRequestNotification();
  }

  @Override
  public void onStop(@NonNull LifecycleOwner owner)
  {
    mIsPermissionCheckEnabled = false;
  }

  @Override
  public void onDestroy(@NonNull LifecycleOwner owner)
  {
    NotificationManagerCompat.from(getCarContext()).cancel(NOTIFICATION_ID);
  }

  private void checkPermissions()
  {
    if (!mIsPermissionCheckEnabled)
      return;

    if (LocationUtils.checkLocationPermission(getCarContext()))
    {
      UiThread.runLater(() -> {
        mPermissionsGrantedCallback.run();
        finish();
      });
    }
    else
      mBackgroundExecutor.execute(this::checkPermissions);
  }

  @RequiresPermission(value = Manifest.permission.POST_NOTIFICATIONS)
  private void sendPermissionsRequestNotification()
  {
    final int FLAG_IMMUTABLE = Build.VERSION.SDK_INT < Build.VERSION_CODES.M ? 0 : PendingIntent.FLAG_IMMUTABLE;
    final Intent contentIntent = new Intent(getCarContext(), RequestPermissionsActivity.class);
    final PendingIntent pendingIntent = PendingIntent.getActivity(getCarContext(), 0, contentIntent,
                                                                  PendingIntent.FLAG_CANCEL_CURRENT | FLAG_IMMUTABLE);

    final NotificationCompat.Builder builder =
        new NotificationCompat.Builder(getCarContext(), CarAppService.ANDROID_AUTO_NOTIFICATION_CHANNEL_ID);
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
