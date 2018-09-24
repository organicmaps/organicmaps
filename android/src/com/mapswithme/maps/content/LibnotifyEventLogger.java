package com.mapswithme.maps.content;

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

public class LibnotifyEventLogger extends DefaultEventLogger
{
  private final NotificationApi mApi;

  protected LibnotifyEventLogger(@NonNull Application application)
  {
    super(application);
    mApi = initLibnotify();
  }

  private NotificationApi initLibnotify()
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
    return NotificationFactory.get(getApplication());
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
    mApi.collectEventBatch(map);
  }
}
