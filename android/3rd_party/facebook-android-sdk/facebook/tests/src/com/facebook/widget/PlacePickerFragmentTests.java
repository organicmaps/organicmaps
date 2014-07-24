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

import android.location.Location;
import android.os.Bundle;
import android.test.TouchUtils;
import android.test.suitebuilder.annotation.LargeTest;
import android.test.suitebuilder.annotation.MediumTest;
import android.view.View;
import android.widget.ListView;
import com.facebook.*;
import com.facebook.sdk.tests.R;

import java.util.Collection;

public class PlacePickerFragmentTests extends FragmentTestCase<PlacePickerFragmentTests.TestActivity> {
    public PlacePickerFragmentTests() {
        super(TestActivity.class);
    }

    @MediumTest
    @LargeTest
    public void testCanSetParametersProgrammatically() throws Throwable {
        TestActivity activity = getActivity();
        assertNotNull(activity);

        final Location location = new Location("");
        location.setLatitude(47.6204);
        location.setLongitude(-122.3491);

        runAndBlockOnUiThread(0, new Runnable() {
            @Override
            public void run() {
                Bundle bundle = new Bundle();
                // We deliberately set these to non-defaults to ensure they are set correctly.
                bundle.putBoolean(PlacePickerFragment.SHOW_PICTURES_BUNDLE_KEY, false);
                bundle.putInt(PlacePickerFragment.RADIUS_IN_METERS_BUNDLE_KEY, 75);
                bundle.putInt(PlacePickerFragment.RESULTS_LIMIT_BUNDLE_KEY, 5);
                bundle.putString(PlacePickerFragment.SEARCH_TEXT_BUNDLE_KEY, "coffee");
                bundle.putParcelable(PlacePickerFragment.LOCATION_BUNDLE_KEY, location);
                bundle.putString(FriendPickerFragment.EXTRA_FIELDS_BUNDLE_KEY, "checkins,general_info");

                PlacePickerFragment fragment = new PlacePickerFragment(bundle);
                getActivity().setContentToFragment(fragment);
            }
        });

        // We don't just test the fragment we created directly above, because we want it to go through the
        // activity lifecycle and ensure the settings are still correct.
        final PlacePickerFragment fragment = activity.getFragment();
        assertNotNull(fragment);

        assertEquals(false, fragment.getShowPictures());
        assertEquals(75, fragment.getRadiusInMeters());
        assertEquals(5, fragment.getResultsLimit());
        assertEquals("coffee", fragment.getSearchText());
        assertEquals(location, fragment.getLocation());
        Collection<String> extraFields = fragment.getExtraFields();
        assertTrue(extraFields.contains("checkins"));
        assertTrue(extraFields.contains("general_info"));
    }

    @MediumTest
    @LargeTest
    public void testCanSetParametersViaLayout() throws Throwable {
        TestActivity activity = getActivity();
        assertNotNull(activity);

        runAndBlockOnUiThread(0, new Runnable() {
            @Override
            public void run() {
                getActivity().setContentToLayout(R.layout.place_picker_test_layout_1, R.id.place_picker_fragment);
            }
        });

        final PlacePickerFragment fragment = activity.getFragment();
        assertNotNull(fragment);

        assertEquals(false, fragment.getShowPictures());
        assertEquals(75, fragment.getRadiusInMeters());
        assertEquals(5, fragment.getResultsLimit());
        assertEquals("coffee", fragment.getSearchText());
        Collection<String> extraFields = fragment.getExtraFields();
        assertTrue(extraFields.contains("checkins"));
        assertTrue(extraFields.contains("general_info"));
        // It doesn't make sense to specify location via layout, so we don't support it.
    }

    @LargeTest
    public void testPlacesLoad() throws Throwable {
        TestActivity activity = getActivity();
        assertNotNull(activity);

        // We don't auto-create any UI, so do it now. Needs to run on the UI thread.
        runAndBlockOnUiThread(0, new Runnable() {
            @Override
            public void run() {
                getActivity().setContentToFragment(null);
            }
        });
        getInstrumentation().waitForIdleSync();

        final PlacePickerFragment fragment = activity.getFragment();
        assertNotNull(fragment);

        final TestSession session = openTestSessionWithSharedUser();

        // Trigger a data load (on the UI thread).
        final TestBlocker blocker = getTestBlocker();
        runAndBlockOnUiThread(1, new Runnable() {
            @Override
            public void run() {
                fragment.setSession(session);

                Location location = new Location("");
                location.setLatitude(47.6204);
                location.setLongitude(-122.3491);
                fragment.setLocation(location);

                fragment.setOnDataChangedListener(new PickerFragment.OnDataChangedListener() {
                    @Override
                    public void onDataChanged(PickerFragment<?> fragment) {
                        blocker.signal();
                    }
                });
                fragment.setOnErrorListener(new PickerFragment.OnErrorListener() {
                    @Override
                    public void onError(PickerFragment<?> fragment, FacebookException error) {
                        fail("Got unexpected error: " + error.toString());
                    }
                });
                fragment.loadData(true);
            }
        });

        // We should have at least one item in the list by now.
        ListView listView = (ListView) fragment.getView().findViewById(R.id.com_facebook_picker_list_view);
        assertNotNull(listView);
        View firstChild = listView.getChildAt(0);
        assertNotNull(firstChild);

        // Assert our state before we touch anything.
        assertNull(fragment.getSelection());

        // Click on the first item in the list view.
        TouchUtils.clickView(this, firstChild);

        // We should have a selection.
        assertNotNull(fragment.getSelection());

        // Touch the item again. We should go back to no selection.
        TouchUtils.clickView(this, firstChild);
        assertNull(fragment.getSelection());
    }

