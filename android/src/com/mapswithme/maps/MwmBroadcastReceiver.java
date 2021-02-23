package com.mapswithme.maps;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

public abstract class MwmBroadcastReceiver extends BroadcastReceiver {

    protected abstract void onReceiveInitialized(Context context, Intent intent);

    @Override
    public final void onReceive(Context context, Intent intent)
    {
        MwmApplication app = MwmApplication.from(context);
        if (!app.arePlatformAndCoreInitialized())
        {
            app.initCore();
        }
        onReceiveInitialized(context, intent);
    }

}
