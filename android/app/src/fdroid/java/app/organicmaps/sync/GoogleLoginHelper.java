package app.organicmaps.sync;

import androidx.appcompat.app.AppCompatActivity;

public class GoogleLoginHelper
{
  public static final String TAG = GoogleLoginHelper.class.getSimpleName();

  public static void attemptPlayServicesAuth(AppCompatActivity activity,
                                             GoogleLoginActivity.GmsAuthFailedCallback gmsAuthFailedCallback)
  {
    gmsAuthFailedCallback.onGmsAuthFailed();
  }
}
