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

package com.facebook.samples.sessionlogin;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import com.facebook.AppEventsLogger;

public class SessionLoginSampleActivity extends Activity {

    private Button buttonLoginActivity;
    private Button buttonCustomFragment;
    private Button buttonLoginFragment;

    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.main);

        // We demonstrate three different ways of managing session login/logout behavior:
        // 1) LoginUsingActivityActivity implements an Activity that handles all of its own
        //    session management.
        // 2) LoginUsingCustomFragmentActivity uses a Fragment that handles session management;
        //    this Fragment could be composed as part of a more complex Activity, although in this
        //    case it is the only UI that the Activity displays.
        // 3) LoginUsingLoginFragmentActivity is similar to LoginUsingCustomFragmentActivity, but
        //    uses the UserSettingsFragment class provided by the SDK to handle session management. As
        //    in (2), this Fragment could be composed as part of a more complex Activity in a real app.
        buttonLoginActivity = (Button) findViewById(R.id.buttonLoginActivity);
        buttonLoginActivity.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(SessionLoginSampleActivity.this, LoginUsingActivityActivity.class);
                startActivity(intent);
            }
        });

        buttonCustomFragment = (Button) findViewById(R.id.buttonLoginCustomFragment);
        buttonCustomFragment.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(SessionLoginSampleActivity.this, LoginUsingCustomFragmentActivity.class);
                startActivity(intent);
            }
        });

        buttonLoginFragment = (Button) findViewById(R.id.buttonLoginFragment);
        buttonLoginFragment.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(SessionLoginSampleActivity.this, LoginUsingLoginFragmentActivity.class);
                startActivity(intent);
            }
        });
    }

    @Override
    protected void onResume() {
        super.onResume();

        // Call the 'activateApp' method to log an app event for use in analytics and advertising reporting.  Do so in
        // the onResume methods of the primary Activities that an app may be launched into.
        AppEventsLogger.activateApp(this);
    }
}
