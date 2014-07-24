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

import android.graphics.Bitmap;
import android.test.suitebuilder.annotation.LargeTest;
import android.test.suitebuilder.annotation.MediumTest;
import android.test.suitebuilder.annotation.SmallTest;
import com.facebook.model.GraphObject;
import com.facebook.model.GraphPlace;
import com.facebook.model.GraphUser;

import java.net.HttpURLConnection;
import java.util.Arrays;
import java.util.List;

public class AsyncRequestTests extends FacebookTestCase {

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCanLaunchAsyncRequestFromUiThread() {
        Request request = Request.newPostRequest(null, "me/feeds", null, null);
        try {
            TestRequestAsyncTask task = createAsyncTaskOnUiThread(request);
            assertNotNull(task);
        } catch (Throwable throwable) {
            assertNull(throwable);
        }
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testExecuteWithNullRequestsThrows() throws Exception {
        try {
            TestRequestAsyncTask task = new TestRequestAsyncTask((Request[]) null);

            task.executeOnBlockerThread();

            waitAndAssertSuccessOrRethrow(1);

            fail("expected NullPointerException");
        } catch (NullPointerException exception) {
        }
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testExecuteBatchWithZeroRequestsThrows() throws Exception {
        try {
            TestRequestAsyncTask task = new TestRequestAsyncTask(new Request[] {});

            task.executeOnBlockerThread();

            waitAndAssertSuccessOrRethrow(1);

            fail("expected IllegalArgumentException");
        } catch (IllegalArgumentException exception) {
        }
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testExecuteBatchWithNullRequestThrows() throws Exception {
        try {
            TestRequestAsyncTask task = new TestRequestAsyncTask(new Request[] { null });

            task.executeOnBlockerThread();

            waitAndAssertSuccessOrRethrow(1);

            fail("expected NullPointerException");
        } catch (NullPointerException exception) {
        }

    }

    @MediumTest
    @LargeTest
    public void testExecuteSingleGet() {
        final TestSession session = openTestSessionWithSharedUser();
        Request request = new Request(session, "TourEiffel", null, null, new ExpectSuccessCallback() {
            @Override
            protected void performAsserts(Response response) {
                assertNotNull(response);
                GraphPlace graphPlace = response.getGraphObjectAs(GraphPlace.class);
                assertEquals("Paris", graphPlace.getLocation().getCity());
            }
        });

        TestRequestAsyncTask task = new TestRequestAsyncTask(request);

        task.executeOnBlockerThread();

        // Wait on 2 signals: request and task will both signal.
        waitAndAssertSuccess(2);
    }

    @MediumTest
    @LargeTest
    public void testExecuteSingleGetUsingHttpURLConnection() {
        final TestSession session = openTestSessionWithSharedUser();
        Request request = new Request(session, "TourEiffel", null, null, new ExpectSuccessCallback() {
            @Override
            protected void performAsserts(Response response) {
                assertNotNull(response);
                GraphPlace graphPlace = response.getGraphObjectAs(GraphPlace.class);
                assertEquals("Paris", graphPlace.getLocation().getCity());
            }
        });
        HttpURLConnection connection = Request.toHttpConnection(request);

        TestRequestAsyncTask task = new TestRequestAsyncTask(connection, Arrays.asList(new Request[] { request }));

        task.executeOnBlockerThread();

        // Wait on 2 signals: request and task will both signal.
        waitAndAssertSuccess(2);
    }

    @MediumTest
    @LargeTest
    public void testExecuteSingleGetFailureCase() {
        final TestSession session = openTestSessionWithSharedUser();
        Request request = new Request(session, "-1", null, null, new ExpectFailureCallback());

        TestRequestAsyncTask task = new TestRequestAsyncTask(request);

        task.executeOnBlockerThread();

        // Wait on 2 signals: request and task will both signal.
        waitAndAssertSuccess(2);
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testBatchWithoutAppIDIsError() throws Throwable {
        Request request1 = new Request(null, "TourEiffel", null, null, new ExpectFailureCallback());
        Request request2 = new Request(null, "SpaceNeedle", null, null, new ExpectFailureCallback());

        TestRequestAsyncTask task = new TestRequestAsyncTask(request1, request2);

        task.executeOnBlockerThread();

        // Wait on 3 signals: request1, request2, and task will all signal.
        waitAndAssertSuccessOrRethrow(3);
    }

    @LargeTest
    public void testMixedSuccessAndFailure() {
        TestSession session = openTestSessionWithSharedUser();

        final int NUM_REQUESTS = 8;
        Request[] requests = new Request[NUM_REQUESTS];
        for (int i = 0; i < NUM_REQUESTS; ++i) {
            boolean shouldSucceed = (i % 2) == 1;
            if (shouldSucceed) {
                requests[i] = new Request(session, "me", null, null, new ExpectSuccessCallback());
            } else {
                requests[i] = new Request(session, "-1", null, null, new ExpectFailureCallback());
            }
        }

        TestRequestAsyncTask task = new TestRequestAsyncTask(requests);

        task.executeOnBlockerThread();

        // Note: plus 1, because the overall async task signals as well.
        waitAndAssertSuccess(NUM_REQUESTS + 1);
    }

    @MediumTest
    @LargeTest
    @SuppressWarnings("deprecation")
    public void testStaticExecuteMeAsync() {
        final TestSession session = openTestSessionWithSharedUser();

        class MeCallback extends ExpectSuccessCallback implements Request.GraphUserCallback {
            @Override
            public void onCompleted(GraphUser me, Response response) {
                assertNotNull(me);
                assertEquals(session.getTestUserId(), me.getId());
                RequestTests.validateMeResponse(session, response);
                onCompleted(response);
            }
        }

        runOnBlockerThread(new Runnable() {
            @Override
            public void run() {
                Request.executeMeRequestAsync(session, new MeCallback());
            }
        }, false);
        waitAndAssertSuccess(1);
    }

    @MediumTest
    @LargeTest
    @SuppressWarnings("deprecation")
    public void testStaticExecuteMyFriendsAsync() {
        final TestSession session = openTestSessionWithSharedUser();

        class FriendsCallback extends ExpectSuccessCallback implements Request.GraphUserListCallback {
            @Override
            public void onCompleted(List<GraphUser> friends, Response response) {
                assertNotNull(friends);
                RequestTests.validateMyFriendsResponse(session, response);
                onCompleted(response);
            }
        }

        runOnBlockerThread(new Runnable() {
            @Override
            public void run() {
                Request.executeMyFriendsRequestAsync(session, new FriendsCallback());
            }
        }, false);
        waitAndAssertSuccess(1);
    }

    @LargeTest
    public void testBatchUploadPhoto() {
        TestSession session = openTestSessionWithSharedUserAndPermissions(null, "user_photos");

        final int image1Size = 120;
        final int image2Size = 150;

        Bitmap bitmap1 = createTestBitmap(image1Size);
        Bitmap bitmap2 = createTestBitmap(image2Size);

        Request uploadRequest1 = Request.newUploadPhotoRequest(session, bitmap1, null);
        uploadRequest1.setBatchEntryName("uploadRequest1");
        Request uploadRequest2 = Request.newUploadPhotoRequest(session, bitmap2, null);
        uploadRequest2.setBatchEntryName("uploadRequest2");
        Request getRequest1 = new Request(session, "{result=uploadRequest1:$.id}", null, null,
                new ExpectSuccessCallback() {
                    @Override
                    protected void performAsserts(Response response) {
                        assertNotNull(response);
                        GraphObject retrievedPhoto = response.getGraphObject();
                        assertNotNull(retrievedPhoto);
                        assertEquals(image1Size, retrievedPhoto.getProperty("width"));
                    }
                });
        Request getRequest2 = new Request(session, "{result=uploadRequest2:$.id}", null, null,
                new ExpectSuccessCallback() {
                    @Override
                    protected void performAsserts(Response response) {
                        assertNotNull(response);
                        GraphObject retrievedPhoto = response.getGraphObject();
                        assertNotNull(retrievedPhoto);
                        assertEquals(image2Size, retrievedPhoto.getProperty("width"));
                    }
                });

        TestRequestAsyncTask task = new TestRequestAsyncTask(uploadRequest1, uploadRequest2, getRequest1, getRequest2);
        task.executeOnBlockerThread();

        // Wait on 3 signals: getRequest1, getRequest2, and task will all signal.
        waitAndAssertSuccess(3);
    }

    @MediumTest
    @LargeTest
    public void testShortTimeoutCausesFailure() {
        TestSession session = openTestSessionWithSharedUser();

        Request request = new Request(session, "me/likes", null, null, new ExpectFailureCallback());

        RequestBatch requestBatch = new RequestBatch(request);

        // 1 millisecond timeout should be too short for response from server.
        requestBatch.setTimeout(1);

        TestRequestAsyncTask task = new TestRequestAsyncTask(requestBatch);
        task.executeOnBlockerThread();

        // Note: plus 1, because the overall async task signals as well.
        waitAndAssertSuccess(2);
    }

    @LargeTest
    public void testLongTimeoutAllowsSuccess() {
        TestSession session = openTestSessionWithSharedUser();

        Request request = new Request(session, "me", null, null, new ExpectSuccessCallback());

        RequestBatch requestBatch = new RequestBatch(request);

        // 10 second timeout should be long enough for successful response from server.
        requestBatch.setTimeout(10000);

        TestRequestAsyncTask task = new TestRequestAsyncTask(requestBatch);
        task.executeOnBlockerThread();

        // Note: plus 1, because the overall async task signals as well.
        waitAndAssertSuccess(2);
    }
}
