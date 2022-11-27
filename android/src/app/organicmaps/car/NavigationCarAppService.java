package app.organicmaps.car;

import android.content.pm.ApplicationInfo;

import androidx.annotation.NonNull;
import androidx.car.app.CarAppService;
import androidx.car.app.Session;
import androidx.car.app.validation.HostValidator;

import app.organicmaps.R;

public final class NavigationCarAppService extends CarAppService
{
  @NonNull
  @Override
  public HostValidator createHostValidator()
  {
    if ((getApplicationInfo().flags & ApplicationInfo.FLAG_DEBUGGABLE) != 0)
    {
      return HostValidator.ALLOW_ALL_HOSTS_VALIDATOR;
    }
    else
    {
      return new HostValidator.Builder(getApplicationContext())
          .addAllowedHosts(R.array.hosts_allowlist)
          .build();
    }
  }

  @NonNull
  @Override
  public Session onCreateSession()
  {
    return new NavigationSession();
  }
}
