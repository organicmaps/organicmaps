package app.organicmaps.car.screens;

import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Intent;
import android.os.Build;

import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import androidx.car.app.model.Action;
import androidx.car.app.model.CarIcon;
import androidx.car.app.model.MessageTemplate;
import androidx.car.app.model.Template;
import androidx.core.app.NotificationCompat;
import androidx.core.app.NotificationManagerCompat;
import androidx.core.content.ContextCompat;
import androidx.core.graphics.drawable.IconCompat;
import androidx.lifecycle.LifecycleOwner;

import app.organicmaps.R;
import app.organicmaps.car.CarAppService;
import app.organicmaps.car.activity.RequestPermissionsActivity;
import app.organicmaps.car.screens.base.BaseScreen;
import app.organicmaps.util.LocationUtils;
import app.organicmaps.util.concurrency.ThreadPool;
import app.organicmaps.util.concurrency.UiThread;

import java.util.concurrent.ExecutorService;

public class RequestPermissionsScreen extends BaseScreen
{
  public static final int NOTIFICATION_ID = RequestPermissionsScreen.class.getSimpleName().hashCode();

  @NonNull
  private final ExecutorService mBackgroundExecutor;
  private boolean mIsPermissionCheckEnabled = true;
  @NonNull
  private final Runnable mPermissionsGrantedCallback;

  public RequestPermissionsScreen(@NonNull CarContext carContext, @NonNull Runnable permissionsGrantedCallback)
  {
    super(carContext);
    mBackgroundExecutor = ThreadPool.getWorker();
    mPermissionsGrantedCallback = permissionsGrantedCallback;
  }

  @NonNull
  @Override
  public Template onGetTemplate()
  {
    final MessageTemplate.Builder builder = new MessageTemplate.Builder(getCarContext().getString(R.string.aa_location_permissions_request));
    builder.setHeaderAction(Action.APP_ICON);
    builder.setTitle(getCarContext().getString(R.string.aa_grant_permissions));
    builder.setIcon(new CarIcon.Builder(IconCompat.createWithResource(getCarContext(), R.drawable.ic_location_off)).build());
    return builder.build();
  }

  @Override
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

  private void sendPermissionsRequestNotification()
  {
    final int FLAG_IMMUTABLE = Build.VERSION.SDK_INT < Build.VERSION_CODES.M ? 0 : PendingIntent.FLAG_IMMUTABLE;
    final Intent contentIntent = new Intent(getCarContext(), RequestPermissionsActivity.class);
    final PendingIntent pendingIntent = PendingIntent.getActivity(getCarContext(), 0, contentIntent,
        PendingIntent.FLAG_CANCEL_CURRENT | FLAG_IMMUTABLE);

    final NotificationCompat.Builder builder = new NotificationCompat.Builder(getCarContext(), CarAppService.ANDROID_AUTO_NOTIFICATION_CHANNEL_ID);
    builder.setCategory(NotificationCompat.CATEGORY_NAVIGATION)
        .setPriority(NotificationManager.IMPORTANCE_HIGH)
        .setVisibility(NotificationCompat.VISIBILITY_PUBLIC)
        .setOngoing(true)
        .setShowWhen(false)
        .setOnlyAlertOnce(true)
        .setSmallIcon(R.drawable.ic_my_location)
        .setColor(ContextCompat.getColor(getCarContext(), R.color.notification))
        .setContentTitle(getCarContext().getString(R.string.aa_request_permission_notification))
        .setContentIntent(pendingIntent);
    NotificationManagerCompat.from(getCarContext()).notify(NOTIFICATION_ID, builder.build());
  }
}
