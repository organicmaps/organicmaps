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

package com.facebook;

import android.app.Activity;
import android.content.Intent;
import android.test.suitebuilder.annotation.LargeTest;
import android.test.suitebuilder.annotation.MediumTest;
import android.test.suitebuilder.annotation.SmallTest;

public class FacebookActivityTests extends FacebookActivityTestCase<FacebookActivityTests.FacebookTestActivity> {
    public FacebookActivityTests() {
        super(FacebookActivityTests.FacebookTestActivity.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        Session activeSession = Session.getActiveSession();
        if (activeSession != null) {
            activeSession.closeAndClearTokenInformation();
        }
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testLaunchingWithEmptyIntent() {
        Intent intent = new Intent(Intent.ACTION_MAIN);
        setActivityIntent(intent);
        FacebookTestActivity activity = getActivity();

        assertNull(Session.getActiveSession());
        assertFalse(activity.hasNativeLinkIntentForTesting());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testLaunchingWithValidNativeLinkingIntent() {
        final String token = "A token less unique than most";

        Intent intent = new Intent(Intent.ACTION_MAIN);
        intent.putExtras(getNativeLinkingExtras(token));
        setActivityIntent(intent);

        assertNull(Session.getActiveSession());

        FacebookTestActivity activity = getActivity();
        Session activeSession = Session.getActiveSession();
        assertNull(activeSession);
        assertTrue(activity.hasNativeLinkIntentForTesting());
    }

    public static class FacebookTestActivity extends Activity {
        public boolean hasNativeLinkIntentForTesting() {
            return AccessToken.createFromNativeLinkingIntent(getIntent()) != null;
        }
    }
}
