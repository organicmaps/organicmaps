package app.organicmaps.sync;

import android.content.Context;
import androidx.annotation.DrawableRes;
import androidx.annotation.StringRes;
import app.organicmaps.R;
import app.organicmaps.sdk.sync.BackendType;
import app.organicmaps.sync.nextcloud.NextcloudLoginFlow;

public class BackendUtils
{
  @StringRes
  public static int getDisplayName(BackendType backendType)
  {
    return switch (backendType)
    {
      case Nextcloud -> R.string.nextcloud;
    };
  }

  @DrawableRes
  public static int getIcon(BackendType backendType)
  {
    return switch (backendType)
    {
      case Nextcloud -> R.drawable.ic_nextcloud;
    };
  }

  public static void login(Context context, BackendType backendType)
  {
    // noinspection SwitchStatementWithTooFewBranches Will be expanded later with more cloud providers
    switch (backendType)
    {
    case Nextcloud -> NextcloudLoginFlow.login(context);
    }
  }
}
