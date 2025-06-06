package app.organicmaps.sync;

import android.content.Context;
import android.graphics.drawable.Drawable;
import androidx.appcompat.content.res.AppCompatResources;
import app.organicmaps.R;
import java.util.Map;
import java.util.stream.Collectors;
import java.util.stream.Stream;

public enum BackendType implements SyncBackend
{
  Nextcloud {
    @Override
    public int getId()
    {
      return 0;
    }

    @Override
    public String getDisplayName(Context context)
    {
      return context.getString(R.string.nextcloud);
    }

    @Override
    public Drawable getIcon(Context context)
    {
      return AppCompatResources.getDrawable(context, R.drawable.ic_nextcloud);
    }

    @Override
    public void login(Context context, LoginSuccessCallback callback)
    {
      NextcloudLoginHelper.login(context, callback);
    }

    @Override
    public Class<? extends AuthState> getAuthStateClass()
    {
      return NextcloudAuth.class;
    }
  },
  GoogleDrive {
    @Override
    public int getId()
    {
      return 1;
    }

    @Override
    public String getDisplayName(Context context)
    {
      return context.getString(R.string.google_drive);
    }

    @Override
    public Drawable getIcon(Context context)
    {
      return AppCompatResources.getDrawable(context, R.drawable.ic_google_drive);
    }

    @Override
    public void login(Context context, LoginSuccessCallback callback)
    {
      throw new RuntimeException("TODO implement");
    }

    @Override
    public Class<? extends AuthState> getAuthStateClass()
    {
      return GoogleDriveAuth.class;
    }
  };

  public static final Map<Integer, BackendType> idToBackendType =
      Stream.of(BackendType.values()).collect(Collectors.toMap(BackendType::getId, b -> b));
}
