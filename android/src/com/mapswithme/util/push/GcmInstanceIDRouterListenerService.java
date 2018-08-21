package com.mapswithme.util.push;

import android.support.annotation.Nullable;

import com.google.android.gms.gcm.GoogleCloudMessaging;
import com.google.android.gms.iid.InstanceID;
import com.google.android.gms.iid.InstanceIDListenerService;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import com.pushwoosh.PushwooshFcmHelper;

import java.io.IOException;

import ru.mail.libnotify.api.NotificationFactory;

public class GcmInstanceIDRouterListenerService extends InstanceIDListenerService
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.THIRD_PARTY);
  private static final String TAG = GcmInstanceIDRouterListenerService.class.getSimpleName();
  
  @Override
  public void onTokenRefresh()
  {
    LOGGER.i(TAG, "onTokenRefresh()");
    super.onTokenRefresh();
    try
    {
      onTokenRefreshInternal();
    }
    catch (IOException e)
    {
      LOGGER.d(TAG, String.valueOf(e));
    }
  }

  private void onTokenRefreshInternal() throws IOException
  {    
    String token = getRefreshedToken();
    PushwooshFcmHelper.onTokenRefresh(this, token);
    NotificationFactory.refreshGcmToken(this);
  }

  @Nullable
  private String getRefreshedToken() throws IOException
  {
    String projectId = GCMListenerRouterService.getPWProjectId(this);
    InstanceID instanceID = InstanceID.getInstance(getApplicationContext());
    return instanceID.getToken(projectId, GoogleCloudMessaging.INSTANCE_ID_SCOPE);
  }
}
