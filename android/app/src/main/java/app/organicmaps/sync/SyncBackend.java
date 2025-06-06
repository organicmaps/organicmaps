package app.organicmaps.sync;

import android.content.Context;
import android.graphics.drawable.Drawable;
import androidx.annotation.Nullable;

public interface SyncBackend
{
  /**
   * @return an integer that denotes the backend type. Persisted to the disk so must not be changed.
   */
  int getId();

  String getDisplayName(Context context);

  Drawable getIcon(Context context);

  void login(Context context, @Nullable LoginSuccessCallback callback);

  Class<? extends AuthState> getAuthStateClass();

  interface LoginSuccessCallback
  {
    void onLoginSuccess();
  }
}
