package app.organicmaps.car;

import android.annotation.SuppressLint;
import android.content.ComponentName;
import android.content.Intent;
import android.net.Uri;
import androidx.annotation.CallSuper;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RestrictTo;
import androidx.car.app.CarAppService;
import androidx.car.app.Session;
import androidx.car.app.SessionInfo;
import androidx.car.app.notification.CarAppExtender;
import androidx.car.app.notification.CarPendingIntent;
import androidx.car.app.validation.HostValidator;
import androidx.core.app.NotificationCompat;
import androidx.core.app.NotificationManagerCompat;
import app.organicmaps.api.Const;
import app.organicmaps.routing.NavigationService;
import java.util.Objects;

public abstract class CarAppServiceBase extends CarAppService
{
  public static final String API_CAR_HOST = Const.AUTHORITY + ".car";
  public static final String ACTION_SHOW_NAVIGATION_SCREEN = Const.ACTION_PREFIX + ".SHOW_NAVIGATION_SCREEN";

  @Nullable
  private static Class<? extends CarAppServiceBase> sCarService;

  private final boolean mIsDebug;

  protected CarAppServiceBase(boolean isDebug)
  {
    sCarService = getClass();
    mIsDebug = isDebug;
  }

  @SuppressLint("PrivateResource")
  @NonNull
  @Override
  public HostValidator createHostValidator()
  {
    if (mIsDebug)
      return HostValidator.ALLOW_ALL_HOSTS_VALIDATOR;

    return new HostValidator.Builder(getApplicationContext())
        .addAllowedHosts(androidx.car.app.R.array.hosts_allowlist_sample)
        .build();
  }

  @Override
  @NonNull
  public abstract Session onCreateSession(@NonNull SessionInfo sessionInfo);

  @NonNull
  @Override
  public final Session onCreateSession()
  {
    return onCreateSession(SessionInfo.DEFAULT_SESSION_INFO);
  }

  @Override
  @CallSuper
  public void onCreate()
  {
    super.onCreate();
    NavigationService.setCarNotificationExtender(buildCarNotificationExtender());
  }

  @Override
  @CallSuper
  public void onDestroy()
  {
    super.onDestroy();
    NavigationService.setCarNotificationExtender(null);
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY)
  @NonNull
  public static Class<? extends CarAppServiceBase> getCarServiceClass()
  {
    return Objects.requireNonNull(sCarService);
  }

  @NonNull
  private NotificationCompat.Extender buildCarNotificationExtender()
  {
    final Intent intent = new Intent(Intent.ACTION_VIEW)
                              .setComponent(new ComponentName(this, getCarServiceClass()))
                              .setData(Uri.fromParts(Const.API_SCHEME, CarAppServiceBase.API_CAR_HOST,
                                                     CarAppServiceBase.ACTION_SHOW_NAVIGATION_SCREEN));
    return new CarAppExtender.Builder()
        .setImportance(NotificationManagerCompat.IMPORTANCE_MIN)
        .setContentIntent(CarPendingIntent.getCarApp(this, intent.hashCode(), intent, 0))
        .build();
  }
}
