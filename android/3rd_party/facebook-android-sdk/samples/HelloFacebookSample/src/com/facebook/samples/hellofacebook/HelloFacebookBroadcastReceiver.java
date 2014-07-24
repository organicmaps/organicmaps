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

package com.facebook.samples.hellofacebook;

import android.os.Bundle;
import android.util.Log;
import com.facebook.FacebookBroadcastReceiver;

/**
 * This is a simple example to demonstrate how an app could extend FacebookBroadcastReceiver to handle
 * notifications that long-running operations such as photo uploads have finished.
 */
public class HelloFacebookBroadcastReceiver extends FacebookBroadcastReceiver {

    @Override
    protected void onSuccessfulAppCall(String appCallId, String action, Bundle extras) {
        // A real app could update UI or notify the user that their photo was uploaded.
        Log.d("HelloFacebook", String.format("Photo uploaded by call " + appCallId + " succeeded."));
    }

    @Override
    protected void onFailedAppCall(String appCallId, String action, Bundle extras) {
        // A real app could update UI or notify the user that their photo was not uploaded.
        Log.d("HelloFacebook", String.format("Photo uploaded by call " + appCallId + " failed."));
    }
}
