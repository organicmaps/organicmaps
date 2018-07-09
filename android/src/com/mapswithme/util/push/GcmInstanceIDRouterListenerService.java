package com.mapswithme.util.push;

import android.content.Intent;

import com.google.android.gms.iid.InstanceIDListenerService;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import com.pushwoosh.GCMInstanceIDListenerService;
import ru.mail.libnotify.api.NotificationFactory;

public class GcmInstanceIDRouterListenerService extends InstanceIDListenerService
{
  @Override
  public void onTokenRefresh()
  {
    super.onTokenRefresh();
    Logger l = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.THIRD_PARTY);
    l.i(GcmInstanceIDRouterListenerService.class.getSimpleName(), "onTokenRefresh()");
    Intent pwIntent = new Intent(this, GCMInstanceIDListenerService.class);
    pwIntent.setAction("com.google.android.gms.iid.InstanceID");
    startService(pwIntent);
    NotificationFactory.refreshGcmToken(this);
  }
}
