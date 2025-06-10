package app.organicmaps.sync;

import android.content.Context;
import android.graphics.drawable.Drawable;

public interface SyncBackend
{
  /**
   * @return an integer that denotes the backend type. Persisted to the disk, so must not be changed once the app is released.
   */
  int getId();

  String getDisplayName(Context context);

  Drawable getIcon(Context context);

  void login(Context context);

  Class<? extends AuthState> getAuthStateClass();

  interface LoginSuccessCallback
  {
    void onLoginSuccess();
  }
}
