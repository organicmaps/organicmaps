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
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.test.suitebuilder.annotation.LargeTest;
import android.test.suitebuilder.annotation.MediumTest;
import android.test.suitebuilder.annotation.SmallTest;
import com.facebook.internal.NativeProtocol;
import com.facebook.model.GraphMultiResult;
import com.facebook.model.GraphObject;
import com.facebook.model.GraphObjectList;
import com.facebook.model.GraphUser;

import java.io.*;
import java.util.*;

public class AuthorizationClientTests extends FacebookTestCase {
    private static final String ACCESS_TOKEN = "An access token";
    private static final long EXPIRES_IN_DELTA = 3600 * 24 * 60;
    private static final ArrayList<String> PERMISSIONS = new ArrayList<String>(
            Arrays.asList("go outside", "come back in"));
    private static final String ERROR_MESSAGE = "This is bad!";

    class MockAuthorizationClient extends AuthorizationClient {
        Result result;
        boolean triedNextHandler = false;

        MockAuthorizationClient() {
            setContext(getActivity());
        }

        AuthorizationClient.AuthorizationRequest getRequest() {
            return pendingRequest;
        }

        void setRequest(AuthorizationClient.AuthorizationRequest request) {
            pendingRequest = request;
        }

        @Override
        void complete(Result result) {
            this.result = result;
        }

        @Override
        void tryNextHandler() {
            triedNextHandler = true;
        }
    }

    // WebViewAuthHandler tests

