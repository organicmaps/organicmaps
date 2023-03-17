package app.organicmaps.compat;

import android.os.Build;
import androidx.annotation.NonNull;


public class CompatHelper
{

  private static CompatHelper sInstance;

  @NonNull
  private final Compat mCompat;

  private CompatHelper() {
    if (sdkVersion() >= Build.VERSION_CODES.M) {
      mCompat = new CompatV23();
    } else {
      mCompat = new CompatV21();
    }
  }

  @NonNull
  public static Compat getCompat() {
    return getInstance().mCompat;
  }

  @NonNull
  public static synchronized CompatHelper getInstance() {
    if (sInstance == null) {
      sInstance = new CompatHelper();
    }
    return sInstance;
  }

  /** Get the current Android API level.  */
  public static int sdkVersion() {
    return Build.VERSION.SDK_INT;
  }

}
