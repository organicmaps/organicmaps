package app.organicmaps.sdk.sync;

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
    public void login(Context context)
    {
      NextcloudLoginHelper.login(context);
    }
  };

  public static final Map<Integer, BackendType> idToBackendType =
      Stream.of(BackendType.values()).collect(Collectors.toMap(BackendType::getId, b -> b));
}
