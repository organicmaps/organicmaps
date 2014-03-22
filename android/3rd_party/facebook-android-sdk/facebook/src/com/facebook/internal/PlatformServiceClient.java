/**
 * Copyright 2010-present Facebook.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.facebook.internal;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.*;

/**
 * com.facebook.internal is solely for the use of other packages within the Facebook SDK for Android. Use of
 * any of the classes in this package is unsupported, and they may be modified or removed without warning at
 * any time.
 */
abstract public class PlatformServiceClient implements ServiceConnection {
    private final Context context;
    private final Handler handler;
    private CompletedListener listener;
    private boolean running;
    private Messenger sender;
    private int requestMessage;
    private int replyMessage;
    private final String applicationId;
    private final int protocolVersion;

    public PlatformServiceClient(Context context, int requestMessage, int replyMessage, int protocolVersion,
            String applicationId) {
        Context applicationContext = context.getApplicationContext();

        this.context = (applicationContext != null) ? applicationContext : context;
        this.requestMessage = requestMessage;
        this.replyMessage = replyMessage;
        this.applicationId = applicationId;
        this.protocolVersion = protocolVersion;

        handler = new Handler() {
            @Override
            public void handleMessage(Message message) {
                PlatformServiceClient.this.handleMessage(message);
            }
        };
    }

    public void setCompletedListener(CompletedListener listener) {
        this.listener = listener;
    }

    protected Context getContext() {
        return context;
    }

    public boolean start() {
        if (running) {
            return false;
        }

        // Make sure that the service can handle the requested protocol version
        int availableVersion = NativeProtocol.getLatestAvailableProtocolVersionForService(context, protocolVersion);
        if (availableVersion == NativeProtocol.NO_PROTOCOL_AVAILABLE) {
            return false;
        }

        Intent intent = NativeProtocol.createPlatformServiceIntent(context);
        if (intent == null) {
            return false;
        } else {
            running = true;
            context.bindService(intent, this, Context.BIND_AUTO_CREATE);
            return true;
        }
    }

    public void cancel() {
        running = false;
    }

    public void onServiceConnected(ComponentName name, IBinder service) {
        sender = new Messenger(service);
        sendMessage();
    }

    public void onServiceDisconnected(ComponentName name) {
        sender = null;
        context.unbindService(this);
        callback(null);
    }

    private void sendMessage() {
        Bundle data = new Bundle();
        data.putString(NativeProtocol.EXTRA_APPLICATION_ID, applicationId);

        populateRequestBundle(data);

        Message request = Message.obtain(null, requestMessage);
        request.arg1 = protocolVersion;
        request.setData(data);
        request.replyTo = new Messenger(handler);

        try {
            sender.send(request);
        } catch (RemoteException e) {
            callback(null);
        }
    }

    protected abstract void populateRequestBundle(Bundle data);

    protected void handleMessage(Message message) {
        if (message.what == replyMessage) {
            Bundle extras = message.getData();
            String errorType = extras.getString(NativeProtocol.STATUS_ERROR_TYPE);
            if (errorType != null) {
                callback(null);
            } else {
                callback(extras);
            }
            context.unbindService(this);
        }
    }

    private void callback(Bundle result) {
        if (!running) {
            return;
        }
        running = false;

        CompletedListener callback = listener;
        if (callback != null) {
            callback.completed(result);
        }
    }

    public interface CompletedListener {
        void completed(Bundle result);
    }
}
