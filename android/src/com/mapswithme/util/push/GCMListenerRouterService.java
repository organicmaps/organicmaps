package com.mapswithme.util.push;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.text.TextUtils;

import com.google.android.gms.gcm.GcmListenerService;
import com.google.android.gms.gcm.GcmReceiver;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import com.pushwoosh.PushGcmIntentService;
import ru.mail.libnotify.api.NotificationFactory;

// It's temporary class, it may be deleted along with Pushwoosh sdk.
// The base of this code is taken from https://www.pushwoosh.com/docs/gcm-integration-legacy.
public class GCMListenerRouterService extends GcmListenerService
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.THIRD_PARTY);
  private static final String TAG = GCMListenerRouterService.class.getSimpleName();

  @Override
  public void onMessageReceived(@Nullable String from, @Nullable Bundle data) {
    LOGGER.i(TAG, "Gcm router service received message: "
             + (data != null ? data.toString() : "<null>") + " from: " + from);

    if (data == null || TextUtils.isEmpty(from))
      return;

    // Base GCM listener service removes this extra before calling onMessageReceived.
    // Need to set it again to pass intent to another service.
    data.putString("from", from);

    String pwProjectId = getPWProjectId(getApplicationContext());
    if (!TextUtils.isEmpty(pwProjectId) && pwProjectId.contains(from)) {
      dispatchMessage(PushGcmIntentService.class.getName(), data);
      return;
    }

    NotificationFactory.deliverGcmMessageIntent(this, from, data);
  }

  @Nullable
  private static String getPWProjectId(@NonNull Context context)
  {
    PackageManager pMngr = context.getPackageManager();
    try
    {
      ApplicationInfo ai = pMngr.getApplicationInfo(context.getPackageName(), PackageManager
          .GET_META_DATA);
      Bundle metaData = ai.metaData;
      if (metaData == null)
        return null;
      return metaData.getString("PW_PROJECT_ID");
    }
    catch (PackageManager.NameNotFoundException e)
    {
      LOGGER.e(TAG, "Failed to get push woosh projectId: ", e);
    }
    return null;
  }

  private void dispatchMessage(@NonNull String component, @NonNull Bundle data) {
    Intent intent = new Intent();
    intent.putExtras(data);
    intent.setAction("com.google.android.c2dm.intent.RECEIVE");
    intent.setComponent(new ComponentName(getPackageName(), component));

    GcmReceiver.startWakefulService(getApplicationContext(), intent);
  }
}
