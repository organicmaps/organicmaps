package com.mapswithme.util.push;

import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import com.pushwoosh.GcmRegistrationService;
import ru.mail.libnotify.api.NotificationFactory;

public class GcmInstanceIDRouterListenerService extends GcmRegistrationService
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.THIRD_PARTY);

  @Override
  public void onTokenRefresh()
  {
    LOGGER.i(GcmInstanceIDRouterListenerService.class.getSimpleName(), "onTokenRefresh()");
    super.onTokenRefresh();
    NotificationFactory.refreshGcmToken(this);
  }
}
