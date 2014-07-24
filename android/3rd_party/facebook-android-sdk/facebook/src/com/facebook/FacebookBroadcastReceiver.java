package com.facebook;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import com.facebook.internal.NativeProtocol;

/**
 * This class implements a simple BroadcastReceiver designed to listen for broadcast notifications from the
 * Facebook app. At present, these notifications consistent of success/failure notifications for photo upload
 * operations that happen in the background.
 *
 * Applications may subclass this class and register it in their AndroidManifest.xml, listening on the
 * com.facebook.platform.AppCallResultBroadcast action.
 */
public class FacebookBroadcastReceiver extends BroadcastReceiver {

    @Override
    public void onReceive(Context context, Intent intent) {
        String appCallId = intent.getStringExtra(NativeProtocol.EXTRA_PROTOCOL_CALL_ID);
        String action = intent.getStringExtra(NativeProtocol.EXTRA_PROTOCOL_ACTION);
        if (appCallId != null && action != null) {
            Bundle extras = intent.getExtras();

            if (NativeProtocol.isErrorResult(intent)) {
                onFailedAppCall(appCallId, action, extras);
            } else {
                onSuccessfulAppCall(appCallId, action, extras);
            }
        }
    }

    protected void onSuccessfulAppCall(String appCallId, String action, Bundle extras) {
        // Default does nothing.
    }

    protected void onFailedAppCall(String appCallId, String action, Bundle extras) {
        // Default does nothing.
    }
}
