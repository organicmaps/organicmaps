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
import com.facebook.RequestBatch;
import com.facebook.model.GraphObject;
import com.facebook.model.GraphPlace;
import com.facebook.model.GraphUser;
import com.facebook.internal.CacheableRequestBatch;

import java.io.IOException;
import java.lang.Override;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

public class BatchRequestTests extends FacebookTestCase {
    protected void setUp() throws Exception {
        super.setUp();

        // Tests that need this set should explicitly set it.
        Request.setDefaultBatchApplicationId(null);
    }

    protected String[] getPermissionsForDefaultTestSession()
    {
        return new String[] { "email", "publish_actions", "read_stream" };
    };

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCreateEmptyRequestBatch() {
        CacheableRequestBatch batch = new CacheableRequestBatch();

        Request meRequest = Request.newMeRequest(null, null);
        assertEquals(0, batch.size());
        batch.add(meRequest);
        assertEquals(1, batch.size());
        assertEquals(meRequest, batch.get(0));

        String key = "The Key";
        assertNull(batch.getCacheKeyOverride());
        batch.setCacheKeyOverride(key);
        assertEquals(key, batch.getCacheKeyOverride());

        assertTrue(!batch.getForceRoundTrip());
        batch.setForceRoundTrip(true);
        assertTrue(batch.getForceRoundTrip());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCreateNonemptyRequestBatch() {
        Request meRequest = Request.newMeRequest(null, null);

        RequestBatch batch = new RequestBatch(new Request[] { meRequest, meRequest });
        assertEquals(2, batch.size());
        assertEquals(meRequest, batch.get(0));
        assertEquals(meRequest, batch.get(1));
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testBatchWithoutAppIDIsError() {
        Request request1 = new Request(null, "TourEiffel", null, null, new ExpectFailureCallback());
        Request request2 = new Request(null, "SpaceNeedle", null, null, new ExpectFailureCallback());
        Request.executeBatchAndWait(request1, request2);
    }

    @MediumTest
    @LargeTest
    public void testExecuteBatchRequestsPathEncoding() throws IOException {
        // ensures that paths passed to batch requests are encoded properly before
        // we send it up to the server

        TestSession session = openTestSessionWithSharedUser();

        Request request1 = new Request(session, "TourEiffel");
        request1.setBatchEntryName("eiffel");
        request1.setBatchEntryOmitResultOnSuccess(false);
        Request request2 = new Request(session, "{result=eiffel:$.id}");

        List<Response> responses = Request.executeBatchAndWait(request1, request2);
        assertEquals(2, responses.size());
        assertTrue(responses.get(0).getError() == null);
        assertTrue(responses.get(1).getError() == null);

        GraphPlace eiffelTower1 = responses.get(0).getGraphObjectAs(GraphPlace.class);
        GraphPlace eiffelTower2 = responses.get(1).getGraphObjectAs(GraphPlace.class);
        assertTrue(eiffelTower1 != null);
        assertTrue(eiffelTower2 != null);

        assertEquals("Paris", eiffelTower1.getLocation().getCity());
        assertEquals("Paris", eiffelTower2.getLocation().getCity());
    }

    @MediumTest
    @LargeTest
    public void testExecuteBatchedGets() throws IOException {
        TestSession session = openTestSessionWithSharedUser();

        Request request1 = new Request(session, "TourEiffel");
        Request request2 = new Request(session, "SpaceNeedle");

        List<Response> responses = Request.executeBatchAndWait(request1, request2);
        assertEquals(2, responses.size());
        assertTrue(responses.get(0).getError() == null);
        assertTrue(responses.get(1).getError() == null);

        GraphPlace eiffelTower = responses.get(0).getGraphObjectAs(GraphPlace.class);
        GraphPlace spaceNeedle = responses.get(1).getGraphObjectAs(GraphPlace.class);
        assertTrue(eiffelTower != null);
        assertTrue(spaceNeedle != null);

        assertEquals("Paris", eiffelTower.getLocation().getCity());
        assertEquals("Seattle", spaceNeedle.getLocation().getCity());
    }

    @MediumTest
    @LargeTest
    public void testFacebookErrorResponsesCreateErrors() {
        setBatchApplicationIdForTestApp();

        Request request1 = new Request(null, "somestringthatshouldneverbeavalidfobjectid");
        Request request2 = new Request(null, "someotherstringthatshouldneverbeavalidfobjectid");
        List<Response> responses = Request.executeBatchAndWait(request1, request2);

        assertEquals(2, responses.size());
        assertTrue(responses.get(0).getError() != null);
        assertTrue(responses.get(1).getError() != null);

        FacebookRequestError error = responses.get(0).getError();
        assertTrue(error.getException() instanceof FacebookServiceException);
        assertTrue(error.getErrorType() != null);
        assertTrue(error.getErrorCode() != FacebookRequestError.INVALID_ERROR_CODE);
    }

    @LargeTest
    public void testBatchPostStatusUpdate() {
        TestSession session = openTestSessionWithSharedUser();

        GraphObject statusUpdate1 = createStatusUpdate("1");
        GraphObject statusUpdate2 = createStatusUpdate("2");

        Request postRequest1 = Request.newPostRequest(session, "me/feed", statusUpdate1, null);
        postRequest1.setBatchEntryName("postRequest1");
        postRequest1.setBatchEntryOmitResultOnSuccess(false);
        Request postRequest2 = Request.newPostRequest(session, "me/feed", statusUpdate2, null);
        postRequest2.setBatchEntryName("postRequest2");
        postRequest2.setBatchEntryOmitResultOnSuccess(false);
        Request getRequest1 = new Request(session, "{result=postRequest1:$.id}");
        Request getRequest2 = new Request(session, "{result=postRequest2:$.id}");

        List<Response> responses = Request.executeBatchAndWait(postRequest1, postRequest2, getRequest1, getRequest2);
        assertNotNull(responses);
        assertEquals(4, responses.size());
        assertNoErrors(responses);

        GraphObject retrievedStatusUpdate1 = responses.get(2).getGraphObject();
        GraphObject retrievedStatusUpdate2 = responses.get(3).getGraphObject();
        assertNotNull(retrievedStatusUpdate1);
        assertNotNull(retrievedStatusUpdate2);

        assertEquals(statusUpdate1.getProperty("message"), retrievedStatusUpdate1.getProperty("message"));
        assertEquals(statusUpdate2.getProperty("message"), retrievedStatusUpdate2.getProperty("message"));
    }

    @LargeTest
    public void testTwoDifferentAccessTokens() {
        TestSession session1 = openTestSessionWithSharedUser();
        TestSession session2 = openTestSessionWithSharedUser(SECOND_TEST_USER_TAG);

        Request request1 = Request.newMeRequest(session1, null);
        Request request2 = Request.newMeRequest(session2, null);

        List<Response> responses = Request.executeBatchAndWait(request1, request2);
        assertNotNull(responses);
        assertEquals(2, responses.size());

        GraphUser user1 = responses.get(0).getGraphObjectAs(GraphUser.class);
        GraphUser user2 = responses.get(1).getGraphObjectAs(GraphUser.class);

        assertNotNull(user1);
        assertNotNull(user2);

        assertFalse(user1.getId().equals(user2.getId()));
        assertEquals(session1.getTestUserId(), user1.getId());
        assertEquals(session2.getTestUserId(), user2.getId());
    }

    @LargeTest
    public void testBatchWithValidSessionAndNoSession() {
        TestSession session = openTestSessionWithSharedUser();

        Request request1 = new Request(session, "me");
        Request request2 = new Request(null, "me");

        List<Response> responses = Request.executeBatchAndWait(request1, request2);
        assertNotNull(responses);
        assertEquals(2, responses.size());

        GraphUser user1 = responses.get(0).getGraphObjectAs(GraphUser.class);
        GraphUser user2 = responses.get(1).getGraphObjectAs(GraphUser.class);

        assertNotNull(user1);
        assertNull(user2);

        assertEquals(session.getTestUserId(), user1.getId());
    }

    @LargeTest
    public void testBatchWithNoSessionAndValidSession() {
        TestSession session = openTestSessionWithSharedUser();

        Request request1 = new Request(null, "me");
        Request request2 = new Request(session, "me");

        List<Response> responses = Request.executeBatchAndWait(request1, request2);
        assertNotNull(responses);
        assertEquals(2, responses.size());

        GraphUser user1 = responses.get(0).getGraphObjectAs(GraphUser.class);
        GraphUser user2 = responses.get(1).getGraphObjectAs(GraphUser.class);

        assertNull(user1);
        assertNotNull(user2);

        assertEquals(session.getTestUserId(), user2.getId());
    }

    @LargeTest
    public void testBatchWithTwoSessionlessRequestsAndDefaultAppID() {
        TestSession session = getTestSessionWithSharedUser(null);
        String appId = session.getApplicationId();
        Request.setDefaultBatchApplicationId(appId);

        Request request1 = new Request(null, "me");
        Request request2 = new Request(null, "me");

        List<Response> responses = Request.executeBatchAndWait(request1, request2);
        assertNotNull(responses);
        assertEquals(2, responses.size());

        GraphUser user1 = responses.get(0).getGraphObjectAs(GraphUser.class);
        GraphUser user2 = responses.get(1).getGraphObjectAs(GraphUser.class);

        assertNull(user1);
        assertNull(user2);
    }

    @LargeTest
    public void testMixedSuccessAndFailure() {
        TestSession session = openTestSessionWithSharedUser();

        final int NUM_REQUESTS = 8;
        Request[] requests = new Request[NUM_REQUESTS];
        for (int i = 0; i < NUM_REQUESTS; ++i) {
            boolean shouldSucceed = (i % 2) == 1;
            requests[i] = new Request(session, shouldSucceed ? "me" : "-1");
        }

        List<Response> responses = Request.executeBatchAndWait(requests);
        assertNotNull(responses);
        assertEquals(NUM_REQUESTS, responses.size());

        for (int i = 0; i < NUM_REQUESTS; ++i) {
            boolean shouldSucceed = (i % 2) == 1;

            Response response = responses.get(i);
            assertNotNull(response);
            if (shouldSucceed) {
                assertNull(response.getError());
                assertNotNull(response.getGraphObject());
            } else {
                assertNotNull(response.getError());
                assertNull(response.getGraphObject());
            }
        }
    }

    @MediumTest
    @LargeTest
    public void testClosedSessionDoesntAppendAccessToken() {
        TestSession session = openTestSessionWithSharedUser();
        session.close();
        Request request1 = new Request(session, "me", null, null, new ExpectFailureCallback());
        Request request2 = new Request(session, "me", null, null, new ExpectFailureCallback());

        TestRequestAsyncTask task = new TestRequestAsyncTask(request1, request2);
        task.executeOnBlockerThread();

        waitAndAssertSuccess(2);
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
        Request getRequest1 = new Request(session, "{result=uploadRequest1:$.id}");
        Request getRequest2 = new Request(session, "{result=uploadRequest2:$.id}");

        List<Response> responses = Request.executeBatchAndWait(uploadRequest1, uploadRequest2, getRequest1, getRequest2);
        assertNotNull(responses);
        assertEquals(4, responses.size());
        assertNoErrors(responses);

        GraphObject retrievedPhoto1 = responses.get(2).getGraphObject();
        GraphObject retrievedPhoto2 = responses.get(3).getGraphObject();
        assertNotNull(retrievedPhoto1);
        assertNotNull(retrievedPhoto2);

        assertEquals(image1Size, retrievedPhoto1.getProperty("width"));
        assertEquals(image2Size, retrievedPhoto2.getProperty("width"));
    }

    @MediumTest
    @LargeTest
    public void testCallbacksAreCalled() {
        setBatchApplicationIdForTestApp();

        ArrayList<Request> requests = new ArrayList<Request>();
        final ArrayList<Boolean> calledBack = new ArrayList<Boolean>();

        final int NUM_REQUESTS = 4;
        for (int i = 0; i < NUM_REQUESTS; ++i) {
            Request request = new Request(null, "4");

            request.setCallback(new Request.Callback() {
                @Override
                public void onCompleted(Response response) {
                    calledBack.add(true);
                }
            });

            requests.add(request);
        }

        List<Response> responses = Request.executeBatchAndWait(requests);
        assertNotNull(responses);
        assertTrue(calledBack.size() == NUM_REQUESTS);
    }

    @MediumTest
    @LargeTest
    public void testCacheMyFriendsRequest() throws Exception {
        TestUtils.clearFileLruCache(Response.getResponseCache());
        TestSession session = openTestSessionWithSharedUser();

        Request request = Request.newMyFriendsRequest(session, null);

        CacheableRequestBatch batch = new CacheableRequestBatch(request);
        batch.setCacheKeyOverride("MyFriends");

        // Running the request with empty cache should hit the server.
        List<Response> responses = Request.executeBatchAndWait(batch);
        assertNotNull(responses);
        assertEquals(1, responses.size());

        Response response = responses.get(0);
        assertNotNull(response);
        assertNull(response.getError());
        assertTrue(!response.getIsFromCache());

        // Running again should hit the cache.
        responses = Request.executeBatchAndWait(batch);
        assertNotNull(responses);
        assertEquals(1, responses.size());

        response = responses.get(0);
        assertNotNull(response);
        assertNull(response.getError());
        assertTrue(response.getIsFromCache());

        // Forcing roundtrip should hit the server again.
        batch.setForceRoundTrip(true);
        responses = Request.executeBatchAndWait(batch);
        assertNotNull(responses);
        assertEquals(1, responses.size());

        response = responses.get(0);
        assertNotNull(response);
        assertNull(response.getError());
        assertTrue(!response.getIsFromCache());

        TestUtils.clearFileLruCache(Response.getResponseCache());
    }

    @MediumTest
    @LargeTest
    public void testCacheMeAndMyFriendsRequest() throws Exception {
        TestUtils.clearFileLruCache(Response.getResponseCache());
        TestSession session = openTestSessionWithSharedUser();

        Request requestMe = Request.newMeRequest(session, null);
        Request requestMyFriends = Request.newMyFriendsRequest(session, null);

        CacheableRequestBatch batch = new CacheableRequestBatch(new Request[] { requestMyFriends, requestMe });
        batch.setCacheKeyOverride("MyFriends");

        // Running the request with empty cache should hit the server.
        List<Response> responses = Request.executeBatchAndWait(batch);
        assertNotNull(responses);
        assertEquals(2, responses.size());

        for (Response response : responses) {
            assertNotNull(response);
            assertNull(response.getError());
            assertTrue(!response.getIsFromCache());
        }

        // Running again should hit the cache.
        responses = Request.executeBatchAndWait(batch);
        assertNotNull(responses);
        assertEquals(2, responses.size());

        for (Response response : responses) {
            assertNotNull(response);
            assertNull(response.getError());
            assertTrue(response.getIsFromCache());
        }

        // Forcing roundtrip should hit the server again.
        batch.setForceRoundTrip(true);
        responses = Request.executeBatchAndWait(batch);
        assertNotNull(responses);
        assertEquals(2, responses.size());

        for (Response response : responses) {
            assertNotNull(response);
            assertNull(response.getError());
            assertTrue(!response.getIsFromCache());
        }

        TestUtils.clearFileLruCache(Response.getResponseCache());
    }

    @MediumTest
    @LargeTest
    public void testExplicitDependencyDefaultsToOmitFirstResponse() {
        TestSession session = openTestSessionWithSharedUser();

        Request requestMe = Request.newMeRequest(session, null);
        requestMe.setBatchEntryName("me_request");

        Request requestMyFriends = Request.newMyFriendsRequest(session, null);
        requestMyFriends.setBatchEntryDependsOn("me_request");

        List<Response> responses = Request.executeBatchAndWait(requestMe, requestMyFriends);

        Response meResponse = responses.get(0);
        Response myFriendsResponse = responses.get(1);

        assertNull(meResponse.getGraphObject());
        assertNotNull(myFriendsResponse.getGraphObject());
    }

    @MediumTest
    @LargeTest
    public void testExplicitDependencyCanIncludeFirstResponse() {
        TestSession session = openTestSessionWithSharedUser();

        Request requestMe = Request.newMeRequest(session, null);
        requestMe.setBatchEntryName("me_request");
        requestMe.setBatchEntryOmitResultOnSuccess(false);

        Request requestMyFriends = Request.newMyFriendsRequest(session, null);
        requestMyFriends.setBatchEntryDependsOn("me_request");

        List<Response> responses = Request.executeBatchAndWait(requestMe, requestMyFriends);

        Response meResponse = responses.get(0);
        Response myFriendsResponse = responses.get(1);

        assertNotNull(meResponse.getGraphObject());
        assertNotNull(myFriendsResponse.getGraphObject());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testAddAndRemoveBatchCallbacks() {
        RequestBatch batch = new RequestBatch();

        RequestBatch.Callback callback1 = new RequestBatch.Callback() {
            @Override
            public void onBatchCompleted(RequestBatch batch) {
            }
        };

        RequestBatch.Callback callback2 = new RequestBatch.Callback() {
            @Override
            public void onBatchCompleted(RequestBatch batch) {
            }
        };

        batch.addCallback(callback1);
        batch.addCallback(callback2);

        assertEquals(2, batch.getCallbacks().size());

        batch.removeCallback(callback1);
        batch.removeCallback(callback2);

        assertEquals(0, batch.getCallbacks().size());
    }

    @MediumTest
    @LargeTest
    public void testBatchCallbackIsCalled() {
        final AtomicInteger count = new AtomicInteger();
        Request request1 = Request.newGraphPathRequest(null, "4", new Request.Callback() {
            @Override
            public void onCompleted(Response response) {
                count.incrementAndGet();
            }
        });
        Request request2 = Request.newGraphPathRequest(null, "4", new Request.Callback() {
            @Override
            public void onCompleted(Response response) {
                count.incrementAndGet();
            }
        });

        RequestBatch batch = new RequestBatch(request1, request2);
        batch.addCallback(new RequestBatch.Callback() {
            @Override
            public void onBatchCompleted(RequestBatch batch) {
                count.incrementAndGet();
            }
        });

        batch.executeAndWait();
        assertEquals(3, count.get());
    }

    @MediumTest
    @LargeTest
    public void testBatchOnProgressCallbackIsCalled() {
        final AtomicInteger count = new AtomicInteger();

        TestSession session = getTestSessionWithSharedUser(null);
        String appId = session.getApplicationId();
        Request.setDefaultBatchApplicationId(appId);

        Request request1 = Request.newGraphPathRequest(null, "4", null);
        assertTrue(request1 != null);

        Request request2 = Request.newGraphPathRequest(null, "4", null);
        assertTrue(request2 != null);

        RequestBatch batch = new RequestBatch(request1, request2);
        batch.addCallback(new RequestBatch.OnProgressCallback() {
            @Override
            public void onBatchCompleted(RequestBatch batch) {
            }

            @Override
            public void onBatchProgress(RequestBatch batch, long current, long max) {
                count.incrementAndGet();
            }
        });

        batch.executeAndWait();
        assertTrue(count.get() > 0);
    }

    @MediumTest
    @LargeTest
    public void testBatchLastOnProgressCallbackIsCalledOnce() {
        final AtomicInteger count = new AtomicInteger();

        TestSession session = getTestSessionWithSharedUser(null);
        String appId = session.getApplicationId();
        Request.setDefaultBatchApplicationId(appId);

        Request request1 = Request.newGraphPathRequest(null, "4", null);
        assertTrue(request1 != null);

        Request request2 = Request.newGraphPathRequest(null, "4", null);
        assertTrue(request2 != null);


        RequestBatch batch = new RequestBatch(request1, request2);
        batch.addCallback(new RequestBatch.OnProgressCallback() {
            @Override
            public void onBatchCompleted(RequestBatch batch) {
            }

            @Override
            public void onBatchProgress(RequestBatch batch, long current, long max) {
                if (current == max) {
                    count.incrementAndGet();
                }
                else if (current > max) {
                    count.set(0);
                }
            }
        });

        batch.executeAndWait();
        assertEquals(1, count.get());
    }


    @MediumTest
    @LargeTest
    public void testMixedBatchCallbacks() {
        final AtomicInteger requestProgressCount = new AtomicInteger();
        final AtomicInteger requestCompletedCount = new AtomicInteger();
        final AtomicInteger batchProgressCount = new AtomicInteger();
        final AtomicInteger batchCompletedCount = new AtomicInteger();

        TestSession session = getTestSessionWithSharedUser(null);
        String appId = session.getApplicationId();
        Request.setDefaultBatchApplicationId(appId);

        Request request1 = Request.newGraphPathRequest(null, "4", new Request.OnProgressCallback() {
            @Override
            public void onCompleted(Response response) {
                requestCompletedCount.incrementAndGet();
            }

            @Override
            public void onProgress(long current, long max) {
                if (current == max) {
                    requestProgressCount.incrementAndGet();
                }
                else if (current > max) {
                    requestProgressCount.set(0);
                }
            }
        });
        assertTrue(request1 != null);

        Request request2 = Request.newGraphPathRequest(null, "4", null);
        assertTrue(request2 != null);

        RequestBatch batch = new RequestBatch(request1, request2);
        batch.addCallback(new RequestBatch.OnProgressCallback() {
            @Override
            public void onBatchCompleted(RequestBatch batch) {
                batchCompletedCount.incrementAndGet();
            }

            @Override
            public void onBatchProgress(RequestBatch batch, long current, long max) {
                if (current == max) {
                    batchProgressCount.incrementAndGet();
                } else if (current > max) {
                    batchProgressCount.set(0);
                }
            }
        });

        batch.executeAndWait();
        
        assertEquals(1, requestProgressCount.get());
        assertEquals(1, requestCompletedCount.get());
        assertEquals(1, batchProgressCount.get());
        assertEquals(1, batchCompletedCount.get());
    }
}
