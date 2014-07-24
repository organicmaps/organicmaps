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

package com.facebook.widget;

import android.os.Bundle;
import android.test.suitebuilder.annotation.LargeTest;
import android.test.suitebuilder.annotation.MediumTest;
import com.facebook.SessionDefaultAudience;
import com.facebook.SessionLoginBehavior;

import java.util.List;

public class UserSettingsFragmentTests extends FragmentTestCase<UserSettingsFragmentTests.TestActivity> {

    public UserSettingsFragmentTests() {
        super(TestActivity.class);
    }

    @MediumTest
    @LargeTest
    public void testCanSetParametersViaLayout() throws Throwable {
        TestActivity activity = getActivity();
        assertNotNull(activity);

        final UserSettingsFragment fragment = activity.getFragment();
        assertNotNull(fragment);

        assertEquals(SessionLoginBehavior.SUPPRESS_SSO, fragment.getLoginBehavior());
        assertEquals(SessionDefaultAudience.EVERYONE, fragment.getDefaultAudience());
        List<String> permissions = fragment.getPermissions();
        assertEquals(2, permissions.size());
        assertEquals("read_1", permissions.get(0));
    }

    public static class TestActivity extends FragmentTestCase.TestFragmentActivity<UserSettingsFragment> {
        public TestActivity() {
            super(UserSettingsFragment.class);
        }

        @Override
        public void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            getSupportFragmentManager().executePendingTransactions();
            UserSettingsFragment fragment = getFragment();
            fragment.setLoginBehavior(SessionLoginBehavior.SUPPRESS_SSO);
            fragment.setReadPermissions("read_1", "read_2");
            fragment.setDefaultAudience(SessionDefaultAudience.EVERYONE);
        }
    }
}
