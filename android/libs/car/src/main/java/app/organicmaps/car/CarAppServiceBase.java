package app.organicmaps.car;

import android.content.Context;
import androidx.annotation.CallSuper;
import androidx.annotation.NonNull;
import androidx.car.app.CarAppService;
import androidx.car.app.Session;
import androidx.car.app.validation.HostValidator;
import androidx.core.app.NotificationCompat;
import app.organicmaps.api.Const;
import app.organicmaps.routing.NavigationService;

public abstract class CarAppServiceBase extends CarAppService
{
  public static final String API_CAR_HOST = Const.AUTHORITY + ".car";
  public static final String ACTION_SHOW_NAVIGATION_SCREEN = Const.ACTION_PREFIX + ".SHOW_NAVIGATION_SCREEN";

  private final boolean mIsDebug;

  protected CarAppServiceBase(boolean isDebug)
  {
    mIsDebug = isDebug;
  }

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

  @NonNull
  protected abstract NotificationCompat.Extender buildCarNotificationExtender(@NonNull Context context);

  @NonNull
  @Override
  public final Session onCreateSession()
  {
    return onCreateSession(null);
  }

  @Override
  @CallSuper
  public void onCreate()
  {
    super.onCreate();
    NavigationService.setCarNotificationExtender(buildCarNotificationExtender(getApplicationContext()));
  }

  @Override
  @CallSuper
  public void onDestroy()
  {
    super.onDestroy();
    NavigationService.setCarNotificationExtender(null);
  }
}