    @LargeTest
    public void testClearsResultsWhenSessionClosed() throws Throwable {
        TestActivity activity = getActivity();
        assertNotNull(activity);

        // We don't auto-create any UI, so do it now. Needs to run on the UI thread.
        runAndBlockOnUiThread(0, new Runnable() {
            @Override
            public void run() {
                getActivity().setContentToFragment(null);
            }
        });
        getInstrumentation().waitForIdleSync();

        final PlacePickerFragment fragment = activity.getFragment();
        assertNotNull(fragment);

        final TestSession session = openTestSessionWithSharedUser();

        // Trigger a data load (on the UI thread).
        // We use multiple test blockers to keep the counts from getting confused if other events
        // cause our listeners to fire.
        final TestBlocker blocker1 = TestBlocker.createTestBlocker();
        runAndBlockOnUiThread(0, new Runnable() {
            @Override
            public void run() {
                fragment.setSession(session);

                Location location = new Location("");
                location.setLatitude(47.6204);
                location.setLongitude(-122.3491);
                fragment.setLocation(location);

                fragment.setOnDataChangedListener(new PickerFragment.OnDataChangedListener() {
                    @Override
                    public void onDataChanged(PickerFragment<?> fragment) {
                        blocker1.signal();
                    }
                });
                fragment.setOnErrorListener(new PickerFragment.OnErrorListener() {
                    @Override
                    public void onError(PickerFragment<?> fragment, FacebookException error) {
                        fail("Got unexpected error: " + error.getMessage());
                    }
                });
                fragment.loadData(true);
            }
        });
        blocker1.waitForSignals(1);

        // We should have at least one item in the list by now.
        ListView listView = (ListView) fragment.getView().findViewById(R.id.com_facebook_picker_list_view);
        assertNotNull(listView);

        Thread.sleep(500);

        int lastPosition = listView.getLastVisiblePosition();
        assertTrue(lastPosition > -1);

        View firstChild = listView.getChildAt(0);
        assertNotNull(firstChild);

        // Assert our state before we touch anything.
        assertNull(fragment.getSelection());

        // Click on the first item in the list view.
        TouchUtils.clickView(this, firstChild);

        // We should have a selection.
        assertNotNull(fragment.getSelection());

        // To validate the behavior, we need to wait until the session state notifications have been processed.
        // We run this on the UI thread but don't wait on the blocker until we've closed the session.
        final TestBlocker blocker2 = TestBlocker.createTestBlocker();
        runAndBlockOnUiThread(0, new Runnable() {
            @Override
            public void run() {
                session.addCallback(new Session.StatusCallback() {
                    @Override
                    public void call(Session session, SessionState state, Exception exception) {
                        blocker2.signal();
                    }
                });
            }
        });
        session.close();
        // Wait for the notification and for any UI activity to stop.
        blocker2.waitForSignals(1);
        getInstrumentation().waitForIdleSync();

        Thread.sleep(500);
        // The list and the selection should have been cleared.
        lastPosition = listView.getLastVisiblePosition();
        assertTrue(lastPosition == -1);
        assertNull(fragment.getSelection());
    }

    public static class TestActivity extends FragmentTestCase.TestFragmentActivity<PlacePickerFragment> {
        public TestActivity() {
            super(PlacePickerFragment.class);
        }

        @Override
        protected boolean getAutoCreateUI() {
            return false;
        }

        protected PlacePickerFragment createFragment() throws InstantiationException, IllegalAccessException {
            Bundle bundle = new Bundle();
            bundle.putBoolean(PlacePickerFragment.SHOW_SEARCH_BOX_BUNDLE_KEY, false);

            return new PlacePickerFragment(bundle);
        }

    }

}
