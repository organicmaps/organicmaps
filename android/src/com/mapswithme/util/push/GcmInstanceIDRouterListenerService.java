package com.mapswithme.util.push;

import androidx.annotation.Nullable;

import com.google.android.gms.gcm.GoogleCloudMessaging;
import com.google.android.gms.iid.InstanceID;
import com.google.android.gms.iid.InstanceIDListenerService;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import com.pushwoosh.PushwooshFcmHelper;

import java.io.IOException;

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
      LOGGER.e(TAG, "Failed to obtained refreshed token: ", e);
    }
  }

  private void onTokenRefreshInternal() throws IOException
  {    
    String token = getRefreshedToken();
    PushwooshFcmHelper.onTokenRefresh(this, token);
  }

  @Nullable
  private String getRefreshedToken() throws IOException
  {
    String projectId = GCMListenerRouterService.getPWProjectId(this);
    InstanceID instanceID = InstanceID.getInstance(getApplicationContext());
    return instanceID.getToken(projectId, GoogleCloudMessaging.INSTANCE_ID_SCOPE);
  }
}
