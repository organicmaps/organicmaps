package app.organicmaps.sync;

import android.content.Context;
import android.content.Intent;
import androidx.annotation.DrawableRes;
import androidx.annotation.StringRes;
import app.organicmaps.R;
import app.organicmaps.sdk.sync.BackendType;
import app.organicmaps.sync.googledrive.GoogleLoginActivity;
import app.organicmaps.sync.nextcloud.NextcloudLoginFlow;

public class BackendUtils
{
  @StringRes
  public static int getDisplayName(BackendType backendType)
  {
    return switch (backendType)
    {
      case Nextcloud -> R.string.nextcloud;
      case GoogleDrive -> R.string.google_drive;
    };
  }

  @DrawableRes
  public static int getIcon(BackendType backendType)
  {
    return switch (backendType)
    {
      case Nextcloud -> R.drawable.ic_nextcloud;
      case GoogleDrive -> R.drawable.ic_google_drive;
    };
  }

  public static void login(Context context, BackendType backendType)
  {
    switch (backendType)
    {
    case Nextcloud -> NextcloudLoginFlow.login(context);
    case GoogleDrive ->
      context.startActivity(new Intent(context, GoogleLoginActivity.class)
                                .setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_NO_ANIMATION)
                                .putExtra(GoogleLoginActivity.EXTRA_PERFORM_LOGIN, true));
    }
  }
}
