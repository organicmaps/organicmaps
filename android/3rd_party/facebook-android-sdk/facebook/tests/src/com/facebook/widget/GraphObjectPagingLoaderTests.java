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
import android.support.v4.app.Fragment;
import android.support.v4.app.LoaderManager;
import android.support.v4.content.Loader;
import android.test.suitebuilder.annotation.LargeTest;
import android.test.suitebuilder.annotation.MediumTest;
import com.facebook.*;
import com.facebook.model.GraphPlace;

public class GraphObjectPagingLoaderTests extends FragmentTestCase<GraphObjectPagingLoaderTests.TestActivity> {
    public GraphObjectPagingLoaderTests() {
        super(TestActivity.class);
    }

    @MediumTest
    @LargeTest
    public void testLoaderLoadsAndFollowsNextLinks() throws Exception {
        CountingCallback callback = new CountingCallback();
        final GraphObjectPagingLoader<GraphPlace> loader = (GraphObjectPagingLoader<GraphPlace>)
                getActivity().getSupportLoaderManager().initLoader(0, null, callback);

        TestSession session = openTestSessionWithSharedUser();

        Location location = new Location("");
        location.setLatitude(47.6204);
        location.setLongitude(-122.3491);

        final Request request = Request.newPlacesSearchRequest(session, location, 1000, 5, null, null);

        // Need to run this on blocker thread so callbacks are made there.
        runOnBlockerThread(new Runnable() {
            @Override
            public void run() {
                loader.startLoading(request, false);
            }
        }, false);

        getTestBlocker().waitForSignals(1);
        assertEquals(1, callback.onLoadFinishedCount);
        assertEquals(0, callback.onErrorCount);
        assertEquals(0, callback.onLoadResetCount);
        // We might not get back the exact number we requested because of privacy or other rules on
        // the service side.
        assertNotNull(callback.results);
        assertTrue(callback.results.getCount() > 0);

        runOnBlockerThread(new Runnable() {
            @Override
            public void run() {
                loader.followNextLink();
            }
        }, false);
        getTestBlocker().waitForSignals(1);
        assertEquals(2, callback.onLoadFinishedCount);
        assertEquals(0, callback.onErrorCount);
        assertEquals(0, callback.onLoadResetCount);
    }

    @MediumTest
    @LargeTest
    public void testLoaderFinishesImmediatelyOnNoResults() throws Exception {
        CountingCallback callback = new CountingCallback();
        final GraphObjectPagingLoader<GraphPlace> loader = (GraphObjectPagingLoader<GraphPlace>)
                getActivity().getSupportLoaderManager().initLoader(0, null, callback);

        TestSession session = openTestSessionWithSharedUser();

        // Unlikely to ever be a Place here.
        Location location = new Location("");
        location.setLatitude(-1.0);
        location.setLongitude(-1.0);

        final Request request = Request.newPlacesSearchRequest(session, location, 10, 5, null, null);

        // Need to run this on blocker thread so callbacks are made there.
        runOnBlockerThread(new Runnable() {
            @Override
            public void run() {
                loader.startLoading(request, false);
            }
        }, false);

        getTestBlocker().waitForSignals(1);
        assertEquals(1, callback.onLoadFinishedCount);
        assertEquals(0, callback.onErrorCount);
        assertEquals(0, callback.onLoadResetCount);
        assertNotNull(callback.results);
        assertEquals(0, callback.results.getCount());
    }

    private class CountingCallback implements
            GraphObjectPagingLoader.OnErrorListener, LoaderManager.LoaderCallbacks<SimpleGraphObjectCursor<GraphPlace>> {
        public int onLoadFinishedCount;
        public int onLoadResetCount;
        public int onErrorCount;
        public SimpleGraphObjectCursor<GraphPlace> results;

        private TestBlocker testBlocker = getTestBlocker();

        @Override
        public void onError(FacebookException error, GraphObjectPagingLoader<?> loader) {
            ++onErrorCount;
            testBlocker.signal();
        }

        @Override
        public Loader<SimpleGraphObjectCursor<GraphPlace>> onCreateLoader(int id, Bundle args) {
            GraphObjectPagingLoader<GraphPlace> loader = new GraphObjectPagingLoader<GraphPlace>(getActivity(),
                    GraphPlace.class);
            loader.setOnErrorListener(this);
            return loader;
        }

        @Override
        public void onLoadFinished(Loader<SimpleGraphObjectCursor<GraphPlace>> loader,
                SimpleGraphObjectCursor<GraphPlace> data) {
            results = data;
            ++onLoadFinishedCount;
            testBlocker.signal();
        }

        @Override
        public void onLoaderReset(Loader<SimpleGraphObjectCursor<GraphPlace>> loader) {
            ++onLoadResetCount;
            testBlocker.signal();
        }
    }

    public static class DummyFragment extends Fragment  {
    }

    public static class TestActivity extends FragmentTestCase.TestFragmentActivity<DummyFragment> {
        public TestActivity() {
            super(DummyFragment.class);
        }

        @Override
        protected boolean getAutoCreateUI() {
            return false;
        }
    }

}
