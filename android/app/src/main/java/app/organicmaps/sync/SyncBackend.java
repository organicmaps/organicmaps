package app.organicmaps.sync;

import android.content.Context;
import android.graphics.drawable.Drawable;

public interface SyncBackend
{
  /**
   * @return an integer that denotes the backend type.
   * Persisted to the disk, so it must not be changed once published.
   */
  int getId();

  String getDisplayName(Context context);

  Drawable getIcon(Context context);

  void login(Context context);
}
