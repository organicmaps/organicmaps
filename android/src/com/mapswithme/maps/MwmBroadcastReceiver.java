package com.mapswithme.maps;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

public abstract class MwmBroadcastReceiver extends BroadcastReceiver {

    protected abstract void onReceiveInitialized(@NonNull Context context, @Nullable Intent intent);

    @Override
    public final void onReceive(Context context, Intent intent)
    {
        MwmApplication app = MwmApplication.from(context);
        if (!app.arePlatformAndCoreInitialized() && !app.initCore())
            return;
        onReceiveInitialized(context, intent);
    }

}
