package com.mapswithme.maps.analytics;

import android.app.Application;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.maps.BuildConfig;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import ru.mail.libnotify.api.NotificationApi;
import ru.mail.libnotify.api.NotificationFactory;
import ru.mail.notify.core.api.BackgroundAwakeMode;
import ru.mail.notify.core.api.NetworkSyncMode;

import java.util.Collections;
import java.util.Map;

class LibnotifyEventLogger extends DefaultEventLogger
{
  LibnotifyEventLogger(@NonNull Application application)
  {
    super(application);
  }

  @Override
  public void initialize()
  {
    if (BuildConfig.DEBUG || BuildConfig.BUILD_TYPE.equals("beta"))
    {
      NotificationFactory.enableDebugMode();
      NotificationFactory.setLogReceiver(LoggerFactory.INSTANCE.createLibnotifyLogger());
      NotificationFactory.setUncaughtExceptionListener((thread, throwable) -> {
        Logger l = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.THIRD_PARTY);
        l.e("LIBNOTIFY", "Thread: " + thread, throwable);
      });
    }
    NotificationFactory.setNetworkSyncMode(NetworkSyncMode.WIFI_ONLY);
    NotificationFactory.setBackgroundAwakeMode(BackgroundAwakeMode.DISABLED);
    NotificationFactory.initialize(getApplication());
    NotificationApi api = NotificationFactory.get(getApplication());
    api.allowDeviceIdTracking(false, false);
  }

  @Override
  public void sendTags(@NonNull String tag, @Nullable String[] params)
  {
    super.sendTags(tag, params);
    if (params == null)
      return;

    boolean isSingleParam = params.length == 1;
    Object value = isSingleParam ? params[0] : params;
    Map<String, Object> map = Collections.singletonMap(tag, value);
    NotificationApi api = NotificationFactory.get(getApplication());
    api.collectEventBatch(map);
  }
}