    AuthorizationClient.AuthorizationRequest createRequest() {
        Session.AuthorizationRequest request = new Session.AuthorizationRequest(getActivity());
        request.setPermissions(PERMISSIONS);
        return request.getAuthorizationClientRequest();
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testWebViewHandlesSuccess() {
        Bundle bundle = new Bundle();
        bundle.putString("access_token", ACCESS_TOKEN);
        bundle.putString("expires_in", String.format("%d", EXPIRES_IN_DELTA));
        bundle.putString("code", "Something else");

        MockAuthorizationClient client = new MockAuthorizationClient();
        AuthorizationClient.WebViewAuthHandler handler = client.new WebViewAuthHandler();

        AuthorizationClient.AuthorizationRequest request = createRequest();
        client.setRequest(request);
        handler.onWebDialogComplete(request, bundle, null);

        assertNotNull(client.result);
        assertEquals(AuthorizationClient.Result.Code.SUCCESS, client.result.code);

        AccessToken token = client.result.token;
        assertNotNull(token);
        assertEquals(ACCESS_TOKEN, token.getToken());
        assertDateDiffersWithinDelta(new Date(), token.getExpires(), EXPIRES_IN_DELTA * 1000, 1000);
        assertEquals(PERMISSIONS, token.getPermissions());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testWebViewHandlesCancel() {
        MockAuthorizationClient client = new MockAuthorizationClient();
        AuthorizationClient.WebViewAuthHandler handler = client.new WebViewAuthHandler();

        AuthorizationClient.AuthorizationRequest request = createRequest();
        client.setRequest(request);
        handler.onWebDialogComplete(request, null, new FacebookOperationCanceledException());

        assertNotNull(client.result);
        assertEquals(AuthorizationClient.Result.Code.CANCEL, client.result.code);
        assertNull(client.result.token);
        assertNotNull(client.result.errorMessage);
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testWebViewHandlesError() {
        MockAuthorizationClient client = new MockAuthorizationClient();
        AuthorizationClient.WebViewAuthHandler handler = client.new WebViewAuthHandler();

        AuthorizationClient.AuthorizationRequest request = createRequest();
        client.setRequest(request);
        handler.onWebDialogComplete(request, null, new FacebookException(ERROR_MESSAGE));

        assertNotNull(client.result);
        assertEquals(AuthorizationClient.Result.Code.ERROR, client.result.code);
        assertNull(client.result.token);
        assertNotNull(client.result.errorMessage);
        assertEquals(ERROR_MESSAGE, client.result.errorMessage);
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testWebViewChecksInternetPermission() {
        MockAuthorizationClient client = new MockAuthorizationClient() {
            @Override
            int checkPermission(String permission) {
                return PackageManager.PERMISSION_DENIED;
            }
        };
        AuthorizationClient.WebViewAuthHandler handler = client.new WebViewAuthHandler();

        AuthorizationClient.AuthorizationRequest request = createRequest();
        client.setRequest(request);
        handler.onWebDialogComplete(request, null, new FacebookException(ERROR_MESSAGE));

        assertNotNull(client.result);
        assertEquals(AuthorizationClient.Result.Code.ERROR, client.result.code);
        assertNull(client.result.token);
        assertNotNull(client.result.errorMessage);
    }

    // GetTokenAuthHandler tests

    @SmallTest
    @MediumTest
    @LargeTest
    public void testGetTokenHandlesSuccessWithAllPermissions() {
        Bundle bundle = new Bundle();
        bundle.putStringArrayList(NativeProtocol.EXTRA_PERMISSIONS, PERMISSIONS);
        bundle.putLong(NativeProtocol.EXTRA_EXPIRES_SECONDS_SINCE_EPOCH, new Date().getTime() / 1000 + EXPIRES_IN_DELTA);
        bundle.putString(NativeProtocol.EXTRA_ACCESS_TOKEN, ACCESS_TOKEN);

        MockAuthorizationClient client = new MockAuthorizationClient();
        AuthorizationClient.GetTokenAuthHandler handler = client.new GetTokenAuthHandler();

        AuthorizationClient.AuthorizationRequest request = createRequest();
        client.setRequest(request);
        handler.getTokenCompleted(request, bundle);

        assertNotNull(client.result);
        assertEquals(AuthorizationClient.Result.Code.SUCCESS, client.result.code);

        AccessToken token = client.result.token;
        assertNotNull(token);
        assertEquals(ACCESS_TOKEN, token.getToken());
        assertDateDiffersWithinDelta(new Date(), token.getExpires(), EXPIRES_IN_DELTA * 1000, 1000);
        assertEquals(PERMISSIONS, token.getPermissions());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testGetTokenHandlesSuccessWithSomePermissions() {
        Bundle bundle = new Bundle();
        bundle.putStringArrayList(NativeProtocol.EXTRA_PERMISSIONS, new ArrayList<String>(Arrays.asList("go outside")));
        bundle.putLong(NativeProtocol.EXTRA_EXPIRES_SECONDS_SINCE_EPOCH, new Date().getTime() / 1000 + EXPIRES_IN_DELTA);
        bundle.putString(NativeProtocol.EXTRA_ACCESS_TOKEN, ACCESS_TOKEN);

        MockAuthorizationClient client = new MockAuthorizationClient();
        AuthorizationClient.GetTokenAuthHandler handler = client.new GetTokenAuthHandler();

        AuthorizationClient.AuthorizationRequest request = createRequest();
        assertEquals(PERMISSIONS.size(), request.getPermissions().size());

        client.setRequest(request);
        handler.getTokenCompleted(request, bundle);

        assertNull(client.result);
        assertTrue(client.triedNextHandler);

        assertEquals(1, request.getPermissions().size());
        assertTrue(request.getPermissions().contains("come back in"));
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testGetTokenHandlesNoResult() {
        MockAuthorizationClient client = new MockAuthorizationClient();
        AuthorizationClient.GetTokenAuthHandler handler = client.new GetTokenAuthHandler();

        AuthorizationClient.AuthorizationRequest request = createRequest();
        assertEquals(PERMISSIONS.size(), request.getPermissions().size());

        client.setRequest(request);
        handler.getTokenCompleted(request, null);

        assertNull(client.result);
        assertTrue(client.triedNextHandler);

        assertEquals(PERMISSIONS.size(), request.getPermissions().size());
    }

    // KatanaProxyAuthHandler tests

    @SmallTest
    @MediumTest
    @LargeTest
    public void testProxyAuthHandlesSuccess() {
        Bundle bundle = new Bundle();
        bundle.putLong(AccessToken.EXPIRES_IN_KEY, EXPIRES_IN_DELTA);
        bundle.putString(AccessToken.ACCESS_TOKEN_KEY, ACCESS_TOKEN);

        Intent intent = new Intent();
        intent.putExtras(bundle);

        MockAuthorizationClient client = new MockAuthorizationClient();
        AuthorizationClient.KatanaProxyAuthHandler handler = client.new KatanaProxyAuthHandler();

        AuthorizationClient.AuthorizationRequest request = createRequest();
        client.setRequest(request);
        handler.onActivityResult(0, Activity.RESULT_OK, intent);

        assertNotNull(client.result);
        assertEquals(AuthorizationClient.Result.Code.SUCCESS, client.result.code);

        AccessToken token = client.result.token;
        assertNotNull(token);
        assertEquals(ACCESS_TOKEN, token.getToken());
        assertDateDiffersWithinDelta(new Date(), token.getExpires(), EXPIRES_IN_DELTA * 1000, 1000);
        assertEquals(PERMISSIONS, token.getPermissions());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testProxyAuthHandlesCancel() {
        Bundle bundle = new Bundle();
        bundle.putString("error", ERROR_MESSAGE);

        Intent intent = new Intent();
        intent.putExtras(bundle);

        MockAuthorizationClient client = new MockAuthorizationClient();
        AuthorizationClient.KatanaProxyAuthHandler handler = client.new KatanaProxyAuthHandler();

        AuthorizationClient.AuthorizationRequest request = createRequest();
        client.setRequest(request);
        handler.onActivityResult(0, Activity.RESULT_CANCELED, intent);

        assertNotNull(client.result);
        assertEquals(AuthorizationClient.Result.Code.CANCEL, client.result.code);

        assertNull(client.result.token);
        assertNotNull(client.result.errorMessage);
        assertTrue(client.result.errorMessage.contains(ERROR_MESSAGE));
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testProxyAuthHandlesCancelErrorMessage() {
        Bundle bundle = new Bundle();
        bundle.putString("error", "access_denied");

        Intent intent = new Intent();
        intent.putExtras(bundle);

        MockAuthorizationClient client = new MockAuthorizationClient();
        AuthorizationClient.KatanaProxyAuthHandler handler = client.new KatanaProxyAuthHandler();

        AuthorizationClient.AuthorizationRequest request = createRequest();
        client.setRequest(request);
        handler.onActivityResult(0, Activity.RESULT_CANCELED, intent);

        assertNotNull(client.result);
        assertEquals(AuthorizationClient.Result.Code.CANCEL, client.result.code);

        assertNull(client.result.token);
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testProxyAuthHandlesDisabled() {
        Bundle bundle = new Bundle();
        bundle.putString("error", "service_disabled");

        Intent intent = new Intent();
        intent.putExtras(bundle);

        MockAuthorizationClient client = new MockAuthorizationClient();
        AuthorizationClient.KatanaProxyAuthHandler handler = client.new KatanaProxyAuthHandler();

        AuthorizationClient.AuthorizationRequest request = createRequest();
        client.setRequest(request);
        handler.onActivityResult(0, Activity.RESULT_OK, intent);

        assertNull(client.result);
        assertTrue(client.triedNextHandler);
    }

    // Reauthorization validation tests

    class MockValidatingAuthorizationClient extends MockAuthorizationClient {
        private final HashMap<String, String> mapAccessTokenToFbid = new HashMap<String, String>();
        private List<String> permissionsToReport = Arrays.asList();
        private TestBlocker blocker;

        public MockValidatingAuthorizationClient(TestBlocker blocker) {
            this.blocker = blocker;
        }

        public void addAccessTokenToFbidMapping(String accessToken, String fbid) {
            mapAccessTokenToFbid.put(accessToken, fbid);
        }

        public void addAccessTokenToFbidMapping(AccessToken accessToken, String fbid) {
            mapAccessTokenToFbid.put(accessToken.getToken(), fbid);
        }

        public void setPermissionsToReport(List<String> permissionsToReport) {
            this.permissionsToReport = permissionsToReport;
        }

        @Override
        void complete(Result result) {
            super.complete(result);
            blocker.signal();
        }

        @Override
        Request createGetProfileIdRequest(final String accessToken) {
            return new MockRequest() {
                @Override
                public Response createResponse() {
                    String fbid = mapAccessTokenToFbid.get(accessToken);
                    GraphUser user = GraphObject.Factory.create(GraphUser.class);
                    user.setId(fbid);
                    return new Response(this, null, null, user, false);
                }
            };
        }

        @Override
        Request createGetPermissionsRequest(String accessToken) {
            final List<String> permissions = permissionsToReport;
            return new MockRequest() {
                @Override
                public Response createResponse() {
                    GraphObject permissionsObject = GraphObject.Factory.create();
                    if (permissions != null) {
                        for (String permission : permissions) {
                            permissionsObject.setProperty(permission, 1);
                        }
                    }
                    GraphObjectList<GraphObject> data = GraphObject.Factory.createList(GraphObject.class);
                    data.add(permissionsObject);

                    GraphMultiResult result = GraphObject.Factory.create(GraphMultiResult.class);
                    result.setProperty("data", data);

                    return new Response(this, null, null, result, false);
                }
            };
        }

        @Override
        RequestBatch createReauthValidationBatch(final Result pendingResult) {
            RequestBatch batch = super.createReauthValidationBatch(pendingResult);

            batch.setCallbackHandler(blocker.getHandler());
            // Turn it into a MockRequestBatch.
            return new MockRequestBatch(batch);
        }
    }

    static final String USER_1_FBID = "user1";
    static final String USER_1_ACCESS_TOKEN = "An access token for user 1";
    static final String USER_2_FBID = "user2";
    static final String USER_2_ACCESS_TOKEN = "An access token for user 2";

    AuthorizationClient.AuthorizationRequest createNewPermissionRequest(String accessToken) {
        Session.NewPermissionsRequest request = new Session.NewPermissionsRequest(getActivity(), PERMISSIONS);
        request.setValidateSameFbidAsToken(accessToken);
        return request.getAuthorizationClientRequest();
    }

    @MediumTest
    @LargeTest
    public void testReauthorizationWithSameFbidSucceeds() throws Exception {
        TestBlocker blocker = getTestBlocker();

        MockValidatingAuthorizationClient client = new MockValidatingAuthorizationClient(blocker);
        client.addAccessTokenToFbidMapping(USER_1_ACCESS_TOKEN, USER_1_FBID);
        client.addAccessTokenToFbidMapping(USER_2_ACCESS_TOKEN, USER_2_FBID);
        client.setPermissionsToReport(PERMISSIONS);

        AuthorizationClient.AuthorizationRequest request = createNewPermissionRequest(USER_1_ACCESS_TOKEN);
        client.setRequest(request);

        AccessToken token = AccessToken.createFromExistingAccessToken(USER_1_ACCESS_TOKEN, null, null, null, PERMISSIONS);
        AuthorizationClient.Result result = AuthorizationClient.Result.createTokenResult(request, token);

        client.completeAndValidate(result);

        blocker.waitForSignals(1);

        assertNotNull(client.result);
        assertEquals(AuthorizationClient.Result.Code.SUCCESS, client.result.code);

        AccessToken resultToken = client.result.token;
        assertNotNull(resultToken);
        assertEquals(USER_1_ACCESS_TOKEN, resultToken.getToken());

        // We don't care about ordering.
        assertEquals(new HashSet<String>(PERMISSIONS), new HashSet<String>(resultToken.getPermissions()));
    }

    @MediumTest
    @LargeTest
    public void testReauthorizationWithFewerPermissionsSucceeds() throws Exception {
        TestBlocker blocker = getTestBlocker();

        MockValidatingAuthorizationClient client = new MockValidatingAuthorizationClient(blocker);
        client.addAccessTokenToFbidMapping(USER_1_ACCESS_TOKEN, USER_1_FBID);
        client.addAccessTokenToFbidMapping(USER_2_ACCESS_TOKEN, USER_2_FBID);
        client.setPermissionsToReport(Arrays.asList("go outside"));

        AuthorizationClient.AuthorizationRequest request = createNewPermissionRequest(USER_1_ACCESS_TOKEN);
        client.setRequest(request);

        AccessToken token = AccessToken.createFromExistingAccessToken(USER_1_ACCESS_TOKEN, null, null, null, PERMISSIONS);
        AuthorizationClient.Result result = AuthorizationClient.Result.createTokenResult(request, token);

        client.completeAndValidate(result);

        blocker.waitForSignals(1);

        assertNotNull(client.result);
        assertEquals(AuthorizationClient.Result.Code.SUCCESS, client.result.code);

        AccessToken resultToken = client.result.token;
        assertNotNull(resultToken);
        assertEquals(USER_1_ACCESS_TOKEN, resultToken.getToken());
        assertEquals(Arrays.asList("go outside"), resultToken.getPermissions());
    }

    @MediumTest
    @LargeTest
    public void testReauthorizationWithDifferentFbidsFails() throws Exception {
        TestBlocker blocker = getTestBlocker();

        MockValidatingAuthorizationClient client = new MockValidatingAuthorizationClient(blocker);
        client.addAccessTokenToFbidMapping(USER_1_ACCESS_TOKEN, USER_1_FBID);
        client.addAccessTokenToFbidMapping(USER_2_ACCESS_TOKEN, USER_2_FBID);
        client.setPermissionsToReport(PERMISSIONS);

        AuthorizationClient.AuthorizationRequest request = createNewPermissionRequest(USER_1_ACCESS_TOKEN);
        client.setRequest(request);

        AccessToken token = AccessToken.createFromExistingAccessToken(USER_2_ACCESS_TOKEN, null, null, null, PERMISSIONS);
        AuthorizationClient.Result result = AuthorizationClient.Result.createTokenResult(request, token);

        client.completeAndValidate(result);

        blocker.waitForSignals(1);

        assertNotNull(client.result);
        assertEquals(AuthorizationClient.Result.Code.ERROR, client.result.code);

        assertNull(client.result.token);
        assertNotNull(client.result.errorMessage);
    }

    @MediumTest
    @LargeTest
    public void testLegacyReauthDoesntValidate() throws Exception {
        TestBlocker blocker = getTestBlocker();

        MockValidatingAuthorizationClient client = new MockValidatingAuthorizationClient(blocker);
        AuthorizationClient.AuthorizationRequest request = createNewPermissionRequest(USER_1_ACCESS_TOKEN);
        request.setIsLegacy(true);
        client.setRequest(request);

        AccessToken token = AccessToken.createFromExistingAccessToken(USER_2_ACCESS_TOKEN, null, null, null, PERMISSIONS);
        AuthorizationClient.Result result = AuthorizationClient.Result.createTokenResult(request, token);

        client.completeAndValidate(result);

        AccessToken resultToken = client.result.token;
        assertNotNull(resultToken);
        assertEquals(USER_2_ACCESS_TOKEN, resultToken.getToken());
        assertEquals(PERMISSIONS, resultToken.getPermissions());
    }

    // Serialization tests

    static class DoNothingAuthorizationClient extends AuthorizationClient {
        // Don't actually do anything.
        @Override
        boolean tryCurrentHandler() {
            return true;
        }
    }

    public void testSerialization() throws IOException, ClassNotFoundException {
        AuthorizationClient client = new DoNothingAuthorizationClient();

        // Call this to set up some state.
        client.setContext(getActivity());
        client.setOnCompletedListener(new AuthorizationClient.OnCompletedListener() {
            @Override
            public void onCompleted(AuthorizationClient.Result result) {
            }
        });
        client.setBackgroundProcessingListener(new AuthorizationClient.BackgroundProcessingListener() {
            @Override
            public void onBackgroundProcessingStarted() {
            }

            @Override
            public void onBackgroundProcessingStopped() {
            }
        });
        client.authorize(createRequest());

        ByteArrayOutputStream byteArrayOutputStream = new ByteArrayOutputStream();
        ObjectOutputStream outputStream = new ObjectOutputStream(byteArrayOutputStream);
        outputStream.writeObject(client);
        outputStream.close();

        byte [] byteArray = byteArrayOutputStream.toByteArray();

        ByteArrayInputStream byteArrayInputStream = new ByteArrayInputStream(byteArray);
        ObjectInputStream inputStream = new ObjectInputStream(byteArrayInputStream);

        Object obj = inputStream.readObject();
        assertNotNull(obj);
        assertTrue(obj instanceof AuthorizationClient);

        AuthorizationClient resultClient = (AuthorizationClient)obj;
        assertNull(resultClient.startActivityDelegate);
        assertNull(resultClient.onCompletedListener);
        assertNull(resultClient.backgroundProcessingListener);
        assertNull(resultClient.context);
        assertNotNull(resultClient.currentHandler);
        assertTrue(resultClient.currentHandler instanceof AuthorizationClient.GetTokenAuthHandler);
        assertNotNull(resultClient.handlersToTry);
        assertTrue(resultClient.handlersToTry.size() > 0);
        assertNotNull(resultClient.pendingRequest);
        assertEquals(PERMISSIONS, resultClient.pendingRequest.getPermissions());
    }
}
