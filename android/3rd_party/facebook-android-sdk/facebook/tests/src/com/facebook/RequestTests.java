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
import android.location.Location;
import android.net.Uri;
import android.os.Bundle;
import android.test.suitebuilder.annotation.LargeTest;
import android.test.suitebuilder.annotation.MediumTest;
import android.test.suitebuilder.annotation.SmallTest;
import com.facebook.internal.ServerProtocol;
import com.facebook.model.*;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.net.HttpURLConnection;
import java.net.URISyntaxException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class RequestTests extends FacebookTestCase {
    private final static String TEST_OG_TYPE = "facebooksdktests:test";

    protected String[] getPermissionsForDefaultTestSession()
    {
        return new String[] { "email", "publish_actions", "read_stream" };
    };

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCreateRequest() {
        Request request = new Request();
        assertTrue(request != null);
        assertEquals(HttpMethod.GET, request.getHttpMethod());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCreatePostRequest() {
        GraphObject graphObject = GraphObject.Factory.create();
        Request request = Request.newPostRequest(null, "me/statuses", graphObject, null);
        assertTrue(request != null);
        assertEquals(HttpMethod.POST, request.getHttpMethod());
        assertEquals("me/statuses", request.getGraphPath());
        assertEquals(graphObject, request.getGraphObject());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCreateMeRequest() {
        Request request = Request.newMeRequest(null, null);
        assertTrue(request != null);
        assertEquals(HttpMethod.GET, request.getHttpMethod());
        assertEquals("me", request.getGraphPath());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCreateMyFriendsRequest() {
        Request request = Request.newMyFriendsRequest(null, null);
        assertTrue(request != null);
        assertEquals(HttpMethod.GET, request.getHttpMethod());
        assertEquals("me/friends", request.getGraphPath());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCreateUploadPhotoRequest() {
        Bitmap image = Bitmap.createBitmap(128, 128, Bitmap.Config.ALPHA_8);

        Request request = Request.newUploadPhotoRequest(null, image, null);
        assertTrue(request != null);

        Bundle parameters = request.getParameters();
        assertTrue(parameters != null);

        assertTrue(parameters.containsKey("picture"));
        assertEquals(image, parameters.getParcelable("picture"));
        assertEquals("me/photos", request.getGraphPath());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCreatePlacesSearchRequestWithLocation() {
        Location location = new Location("");
        location.setLatitude(47.6204);
        location.setLongitude(-122.3491);

        Request request = Request.newPlacesSearchRequest(null, location, 1000, 50, null, null);

        assertTrue(request != null);
        assertEquals(HttpMethod.GET, request.getHttpMethod());
        assertEquals("search", request.getGraphPath());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCreatePlacesSearchRequestWithSearchText() {
        Request request = Request.newPlacesSearchRequest(null, null, 1000, 50, "Starbucks", null);

        assertTrue(request != null);
        assertEquals(HttpMethod.GET, request.getHttpMethod());
        assertEquals("search", request.getGraphPath());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCreatePlacesSearchRequestRequiresLocationOrSearchText() {
        try {
            Request.newPlacesSearchRequest(null, null, 1000, 50, null, null);
            fail("expected exception");
        } catch (FacebookException exception) {
            // Success
        }
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testNewPostOpenGraphObjectRequestRequiresObject() {
        try {
            Request.newPostOpenGraphObjectRequest(null, null, null);
            fail("expected exception");
        } catch (FacebookException exception) {
            // Success
        }
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testNewPostOpenGraphObjectRequestRequiresObjectType() {
        try {
            OpenGraphObject object = OpenGraphObject.Factory.createForPost(null);
            Request.newPostOpenGraphObjectRequest(null, object, null);
            fail("expected exception");
        } catch (FacebookException exception) {
            // Success
        }
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testNewPostOpenGraphObjectRequestRequiresNonEmptyObjectType() {
        try {
            OpenGraphObject object = OpenGraphObject.Factory.createForPost("");
            object.setTitle("bar");
            Request.newPostOpenGraphObjectRequest(null, object, null);
            fail("expected exception");
        } catch (FacebookException exception) {
            // Success
        }
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testNewPostOpenGraphObjectRequestRequiresTitle() {
        try {
            OpenGraphObject object = OpenGraphObject.Factory.createForPost("foo");
            Request.newPostOpenGraphObjectRequest(null, object, null);
            fail("expected exception");
        } catch (FacebookException exception) {
            // Success
        }
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testNewPostOpenGraphObjectRequestRequiresNonEmptyTitle() {
        try {
            OpenGraphObject object = OpenGraphObject.Factory.createForPost("foo");
            object.setTitle("");
            Request.newPostOpenGraphObjectRequest(null, object, null);
            fail("expected exception");
        } catch (FacebookException exception) {
            // Success
        }
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testNewPostOpenGraphObjectRequest() {
        OpenGraphObject object = OpenGraphObject.Factory.createForPost("foo");
        object.setTitle("bar");
        Request request = Request.newPostOpenGraphObjectRequest(null, object, null);
        assertNotNull(request);
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testNewPostOpenGraphActionRequestRequiresAction() {
        try {
            Request.newPostOpenGraphActionRequest(null, null, null);
            fail("expected exception");
        } catch (FacebookException exception) {
            // Success
        }
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testNewPostOpenGraphActionRequestRequiresActionType() {
        try {
            OpenGraphAction action = OpenGraphAction.Factory.createForPost(null);
            Request.newPostOpenGraphActionRequest(null, action, null);
            fail("expected exception");
        } catch (FacebookException exception) {
            // Success
        }
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testNewPostOpenGraphActionRequestRequiresNonEmptyActionType() {
        try {
            OpenGraphAction action = OpenGraphAction.Factory.createForPost("");
            Request.newPostOpenGraphActionRequest(null, action, null);
            fail("expected exception");
        } catch (FacebookException exception) {
            // Success
        }
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testNewPostOpenGraphActionRequest() {
        OpenGraphAction action = OpenGraphAction.Factory.createForPost("foo");
        Request request = Request.newPostOpenGraphActionRequest(null, action, null);
        assertNotNull(request);
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testSetHttpMethodToNilGivesDefault() {
        Request request = new Request();
        assertEquals(HttpMethod.GET, request.getHttpMethod());

        request.setHttpMethod(null);
        assertEquals(HttpMethod.GET, request.getHttpMethod());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testExecuteBatchWithNullRequestsThrows() {
        try {
            Request.executeBatchAndWait((Request[]) null);
            fail("expected NullPointerException");
        } catch (NullPointerException exception) {
        }
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testExecuteBatchWithZeroRequestsThrows() {
        try {
            Request.executeBatchAndWait(new Request[]{});
            fail("expected IllegalArgumentException");
        } catch (IllegalArgumentException exception) {
        }
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testExecuteBatchWithNullRequestThrows() {
        try {
            Request.executeBatchAndWait(new Request[]{null});
            fail("expected NullPointerException");
        } catch (NullPointerException exception) {
        }
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testToHttpConnectionWithNullRequestsThrows() {
        try {
            Request.toHttpConnection((Request[]) null);
            fail("expected NullPointerException");
        } catch (NullPointerException exception) {
        }
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testToHttpConnectionWithZeroRequestsThrows() {
        try {
            Request.toHttpConnection(new Request[]{});
            fail("expected IllegalArgumentException");
        } catch (IllegalArgumentException exception) {
        }
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testToHttpConnectionWithNullRequestThrows() {
        try {
            Request.toHttpConnection(new Request[]{null});
            fail("expected NullPointerException");
        } catch (NullPointerException exception) {
        }
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testSingleGetToHttpRequest() throws Exception {
        Request requestMe = new Request(null, "TourEiffel");
        HttpURLConnection connection = Request.toHttpConnection(requestMe);

        assertTrue(connection != null);

        assertEquals("GET", connection.getRequestMethod());
        assertEquals("/" + ServerProtocol.getAPIVersion() + "/TourEiffel", connection.getURL().getPath());

        assertTrue(connection.getRequestProperty("User-Agent").startsWith("FBAndroidSDK"));

        Uri uri = Uri.parse(connection.getURL().toString());
        assertEquals("android", uri.getQueryParameter("sdk"));
        assertEquals("json", uri.getQueryParameter("format"));
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testBuildsClientTokenIfNeeded() throws Exception {
        Request requestMe = new Request(null, "TourEiffel");
        HttpURLConnection connection = Request.toHttpConnection(requestMe);

        assertTrue(connection != null);

        Uri uri = Uri.parse(connection.getURL().toString());
        String accessToken = uri.getQueryParameter("access_token");
        assertNotNull(accessToken);
        assertTrue(accessToken.contains(Settings.getApplicationId()));
        assertTrue(accessToken.contains(Settings.getClientToken()));
    }

    @MediumTest
    @LargeTest
    public void testExecuteSingleGet() {
        TestSession session = openTestSessionWithSharedUser();
        Request request = new Request(session, "TourEiffel");
        Response response = request.executeAndWait();

        assertTrue(response != null);
        assertTrue(response.getError() == null);
        assertTrue(response.getGraphObject() != null);
        assertNotNull(response.getRawResponse());

        GraphPlace graphPlace = response.getGraphObjectAs(GraphPlace.class);
        assertEquals("Paris", graphPlace.getLocation().getCity());
    }

    @MediumTest
    @LargeTest
    public void testExecuteSingleGetUsingHttpURLConnection() throws IOException {
        TestSession session = openTestSessionWithSharedUser();
        Request request = new Request(session, "TourEiffel");
        HttpURLConnection connection = Request.toHttpConnection(request);

        List<Response> responses = Request.executeConnectionAndWait(connection, Arrays.asList(new Request[]{request}));
        assertNotNull(responses);
        assertEquals(1, responses.size());

        Response response = responses.get(0);

        assertTrue(response != null);
        assertTrue(response.getError() == null);
        assertTrue(response.getGraphObject() != null);
        assertNotNull(response.getRawResponse());

        GraphPlace graphPlace = response.getGraphObjectAs(GraphPlace.class);
        assertEquals("Paris", graphPlace.getLocation().getCity());

        // Make sure calling code can still access HTTP headers and call disconnect themselves.
        int code = connection.getResponseCode();
        assertEquals(200, code);
        assertTrue(connection.getHeaderFields().keySet().contains("Content-Type"));
        connection.disconnect();
    }

    @MediumTest
    @LargeTest
    public void testFacebookErrorResponseCreatesError() {
        Request request = new Request(null, "somestringthatshouldneverbeavalidfobjectid");
        Response response = request.executeAndWait();

        assertTrue(response != null);

        FacebookRequestError error = response.getError();
        assertNotNull(error);
        FacebookException exception = error.getException();
        assertNotNull(exception);

        assertTrue(exception instanceof FacebookServiceException);
        assertNotNull(error.getErrorType());
        assertTrue(error.getErrorCode() != FacebookRequestError.INVALID_ERROR_CODE);
        assertNotNull(error.getRequestResultBody());
    }

    @LargeTest
    public void testFacebookSuccessResponseWithErrorCodeCreatesError() {
        TestSession session = openTestSessionWithSharedUser();

        Request request = Request.newRestRequest(session, "auth.extendSSOAccessToken", null, null);
        assertNotNull(request);

        // Because TestSession access tokens were not created via SSO, we expect to get an error from the service,
        // but with a 200 (success) code.
        Response response = request.executeAndWait();

        assertTrue(response != null);

        FacebookRequestError error = response.getError();
        assertNotNull(error);

        assertTrue(error.getException() instanceof FacebookServiceException);
        assertTrue(error.getErrorCode() != FacebookRequestError.INVALID_ERROR_CODE);
        assertNotNull(error.getRequestResultBody());
    }

    @MediumTest
    @LargeTest
    public void testRequestWithUnopenedSessionFails() {
        TestSession session = getTestSessionWithSharedUser(null);
        Request request = new Request(session, "me");
        Response response = request.executeAndWait();

        assertNotNull(response.getError());
    }

    @MediumTest
    @LargeTest
    public void testExecuteRequestMe() {
        TestSession session = openTestSessionWithSharedUser();
        Request request = Request.newMeRequest(session, null);
        Response response = request.executeAndWait();

        validateMeResponse(session, response);
    }

    static void validateMeResponse(TestSession session, Response response) {
        assertNull(response.getError());

        GraphUser me = response.getGraphObjectAs(GraphUser.class);
        assertNotNull(me);
        assertEquals(session.getTestUserId(), me.getId());
        assertNotNull(response.getRawResponse());
    }

    @MediumTest
    @LargeTest
    public void testExecuteMyFriendsRequest() {
        TestSession session = openTestSessionWithSharedUser();

        Request request = Request.newMyFriendsRequest(session, null);
        Response response = request.executeAndWait();

        validateMyFriendsResponse(session, response);
    }

    static void validateMyFriendsResponse(TestSession session, Response response) {
        assertNotNull(response);

        assertNull(response.getError());

        GraphMultiResult graphResult = response.getGraphObjectAs(GraphMultiResult.class);
        assertNotNull(graphResult);

        List<GraphObject> results = graphResult.getData();
        assertNotNull(results);

        assertNotNull(response.getRawResponse());
    }

    @MediumTest
    @LargeTest
    public void testExecutePlaceRequestWithLocation() {
        TestSession session = openTestSessionWithSharedUser();

        Location location = new Location("");
        location.setLatitude(47.6204);
        location.setLongitude(-122.3491);

        Request request = Request.newPlacesSearchRequest(session, location, 5, 5, null, null);
        Response response = request.executeAndWait();
        assertNotNull(response);

        assertNull(response.getError());

        GraphMultiResult graphResult = response.getGraphObjectAs(GraphMultiResult.class);
        assertNotNull(graphResult);

        List<GraphObject> results = graphResult.getData();
        assertNotNull(results);

        assertNotNull(response.getRawResponse());
    }

    @MediumTest
    @LargeTest
    public void testExecutePlaceRequestWithSearchText() {
        TestSession session = openTestSessionWithSharedUser();

        // Pass a distance without a location to ensure it is correctly ignored.
        Request request = Request.newPlacesSearchRequest(session, null, 1000, 5, "Starbucks", null);
        Response response = request.executeAndWait();
        assertNotNull(response);

        assertNull(response.getError());

        GraphMultiResult graphResult = response.getGraphObjectAs(GraphMultiResult.class);
        assertNotNull(graphResult);

        List<GraphObject> results = graphResult.getData();
        assertNotNull(results);

        assertNotNull(response.getRawResponse());
    }

    @MediumTest
    @LargeTest
    public void testExecutePlaceRequestWithLocationAndSearchText() {
        TestSession session = openTestSessionWithSharedUser();

        Location location = new Location("");
        location.setLatitude(47.6204);
        location.setLongitude(-122.3491);

        Request request = Request.newPlacesSearchRequest(session, location, 1000, 5, "Starbucks", null);
        Response response = request.executeAndWait();
        assertNotNull(response);

        assertNull(response.getError());

        GraphMultiResult graphResult = response.getGraphObjectAs(GraphMultiResult.class);
        assertNotNull(graphResult);

        List<GraphObject> results = graphResult.getData();
        assertNotNull(results);

        assertNotNull(response.getRawResponse());
    }

    private String executePostOpenGraphRequest() {
        TestSession session = openTestSessionWithSharedUser();

        GraphObject data = GraphObject.Factory.create();
        data.setProperty("a_property", "hello");

        Request request = Request.newPostOpenGraphObjectRequest(session, TEST_OG_TYPE, "a title",
                "http://www.facebook.com", "http://www.facebook.com/zzzzzzzzzzzzzzzzzzz", "a description", data, null);
        Response response = request.executeAndWait();
        assertNotNull(response);

        assertNull(response.getError());

        GraphObject graphResult = response.getGraphObject();
        assertNotNull(graphResult);
        assertNotNull(graphResult.getProperty("id"));

        assertNotNull(response.getRawResponse());

        return (String) graphResult.getProperty("id");
    }

    @LargeTest
    public void testExecutePostOpenGraphRequest() {
        executePostOpenGraphRequest();
    }

    @LargeTest
    public void testDeleteObjectRequest() {
        String id = executePostOpenGraphRequest();

        TestSession session = openTestSessionWithSharedUser();
        Request request = Request.newDeleteObjectRequest(session, id, null);
        Response response = request.executeAndWait();
        assertNotNull(response);

        assertNull(response.getError());

        GraphObject result = response.getGraphObject();
        assertNotNull(result);

        assertTrue((Boolean) result.getProperty(Response.NON_JSON_RESPONSE_PROPERTY));
        assertNotNull(response.getRawResponse());
    }

    @LargeTest
    public void testUpdateOpenGraphObjectRequest() {
        String id = executePostOpenGraphRequest();

        GraphObject data = GraphObject.Factory.create();
        data.setProperty("a_property", "goodbye");

        TestSession session = openTestSessionWithSharedUser();
        Request request = Request.newUpdateOpenGraphObjectRequest(session, id, "another title", null,
                "http://www.facebook.com/aaaaaaaaaaaaaaaaa", "another description", data, null);
        Response response = request.executeAndWait();
        assertNotNull(response);

        assertNull(response.getError());

        GraphObject result = response.getGraphObject();
        assertNotNull(result);
        assertNotNull(response.getRawResponse());
    }

    @LargeTest
    public void testExecuteUploadPhoto() {
        TestSession session = openTestSessionWithSharedUser();
        Bitmap image = createTestBitmap(128);

        Request request = Request.newUploadPhotoRequest(session, image, null);
        Response response = request.executeAndWait();
        assertNotNull(response);

        assertNull(response.getError());

        GraphObject result = response.getGraphObject();
        assertNotNull(result);
        assertNotNull(response.getRawResponse());
    }

    @LargeTest
    public void testExecuteUploadPhotoViaFile() throws IOException {
        File outputFile = null;
        FileOutputStream outStream = null;

        try {
            TestSession session = openTestSessionWithSharedUser();
            Bitmap image = createTestBitmap(128);

            File outputDir = getActivity().getCacheDir(); // context being the Activity pointer
            outputFile = File.createTempFile("prefix", "extension", outputDir);

            outStream = new FileOutputStream(outputFile);
            image.compress(Bitmap.CompressFormat.PNG, 100, outStream);
            outStream.close();
            outStream = null;

            Request request = Request.newUploadPhotoRequest(session, outputFile, null);
            Response response = request.executeAndWait();
            assertNotNull(response);

            assertNull(response.getError());

            GraphObject result = response.getGraphObject();
            assertNotNull(result);
            assertNotNull(response.getRawResponse());
        } finally {
            if (outStream != null) {
                outStream.close();
            }
            if (outputFile != null) {
                outputFile.delete();
            }
        }
    }

    @LargeTest
    public void testUploadVideoFile() throws IOException, URISyntaxException {
        File tempFile = null;
        try {
            TestSession session = openTestSessionWithSharedUser();
            tempFile = createTempFileFromAsset("DarkScreen.mov");

            Request request = Request.newUploadVideoRequest(session, tempFile, null);
            Response response = request.executeAndWait();
            assertNotNull(response);

            assertNull(response.getError());

            GraphObject result = response.getGraphObject();
            assertNotNull(result);
            assertNotNull(response.getRawResponse());
        } catch (Exception ex) {
            return;
        } finally {
            if (tempFile != null) {
                tempFile.delete();
            }
        }
    }

    @LargeTest
    public void testPostStatusUpdate() {
        TestSession session = openTestSessionWithSharedUser();

        GraphObject statusUpdate = createStatusUpdate("");

        GraphObject retrievedStatusUpdate = postGetAndAssert(session, "me/feed", statusUpdate);

        assertEquals(statusUpdate.getProperty("message"), retrievedStatusUpdate.getProperty("message"));
    }

    @LargeTest
    public void testRestMethodGetUser() {
        TestSession session = openTestSessionWithSharedUser();
        String testUserId = session.getTestUserId();

        Bundle parameters = new Bundle();
        parameters.putString("uids", testUserId);
        parameters.putString("fields", "uid,name");

        Request request = Request.newRestRequest(session, "users.getInfo", parameters, null);
        Response response = request.executeAndWait();
        assertNotNull(response);

        GraphObjectList<GraphObject> graphObjects = response.getGraphObjectList();
        assertNotNull(graphObjects);
        assertEquals(1, graphObjects.size());

        GraphObject user = graphObjects.get(0);
        assertNotNull(user);
        assertEquals(testUserId, user.getProperty("uid").toString());

        assertNotNull(response.getRawResponse());
    }

    @MediumTest
    @LargeTest
    public void testCallbackIsCalled() {
        Request request = new Request(null, "4");

        final ArrayList<Boolean> calledBack = new ArrayList<Boolean>();
        request.setCallback(new Request.Callback() {
            @Override
            public void onCompleted(Response response) {
                calledBack.add(true);
            }
        });

        Response response = request.executeAndWait();
        assertNotNull(response);
        assertTrue(calledBack.size() == 1);
    }

    @MediumTest
    @LargeTest
    public void testOnProgressCallbackIsCalled() {
        Bitmap image = Bitmap.createBitmap(128, 128, Bitmap.Config.ALPHA_8);

        Request request = Request.newUploadPhotoRequest(null, image, null);
        assertTrue(request != null);

        final ArrayList<Boolean> calledBack = new ArrayList<Boolean>();
        request.setCallback(new Request.OnProgressCallback() {
            @Override
            public void onCompleted(Response response) {
            }

            @Override
            public void onProgress(long current, long max) {
                calledBack.add(true);
            }
        });

        Response response = request.executeAndWait();
        assertNotNull(response);
        assertFalse(calledBack.isEmpty());
    }

    @MediumTest
    @LargeTest
    public void testLastOnProgressCallbackIsCalledOnce() {
        Bitmap image = Bitmap.createBitmap(128, 128, Bitmap.Config.ALPHA_8);

        Request request = Request.newUploadPhotoRequest(null, image, null);
        assertTrue(request != null);

        final ArrayList<Boolean> calledBack = new ArrayList<Boolean>();
        request.setCallback(new Request.OnProgressCallback() {
            @Override
            public void onCompleted(Response response) {
            }

            @Override
            public void onProgress(long current, long max) {
                if (current == max) calledBack.add(true);
                else if (current > max) calledBack.clear();
            }
        });

        Response response = request.executeAndWait();
        assertNotNull(response);
        assertEquals(1, calledBack.size());
    }

    @MediumTest
    @LargeTest
    public void testBatchTimeoutIsApplied() {
        Request request = new Request(null, "me");
        RequestBatch batch = new RequestBatch(request);

        // We assume 1 ms is short enough to fail
        batch.setTimeout(1);

        List<Response> responses = Request.executeBatchAndWait(batch);
        assertNotNull(responses);
        assertTrue(responses.size() == 1);
        Response response = responses.get(0);
        assertNotNull(response);
        assertNotNull(response.getError());
    }

    @MediumTest
    @LargeTest
    public void testBatchTimeoutCantBeNegative() {
        try {
            RequestBatch batch = new RequestBatch();
            batch.setTimeout(-1);
            fail();
        } catch (IllegalArgumentException ex) {
        }
    }

    @MediumTest
    @LargeTest
    public void testCantSetBothGraphPathAndRestMethod() {
        Request request = new Request();
        request.setGraphPath("me");
        request.setRestMethod("amethod");
        request.setCallback(new ExpectFailureCallback());

        TestRequestAsyncTask task = new TestRequestAsyncTask(request);
        task.executeOnBlockerThread();

        waitAndAssertSuccess(1);
    }

    @MediumTest
    @LargeTest
    public void testClosedSessionDoesntAppendAccessToken() {
        TestSession session = openTestSessionWithSharedUser();
        session.close();
        Request request = new Request(session, "me", null, null, new ExpectFailureCallback());

        TestRequestAsyncTask task = new TestRequestAsyncTask(request);
        task.executeOnBlockerThread();

        waitAndAssertSuccess(1);
    }

    @MediumTest
    @LargeTest
    public void testCantUseComplexParameterInGetRequest() {
        TestSession session = openTestSessionWithSharedUser();

        Bundle parameters = new Bundle();
        parameters.putShortArray("foo", new short[1]);

        Request request = new Request(session, "me", parameters, HttpMethod.GET, new ExpectFailureCallback());
        Response response = request.executeAndWait();

        FacebookRequestError error = response.getError();
        assertNotNull(error);
        FacebookException exception = error.getException();
        assertNotNull(exception);
        assertTrue(exception.getMessage().contains("short[]"));
    }

    private final Location SEATTLE_LOCATION = new Location("") {
        {
            setLatitude(47.6097);
            setLongitude(-122.3331);
        }
    };

    @LargeTest
    public void testPaging() {
        TestSession session = openTestSessionWithSharedUser();
        final List<GraphPlace> returnedPlaces = new ArrayList<GraphPlace>();
        Request request = Request
                .newPlacesSearchRequest(session, SEATTLE_LOCATION, 1000, 5, null, new Request.GraphPlaceListCallback() {
                    @Override
                    public void onCompleted(List<GraphPlace> places, Response response) {
                        returnedPlaces.addAll(places);
                    }
                });
        Response response = request.executeAndWait();

        assertNull(response.getError());
        assertNotNull(response.getGraphObject());
        assertNotSame(0, returnedPlaces.size());

        returnedPlaces.clear();

        Request nextRequest = response.getRequestForPagedResults(Response.PagingDirection.NEXT);
        assertNotNull(nextRequest);

        nextRequest.setCallback(request.getCallback());
        response = nextRequest.executeAndWait();

        assertNull(response.getError());
        assertNotNull(response.getGraphObject());
        assertNotSame(0, returnedPlaces.size());

        returnedPlaces.clear();

        Request previousRequest = response.getRequestForPagedResults(Response.PagingDirection.PREVIOUS);
        assertNotNull(previousRequest);

        previousRequest.setCallback(request.getCallback());
        response = previousRequest.executeAndWait();

        assertNull(response.getError());
        assertNotNull(response.getGraphObject());
        assertNotSame(0, returnedPlaces.size());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testRequestWithClosedSessionThrowsException() {
        TestSession session = getTestSessionWithSharedUser();
        assertFalse(session.isOpened());

        Request request = new Request(session, "4");
        Response response = request.executeAndWait();

        assertNotNull(response.getError());
    }
}
