package com.mapswithme.maps.background;

import android.app.Application;
import androidx.annotation.NonNull;

import com.mapswithme.util.Utils;

public class NotificationChannelFactory
{
  @NonNull
  public static NotificationChannelProvider createProvider(@NonNull Application app)
  {
    return Utils.isOreoOrLater() ? new OreoCompatNotificationChannelProvider(app)
                                 : new StubNotificationChannelProvider(app);
  }
}
