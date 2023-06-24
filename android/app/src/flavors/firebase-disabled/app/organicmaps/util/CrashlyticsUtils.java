package app.organicmaps.util;

import android.content.Context;

import app.organicmaps.MwmApplication;
import app.organicmaps.base.Initializable;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

public enum CrashlyticsUtils implements Initializable<Context>
{
  INSTANCE;

  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private Context mContext;

  public void logException(@NonNull Throwable exception)
  {
    // No op
  }

  public void log(int priority, @NonNull String tag, @NonNull String msg)
  {
    // No op
  }

  @Override
  public void initialize(@Nullable Context context)
  {
    mContext = MwmApplication.from(context);
  }

  @Override
  public void destroy()
  {
    // No op
  }

  public boolean isAvailable()
  {
    return false;
  }

  public boolean isEnabled()
  {
    return SharedPropertiesUtils.isCrashlyticsEnabled(mContext);
  }

  public void setEnabled(@SuppressWarnings("unused") boolean isEnabled)
  {
    // No op
  }
}
