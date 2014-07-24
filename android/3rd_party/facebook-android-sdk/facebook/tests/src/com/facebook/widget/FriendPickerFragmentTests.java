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
import android.test.TouchUtils;
import android.test.suitebuilder.annotation.LargeTest;
import android.test.suitebuilder.annotation.MediumTest;
import android.view.View;
import android.widget.CheckBox;
import android.widget.ListView;
import com.facebook.TestBlocker;
import com.facebook.TestSession;
import com.facebook.sdk.tests.R;

import java.util.Arrays;
import java.util.Collection;

public class FriendPickerFragmentTests extends FragmentTestCase<FriendPickerFragmentTests.TestActivity> {
    public FriendPickerFragmentTests() {
        super(TestActivity.class);
    }

    @MediumTest
    @LargeTest
    public void testCanSetParametersProgrammatically() throws Throwable {
        TestActivity activity = getActivity();
        assertNotNull(activity);

        runAndBlockOnUiThread(0, new Runnable() {
            @Override
            public void run() {
                Bundle bundle = new Bundle();
                // We deliberately set these to non-defaults to ensure they are set correctly.
                bundle.putString(FriendPickerFragment.USER_ID_BUNDLE_KEY, "4");
                bundle.putBoolean(FriendPickerFragment.MULTI_SELECT_BUNDLE_KEY, false);
                bundle.putBoolean(FriendPickerFragment.SHOW_PICTURES_BUNDLE_KEY, false);
                bundle.putString(FriendPickerFragment.EXTRA_FIELDS_BUNDLE_KEY, "middle_name,link");
                FriendPickerFragment fragment = new FriendPickerFragment(bundle);
                getActivity().setContentToFragment(fragment);
            }
        });

        // We don't just test the fragment we created directly above, because we want it to go through the
        // activity lifecycle and ensure the settings are still correct.
        final FriendPickerFragment fragment = activity.getFragment();
        assertNotNull(fragment);

        assertEquals("4", fragment.getUserId());
        assertEquals(false, fragment.getMultiSelect());
        assertEquals(false, fragment.getShowPictures());
        Collection<String> extraFields = fragment.getExtraFields();
        assertTrue(extraFields.contains("middle_name"));
        assertTrue(extraFields.contains("link"));
    }

    @MediumTest
    @LargeTest
    public void testCanSetParametersViaLayout() throws Throwable {
        TestActivity activity = getActivity();
        assertNotNull(activity);

        runAndBlockOnUiThread(0, new Runnable() {
            @Override
            public void run() {
                getActivity().setContentToLayout(R.layout.friend_picker_test_layout_1, R.id.friend_picker_fragment);
            }
        });

        final FriendPickerFragment fragment = activity.getFragment();
        assertNotNull(fragment);

        assertEquals(false, fragment.getShowPictures());
        assertEquals(false, fragment.getMultiSelect());
        Collection<String> extraFields = fragment.getExtraFields();
        assertTrue(extraFields.contains("middle_name"));
        assertTrue(extraFields.contains("link"));
        // It doesn't make sense to specify user id via layout, so we don't support it.
    }

    @LargeTest
    public void testFriendsLoad() throws Throwable {
        TestActivity activity = getActivity();
        assertNotNull(activity);

        // We don't auto-create any UI, so do it now. Needs to run on the UI thread.
        runAndBlockOnUiThread(0, new Runnable() {
            @Override
            public void run() {
                getActivity().setContentToFragment(null);
            }
        });

        final FriendPickerFragment fragment = activity.getFragment();
        assertNotNull(fragment);

        // Ensure our test user has at least one friend.
        final TestSession session1 = openTestSessionWithSharedUser();
        TestSession session2 = openTestSessionWithSharedUser(SECOND_TEST_USER_TAG);
        makeTestUsersFriends(session1, session2);

        // Trigger a data load (on the UI thread).
        final TestBlocker blocker = getTestBlocker();
        // We expect to get called twice -- once with results, once to indicate we are done paging.
        runAndBlockOnUiThread(2, new Runnable() {
            @Override
            public void run() {
                fragment.setSession(session1);
                fragment.setOnDataChangedListener(new PickerFragment.OnDataChangedListener() {
                    @Override
                    public void onDataChanged(PickerFragment<?> fragment) {
                        blocker.signal();
                    }
                });
                fragment.setExtraFields(Arrays.asList("first_name"));
                fragment.loadData(true);
            }
        });

        // We should have at least one item in the list by now.
        ListView listView = (ListView) fragment.getView().findViewById(R.id.com_facebook_picker_list_view);
        assertNotNull(listView);
        View firstChild = listView.getChildAt(0);
        assertNotNull(firstChild);

        // Assert our state before we touch anything.
        CheckBox checkBox = (CheckBox)listView.findViewById(R.id.com_facebook_picker_checkbox);
        assertNotNull(checkBox);
        assertFalse(checkBox.isChecked());
        assertEquals(0, fragment.getSelection().size());

        // Click on the first item in the list view.
        TouchUtils.clickView(this, firstChild);

        // We should have a selection (it might not be the user we made a friend up above, if the
        // test user has more than one friend).
        assertEquals(1, fragment.getSelection().size());

        // We should have gotten the extra field we wanted.
        assertNotNull(fragment.getSelection().iterator().next().getFirstName());

        // And the checkbox should be checked.
        assertTrue(checkBox.isChecked());

        // Touch the item again. We should go back to no selection.
        TouchUtils.clickView(this, firstChild);
        assertEquals(0, fragment.getSelection().size());
        assertFalse(checkBox.isChecked());
    }

    public static class TestActivity extends FragmentTestCase.TestFragmentActivity<FriendPickerFragment> {
        public TestActivity() {
            super(FriendPickerFragment.class);
        }

        @Override
        protected boolean getAutoCreateUI() {
            return false;
        }
    }
}
