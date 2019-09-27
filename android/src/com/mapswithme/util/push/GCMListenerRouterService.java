package com.mapswithme.util.push;

import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import android.text.TextUtils;

import com.google.android.gms.gcm.GcmListenerService;
import com.mapswithme.maps.BuildConfig;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import com.pushwoosh.internal.utils.NotificationRegistrarHelper;

// It's temporary class, it may be deleted along with Pushwoosh sdk.
// The base of this code is taken from https://www.pushwoosh.com/docs/gcm-integration-legacy.
public class GCMListenerRouterService extends GcmListenerService
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.THIRD_PARTY);
  private static final String TAG = GCMListenerRouterService.class.getSimpleName();

  @Override
  public void onMessageReceived(@Nullable String from, @Nullable Bundle data)
  {
    LOGGER.i(TAG, "Gcm router service received message: "
                  + (data != null ? data.toString() : "<null>") + " from: " + from);

    if (data == null || TextUtils.isEmpty(from))
      return;

    // Base GCM listener service removes this extra before calling onMessageReceived.
    // Need to set it again to pass intent to another service.
    data.putString("from", from);

    String pwProjectId = getPWProjectId(getApplicationContext());
    if (!TextUtils.isEmpty(pwProjectId) && pwProjectId.contains(from))
      NotificationRegistrarHelper.handleMessage(data);
  }

  @Nullable
  public static String getPWProjectId(@NonNull Context context)
  {
    PackageManager pMngr = context.getPackageManager();
    try
    {
      ApplicationInfo ai = pMngr.getApplicationInfo(context.getPackageName(), PackageManager
          .GET_META_DATA);
      Bundle metaData = ai.metaData;
      if (metaData == null)
        return null;
      return metaData.getString(BuildConfig.PW_PROJECT_ID);
    }
    catch (PackageManager.NameNotFoundException e)
    {
      LOGGER.e(TAG, "Failed to get push woosh projectId: ", e);
    }
    return null;
  }
}
