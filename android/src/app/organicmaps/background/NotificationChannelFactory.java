package app.organicmaps.background;

import android.app.Application;

import androidx.annotation.NonNull;

import app.organicmaps.util.Utils;

public class NotificationChannelFactory
{
  @NonNull
  public static NotificationChannelProvider createProvider(@NonNull Application app)
  {
    return Utils.isOreoOrLater() ? new OreoCompatNotificationChannelProvider(app)
                                 : new StubNotificationChannelProvider(app);
  }
}
