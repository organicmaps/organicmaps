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

import android.content.Intent;
import android.content.IntentFilter;
import android.support.v4.content.LocalBroadcastManager;
import android.test.suitebuilder.annotation.LargeTest;
import android.test.suitebuilder.annotation.MediumTest;
import android.test.suitebuilder.annotation.SmallTest;
import com.facebook.internal.Utility;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.List;

public class SessionTests extends SessionTestsBase {

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        new SharedPreferencesTokenCachingStrategy(getActivity()).clear();
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testFailNullArguments() {
        try {
            new Session(null);

            // Should not get here
            assertFalse(true);
        } catch (NullPointerException e) {
            // got expected exception
        }
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testActiveSessionChangeRegistration() {
        final WaitForBroadcastReceiver receiver0 = new WaitForBroadcastReceiver();
        final WaitForBroadcastReceiver receiver1 = new WaitForBroadcastReceiver();
        final WaitForBroadcastReceiver receiver2 = new WaitForBroadcastReceiver();
        final LocalBroadcastManager broadcastManager = LocalBroadcastManager.getInstance(getActivity());

        try {
            // Register these on the blocker thread so they will send
            // notifications there as well. The notifications need to be on a
            // different thread than the progress.
            Runnable initialize0 = new Runnable() {
                @Override
                public void run() {
                    broadcastManager.registerReceiver(receiver0, getActiveSessionAllFilter());

                    broadcastManager.registerReceiver(receiver1,
                            getActiveSessionFilter(Session.ACTION_ACTIVE_SESSION_SET));
                    broadcastManager.registerReceiver(receiver1,
                            getActiveSessionFilter(Session.ACTION_ACTIVE_SESSION_OPENED));
                    broadcastManager.registerReceiver(receiver1,
                            getActiveSessionFilter(Session.ACTION_ACTIVE_SESSION_CLOSED));

                    broadcastManager.registerReceiver(receiver2,
                            getActiveSessionFilter(Session.ACTION_ACTIVE_SESSION_OPENED));
                    broadcastManager.registerReceiver(receiver2,
                            getActiveSessionFilter(Session.ACTION_ACTIVE_SESSION_CLOSED));
                }
            };
            runOnBlockerThread(initialize0, true);

            // Verify all actions show up where they are expected
            WaitForBroadcastReceiver.incrementExpectCounts(receiver0, receiver1, receiver2);
            Session.postActiveSessionAction(Session.ACTION_ACTIVE_SESSION_OPENED);
            WaitForBroadcastReceiver.waitForExpectedCalls(receiver0, receiver1, receiver2);

            WaitForBroadcastReceiver.incrementExpectCounts(receiver0, receiver1, receiver2);
            Session.postActiveSessionAction(Session.ACTION_ACTIVE_SESSION_CLOSED);
            WaitForBroadcastReceiver.waitForExpectedCalls(receiver0, receiver1, receiver2);

            WaitForBroadcastReceiver.incrementExpectCounts(receiver0, receiver1);
            Session.postActiveSessionAction(Session.ACTION_ACTIVE_SESSION_SET);
            WaitForBroadcastReceiver.waitForExpectedCalls(receiver0, receiver1);

            receiver0.incrementExpectCount();
            Session.postActiveSessionAction(Session.ACTION_ACTIVE_SESSION_UNSET);
            receiver0.waitForExpectedCalls();

            // Remove receiver1 and verify actions continue to show up where
            // expected
            broadcastManager.unregisterReceiver(receiver1);

            WaitForBroadcastReceiver.incrementExpectCounts(receiver0, receiver2);
            Session.postActiveSessionAction(Session.ACTION_ACTIVE_SESSION_OPENED);
            WaitForBroadcastReceiver.waitForExpectedCalls(receiver0, receiver2);

            WaitForBroadcastReceiver.incrementExpectCounts(receiver0, receiver2);
            Session.postActiveSessionAction(Session.ACTION_ACTIVE_SESSION_CLOSED);
            WaitForBroadcastReceiver.waitForExpectedCalls(receiver0, receiver2);

            receiver0.incrementExpectCount();
            Session.postActiveSessionAction(Session.ACTION_ACTIVE_SESSION_SET);
            receiver0.waitForExpectedCalls();

            receiver0.incrementExpectCount();
            Session.postActiveSessionAction(Session.ACTION_ACTIVE_SESSION_UNSET);
            receiver0.waitForExpectedCalls();

            // Remove receiver0 and register receiver1 multiple times for one
            // action
            broadcastManager.unregisterReceiver(receiver0);

            Runnable initialize1 = new Runnable() {
                @Override
                public void run() {
                    broadcastManager.registerReceiver(receiver1,
                            getActiveSessionFilter(Session.ACTION_ACTIVE_SESSION_OPENED));
                    broadcastManager.registerReceiver(receiver1,
                            getActiveSessionFilter(Session.ACTION_ACTIVE_SESSION_OPENED));
                    broadcastManager.registerReceiver(receiver1,
                            getActiveSessionFilter(Session.ACTION_ACTIVE_SESSION_OPENED));
                }
            };
            runOnBlockerThread(initialize1, true);

            receiver1.incrementExpectCount(3);
            receiver2.incrementExpectCount();
            Session.postActiveSessionAction(Session.ACTION_ACTIVE_SESSION_OPENED);
            receiver1.waitForExpectedCalls();
            receiver2.waitForExpectedCalls();

            receiver2.incrementExpectCount();
            Session.postActiveSessionAction(Session.ACTION_ACTIVE_SESSION_CLOSED);
            receiver2.waitForExpectedCalls();

            Session.postActiveSessionAction(Session.ACTION_ACTIVE_SESSION_SET);
            Session.postActiveSessionAction(Session.ACTION_ACTIVE_SESSION_UNSET);

            closeBlockerAndAssertSuccess();
        } finally {
            broadcastManager.unregisterReceiver(receiver0);
            broadcastManager.unregisterReceiver(receiver1);
            broadcastManager.unregisterReceiver(receiver2);
            Session.setActiveSession(null);
        }
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testSetActiveSession() {
        Session.setActiveSession(null);

        final WaitForBroadcastReceiver receiverOpened = new WaitForBroadcastReceiver();
        final WaitForBroadcastReceiver receiverClosed = new WaitForBroadcastReceiver();
        final WaitForBroadcastReceiver receiverSet = new WaitForBroadcastReceiver();
        final WaitForBroadcastReceiver receiverUnset = new WaitForBroadcastReceiver();
        final LocalBroadcastManager broadcastManager = LocalBroadcastManager.getInstance(getActivity());

        try {
            Runnable initializeOnBlockerThread = new Runnable() {
                @Override
                public void run() {
                    broadcastManager.registerReceiver(receiverOpened,
                            getActiveSessionFilter(Session.ACTION_ACTIVE_SESSION_OPENED));
                    broadcastManager.registerReceiver(receiverClosed,
                            getActiveSessionFilter(Session.ACTION_ACTIVE_SESSION_CLOSED));
                    broadcastManager.registerReceiver(receiverSet,
                            getActiveSessionFilter(Session.ACTION_ACTIVE_SESSION_SET));
                    broadcastManager.registerReceiver(receiverUnset,
                            getActiveSessionFilter(Session.ACTION_ACTIVE_SESSION_UNSET));
                }
            };
            runOnBlockerThread(initializeOnBlockerThread, true);

            // null -> null should not fire events
            assertEquals(null, Session.getActiveSession());
            Session.setActiveSession(null);
            assertEquals(null, Session.getActiveSession());

            Session session0 = new Session.Builder(getActivity()).
                    setApplicationId("FakeAppId").
                    setTokenCachingStrategy(new MockTokenCachingStrategy()).
                    build();
            assertEquals(SessionState.CREATED_TOKEN_LOADED, session0.getState());

            // For unopened session, we should only see the Set event.
            receiverSet.incrementExpectCount();
            Session.setActiveSession(session0);
            assertEquals(session0, Session.getActiveSession());
            receiverSet.waitForExpectedCalls();

            // When we open it, then we should see the Opened event.
            receiverOpened.incrementExpectCount();
            session0.openForRead(null);
            receiverOpened.waitForExpectedCalls();

            // Setting to itself should not fire events
            Session.setActiveSession(session0);
            assertEquals(session0, Session.getActiveSession());

            // Setting from one opened session to another should deliver a full
            // cycle of events
            WaitForBroadcastReceiver.incrementExpectCounts(receiverClosed, receiverUnset, receiverSet, receiverOpened);
            Session session1 = new Session.Builder(getActivity()).
                    setApplicationId("FakeAppId").
                    setTokenCachingStrategy(new MockTokenCachingStrategy()).
                    build();
            assertEquals(SessionState.CREATED_TOKEN_LOADED, session1.getState());
            session1.openForRead(null);
            assertEquals(SessionState.OPENED, session1.getState());
            Session.setActiveSession(session1);
            WaitForBroadcastReceiver.waitForExpectedCalls(receiverClosed, receiverUnset, receiverSet, receiverOpened);
            assertEquals(SessionState.CLOSED, session0.getState());
            assertEquals(session1, Session.getActiveSession());

            closeBlockerAndAssertSuccess();
        } finally {
            broadcastManager.unregisterReceiver(receiverOpened);
            broadcastManager.unregisterReceiver(receiverClosed);
            broadcastManager.unregisterReceiver(receiverSet);
            broadcastManager.unregisterReceiver(receiverUnset);
            Session.setActiveSession(null);
        }
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testOpenSuccess() {
        ArrayList<String> permissions = new ArrayList<String>();
        MockTokenCachingStrategy cache = new MockTokenCachingStrategy(null, 0);
        SessionStatusCallbackRecorder statusRecorder = new SessionStatusCallbackRecorder();
        ScriptedSession session = createScriptedSessionOnBlockerThread(cache);
        AccessToken openToken = AccessToken
                .createFromString("A token of thanks", permissions, AccessTokenSource.TEST_USER);

        // Verify state with no token in cache
        assertEquals(SessionState.CREATED, session.getState());

        session.addAuthorizeResult(openToken);
        session.openForRead(new Session.OpenRequest(getActivity()).setCallback(statusRecorder));
        statusRecorder.waitForCall(session, SessionState.OPENING, null);
        statusRecorder.waitForCall(session, SessionState.OPENED, null);

        verifySessionHasToken(session, openToken);

        // Verify we get a close callback.
        session.close();
        statusRecorder.waitForCall(session, SessionState.CLOSED, null);

        // Verify we saved the token to cache.
        assertTrue(cache.getSavedState() != null);
        assertEquals(openToken.getToken(), TokenCachingStrategy.getToken(cache.getSavedState()));

        // Verify token information is cleared.
        session.closeAndClearTokenInformation();
        assertTrue(cache.getSavedState() == null);

        // Wait a bit so we can fail if any unexpected calls arrive on the
        // recorder.
        stall(STRAY_CALLBACK_WAIT_MILLISECONDS);
        statusRecorder.close();
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testOpenForPublishSuccess() {
        ArrayList<String> permissions = new ArrayList<String>();
        MockTokenCachingStrategy cache = new MockTokenCachingStrategy(null, 0);
        SessionStatusCallbackRecorder statusRecorder = new SessionStatusCallbackRecorder();
        ScriptedSession session = createScriptedSessionOnBlockerThread(cache);
        AccessToken openToken = AccessToken
                .createFromString("A token of thanks", permissions, AccessTokenSource.TEST_USER);

        // Verify state with no token in cache
        assertEquals(SessionState.CREATED, session.getState());

        session.addAuthorizeResult(openToken);
        session.openForPublish(new Session.OpenRequest(getActivity()).setCallback(statusRecorder).
                setPermissions(Arrays.asList(new String[]{
                        "publish_something",
                        "manage_something",
                        "ads_management",
                        "create_event",
                        "rsvp_event"
                })));
        statusRecorder.waitForCall(session, SessionState.OPENING, null);
        statusRecorder.waitForCall(session, SessionState.OPENED, null);

        verifySessionHasToken(session, openToken);

        // Verify we get a close callback.
        session.close();
        statusRecorder.waitForCall(session, SessionState.CLOSED, null);

        // Verify we saved the token to cache.
        assertTrue(cache.getSavedState() != null);
        assertEquals(openToken.getToken(), TokenCachingStrategy.getToken(cache.getSavedState()));

        // Verify token information is cleared.
        session.closeAndClearTokenInformation();
        assertTrue(cache.getSavedState() == null);

        // Wait a bit so we can fail if any unexpected calls arrive on the
        // recorder.
        stall(STRAY_CALLBACK_WAIT_MILLISECONDS);
        statusRecorder.close();
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testOpenForPublishSuccessWithReadPermissions() {
        ArrayList<String> permissions = new ArrayList<String>();
        MockTokenCachingStrategy cache = new MockTokenCachingStrategy(null, 0);
        SessionStatusCallbackRecorder statusRecorder = new SessionStatusCallbackRecorder();
        ScriptedSession session = createScriptedSessionOnBlockerThread(cache);
        AccessToken openToken = AccessToken
                .createFromString("A token of thanks", permissions, AccessTokenSource.TEST_USER);

        // Verify state with no token in cache
        assertEquals(SessionState.CREATED, session.getState());

        session.addAuthorizeResult(openToken);
        session.openForPublish(new Session.OpenRequest(getActivity()).setCallback(statusRecorder).
                setPermissions(Arrays.asList(new String[]{
                        "publish_something",
                        "manage_something",
                        "ads_management",
                        "create_event",
                        "rsvp_event",
                        "read_something"
                })));
        statusRecorder.waitForCall(session, SessionState.OPENING, null);
        statusRecorder.waitForCall(session, SessionState.OPENED, null);

        verifySessionHasToken(session, openToken);

        // Verify we get a close callback.
        session.close();
        statusRecorder.waitForCall(session, SessionState.CLOSED, null);

        // Verify we saved the token to cache.
        assertTrue(cache.getSavedState() != null);
        assertEquals(openToken.getToken(), TokenCachingStrategy.getToken(cache.getSavedState()));

        // Verify token information is cleared.
        session.closeAndClearTokenInformation();
        assertTrue(cache.getSavedState() == null);

        // Wait a bit so we can fail if any unexpected calls arrive on the
        // recorder.
        stall(STRAY_CALLBACK_WAIT_MILLISECONDS);
        statusRecorder.close();
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testOpenFromTokenCache() {
        SessionStatusCallbackRecorder statusRecorder = new SessionStatusCallbackRecorder();
        String token = "A token less unique than most";
        MockTokenCachingStrategy cache = new MockTokenCachingStrategy(token, DEFAULT_TIMEOUT_MILLISECONDS);
        ScriptedSession session = createScriptedSessionOnBlockerThread("app-id", cache);

        // Verify state when we have a token in cache.
        assertEquals(SessionState.CREATED_TOKEN_LOADED, session.getState());

        session.openForRead(new Session.OpenRequest(getActivity()).setCallback(statusRecorder));

        // Verify we open with no authorize call.
        statusRecorder.waitForCall(session, SessionState.OPENED, null);

        // Verify no token information is saved.
        assertTrue(cache.getSavedState() == null);

        // Verify we get a close callback.
        session.close();
        statusRecorder.waitForCall(session, SessionState.CLOSED, null);

        // Wait a bit so we can fail if any unexpected calls arrive on the
        // recorder.
        stall(STRAY_CALLBACK_WAIT_MILLISECONDS);
        statusRecorder.close();
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testOpenActiveFromEmptyTokenCache() {
        new SharedPreferencesTokenCachingStrategy(getActivity()).clear();

        assertNull(Session.openActiveSessionFromCache(getActivity()));
    }


    @SmallTest
    @MediumTest
    @LargeTest
    public void testOpenFailure() {
        SessionStatusCallbackRecorder statusRecorder = new SessionStatusCallbackRecorder();
        MockTokenCachingStrategy cache = new MockTokenCachingStrategy(null, 0);
        ScriptedSession session = createScriptedSessionOnBlockerThread(cache);
        Exception openException = new Exception();

        session.addAuthorizeResult(openException);
        session.openForRead(new Session.OpenRequest(getActivity()).setCallback(statusRecorder));
        statusRecorder.waitForCall(session, SessionState.OPENING, null);

        // Verify we get the expected exception and no saved state.
        statusRecorder.waitForCall(session, SessionState.CLOSED_LOGIN_FAILED, openException);
        assertTrue(cache.getSavedState() == null);

        // Wait a bit so we can fail if any unexpected calls arrive on the
        // recorder.
        stall(STRAY_CALLBACK_WAIT_MILLISECONDS);
        statusRecorder.close();
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testOpenForReadFailure() {
        SessionStatusCallbackRecorder statusRecorder = new SessionStatusCallbackRecorder();
        MockTokenCachingStrategy cache = new MockTokenCachingStrategy(null, 0);
        ScriptedSession session = createScriptedSessionOnBlockerThread(cache);

        try {
            session.openForRead(new Session.OpenRequest(getActivity()).setCallback(statusRecorder).
                    setPermissions(Arrays.asList(new String[]{"publish_something"})));
            fail("should not reach here without an exception");
        } catch (FacebookException e) {
            assertTrue(e.getMessage().contains("Cannot pass a publish or manage permission"));
        } finally {
            stall(STRAY_CALLBACK_WAIT_MILLISECONDS);
            statusRecorder.close();
        }
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testRequestNewReadPermissionsSuccess() {
        ArrayList<String> permissions = new ArrayList<String>();
        SessionStatusCallbackRecorder statusRecorder = new SessionStatusCallbackRecorder();
        MockTokenCachingStrategy cache = new MockTokenCachingStrategy(null, 0);
        ScriptedSession session = createScriptedSessionOnBlockerThread(cache);

        // Session.open
        final AccessToken openToken = AccessToken
                .createFromString("Allows playing outside", permissions, AccessTokenSource.TEST_USER);
        permissions.add("play_outside");

        session.addAuthorizeResult(openToken, "play_outside");
        session.openForRead(new Session.OpenRequest(getActivity()).setCallback(statusRecorder));
        statusRecorder.waitForCall(session, SessionState.OPENING, null);
        statusRecorder.waitForCall(session, SessionState.OPENED, null);

        verifySessionHasToken(session, openToken);
        assertTrue(cache.getSavedState() != null);
        assertEquals(openToken.getToken(), TokenCachingStrategy.getToken(cache.getSavedState()));

        // Successful Session.reauthorize with new permissions
        final AccessToken reauthorizeToken = AccessToken.createFromString(
                "Allows playing outside and eating ice cream", permissions, AccessTokenSource.TEST_USER);
        permissions.add("eat_ice_cream");

        session.addAuthorizeResult(reauthorizeToken, "play_outside", "eat_ice_cream");
        session.requestNewReadPermissions(new Session.NewPermissionsRequest(getActivity(), permissions));
        statusRecorder.waitForCall(session, SessionState.OPENED_TOKEN_UPDATED, null);

        verifySessionHasToken(session, reauthorizeToken);
        assertTrue(cache.getSavedState() != null);
        assertEquals(reauthorizeToken.getToken(), TokenCachingStrategy.getToken(cache.getSavedState()));

        // Failing reauthorization with new permissions
        final Exception reauthorizeException = new Exception("Don't run with scissors");
        permissions.add("run_with_scissors");

        session.addAuthorizeResult(reauthorizeException);
        session.requestNewReadPermissions(new Session.NewPermissionsRequest(getActivity(), permissions));
        statusRecorder.waitForCall(session, SessionState.OPENED_TOKEN_UPDATED, reauthorizeException);

        // Verify we do not overwrite cache if reauthorize fails
        assertTrue(cache.getSavedState() != null);
        assertEquals(reauthorizeToken.getToken(), TokenCachingStrategy.getToken(cache.getSavedState()));

        // Wait a bit so we can fail if any unexpected calls arrive on the
        // recorders.
        stall(STRAY_CALLBACK_WAIT_MILLISECONDS);
        statusRecorder.close();
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testRequestNewPublishPermissionsSuccess() {
        ArrayList<String> permissions = new ArrayList<String>();
        SessionStatusCallbackRecorder statusRecorder = new SessionStatusCallbackRecorder();
        MockTokenCachingStrategy cache = new MockTokenCachingStrategy(null, 0);
        ScriptedSession session = createScriptedSessionOnBlockerThread(cache);

        // Session.open
        final AccessToken openToken = AccessToken
                .createFromString("Allows playing outside", permissions, AccessTokenSource.TEST_USER);
        permissions.add("play_outside");

        session.addAuthorizeResult(openToken, "play_outside");
        session.openForRead(new Session.OpenRequest(getActivity()).setCallback(statusRecorder));
        statusRecorder.waitForCall(session, SessionState.OPENING, null);
        statusRecorder.waitForCall(session, SessionState.OPENED, null);

        verifySessionHasToken(session, openToken);
        assertTrue(cache.getSavedState() != null);
        assertEquals(openToken.getToken(), TokenCachingStrategy.getToken(cache.getSavedState()));

        // Successful Session.reauthorize with new permissions
        final AccessToken reauthorizeToken = AccessToken.createFromString(
                "Allows playing outside and publish eating ice cream", permissions, AccessTokenSource.TEST_USER);
        permissions.add("publish_eat_ice_cream");

        session.addAuthorizeResult(reauthorizeToken, "play_outside", "publish_eat_ice_cream");
        session.requestNewPublishPermissions(new Session.NewPermissionsRequest(getActivity(), permissions));
        statusRecorder.waitForCall(session, SessionState.OPENED_TOKEN_UPDATED, null);

        verifySessionHasToken(session, reauthorizeToken);
        assertTrue(cache.getSavedState() != null);
        assertEquals(reauthorizeToken.getToken(), TokenCachingStrategy.getToken(cache.getSavedState()));

        // Failing reauthorization with publish permissions on a read request
        permissions.add("publish_run_with_scissors");

        try {
            session.requestNewReadPermissions(new Session.NewPermissionsRequest(getActivity(), permissions));
            fail("Should not reach here without an exception");
        } catch (FacebookException e) {
            assertTrue(e.getMessage().contains("Cannot pass a publish or manage permission"));
        } finally {
            stall(STRAY_CALLBACK_WAIT_MILLISECONDS);
            statusRecorder.close();
        }
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testOpenWithAccessToken() {
        String token = "This is a fake token.";
        Date expirationDate = new Date(new Date().getTime() + 3600 * 1000);
        Date lastRefreshDate = new Date();
        List<String> permissions = Arrays.asList(new String[]{"email", "publish_stream"});

        MockTokenCachingStrategy cache = new MockTokenCachingStrategy(null, 0);
        SessionStatusCallbackRecorder statusRecorder = new SessionStatusCallbackRecorder();
        ScriptedSession session = createScriptedSessionOnBlockerThread(cache);

        // Verify state with no token in cache
        assertEquals(SessionState.CREATED, session.getState());

        AccessToken accessToken = AccessToken.createFromExistingAccessToken(token, expirationDate, lastRefreshDate,
                AccessTokenSource.FACEBOOK_APPLICATION_WEB, permissions);
        session.open(accessToken, statusRecorder);
        statusRecorder.waitForCall(session, SessionState.OPENED, null);

        AccessToken expectedToken = new AccessToken(token, expirationDate, permissions, null,
                AccessTokenSource.FACEBOOK_APPLICATION_WEB, lastRefreshDate);
        verifySessionHasToken(session, expectedToken);

        // Verify we get a close callback.
        session.close();
        statusRecorder.waitForCall(session, SessionState.CLOSED, null);

        // Verify we saved the token to cache.
        assertTrue(cache.getSavedState() != null);
        assertEquals(expectedToken.getToken(), TokenCachingStrategy.getToken(cache.getSavedState()));

        // Verify token information is cleared.
        session.closeAndClearTokenInformation();
        assertTrue(cache.getSavedState() == null);

        // Wait a bit so we can fail if any unexpected calls arrive on the
        // recorder.
        stall(STRAY_CALLBACK_WAIT_MILLISECONDS);
        statusRecorder.close();
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testOpenWithAccessTokenWithDefaults() {
        String token = "This is a fake token.";

        MockTokenCachingStrategy cache = new MockTokenCachingStrategy(null, 0);
        SessionStatusCallbackRecorder statusRecorder = new SessionStatusCallbackRecorder();
        ScriptedSession session = createScriptedSessionOnBlockerThread(cache);

        // Verify state with no token in cache
        assertEquals(SessionState.CREATED, session.getState());

        AccessToken accessToken = AccessToken.createFromExistingAccessToken(token, null, null, null, null);
        session.open(accessToken, statusRecorder);
        statusRecorder.waitForCall(session, SessionState.OPENED, null);

        assertEquals(token, session.getAccessToken());
        assertEquals(new Date(Long.MAX_VALUE), session.getExpirationDate());
        assertEquals(0, session.getPermissions().size());

        // Verify we get a close callback.
        session.close();
        statusRecorder.waitForCall(session, SessionState.CLOSED, null);

        // Verify we saved the token to cache.
        assertTrue(cache.getSavedState() != null);

        // Verify token information is cleared.
        session.closeAndClearTokenInformation();
        assertTrue(cache.getSavedState() == null);

        // Wait a bit so we can fail if any unexpected calls arrive on the
        // recorder.
        stall(STRAY_CALLBACK_WAIT_MILLISECONDS);
        statusRecorder.close();
    }

    @MediumTest
    @LargeTest
    public void testSessionWillExtendTokenIfNeeded() {
        TestSession session = openTestSessionWithSharedUser();
        session.forceExtendAccessToken(true);

        Request request = Request.newMeRequest(session, null);
        request.executeAndWait();

        assertTrue(session.getWasAskedToExtendAccessToken());
    }

    @MediumTest
    @LargeTest
    public void testSessionWillNotExtendTokenIfCurrentlyAttempting() {
        TestSession session = openTestSessionWithSharedUser();
        session.forceExtendAccessToken(true);
        session.fakeTokenRefreshAttempt();

        Request request = Request.newMeRequest(session, null);
        request.executeAndWait();
        assertFalse(session.getWasAskedToExtendAccessToken());
    }


    @LargeTest
    public void testBasicSerialization() throws IOException, ClassNotFoundException {
        // Try to test the happy path, that there are no unserializable fields
        // in the session.
        Session session0 = new Session.Builder(getActivity()).setApplicationId("fakeID").build();
        Session session1 = TestUtils.serializeAndUnserialize(session0);

        // do some basic assertions
        assertNotNull(session0.getAccessToken());
        assertEquals(session0, session1);

        Session.AuthorizationRequest authRequest0 =
                new Session.OpenRequest(getActivity()).
                        setRequestCode(123).
                        setLoginBehavior(SessionLoginBehavior.SSO_ONLY);
        Session.AuthorizationRequest authRequest1 = TestUtils.serializeAndUnserialize(authRequest0);

        assertEquals(authRequest0.getLoginBehavior(), authRequest1.getLoginBehavior());
        assertEquals(authRequest0.getRequestCode(), authRequest1.getRequestCode());
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testOpenSessionWithNativeLinkingIntent() {
        String token = "A token less unique than most";

        Intent intent = new Intent(Intent.ACTION_MAIN);
        intent.putExtras(getNativeLinkingExtras(token));

        SessionStatusCallbackRecorder statusRecorder = new SessionStatusCallbackRecorder();
        MockTokenCachingStrategy cache = new MockTokenCachingStrategy(null, DEFAULT_TIMEOUT_MILLISECONDS);
        ScriptedSession session = createScriptedSessionOnBlockerThread(cache);

        assertEquals(SessionState.CREATED, session.getState());

        AccessToken accessToken = AccessToken.createFromNativeLinkingIntent(intent);
        assertNotNull(accessToken);
        session.open(accessToken, statusRecorder);

        statusRecorder.waitForCall(session, SessionState.OPENED, null);

        assertEquals(token, session.getAccessToken());
        // Expiration time should be 3600s after now (allow 5s variation for test execution time)
        long delta = session.getExpirationDate().getTime() - new Date().getTime();
        assertTrue(Math.abs(delta - 3600 * 1000) < 5000);
        assertEquals(0, session.getPermissions().size());
        assertEquals(Utility.getMetadataApplicationId(getActivity()), session.getApplicationId());

        // Verify we get a close callback.
        session.close();
        statusRecorder.waitForCall(session, SessionState.CLOSED, null);

        assertFalse(cache.getSavedState() == null);

        // Wait a bit so we can fail if any unexpected calls arrive on the
        // recorder.
        stall(STRAY_CALLBACK_WAIT_MILLISECONDS);
        statusRecorder.close();
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testOpenActiveSessionWithNativeLinkingIntent() {
        Session activeSession = Session.getActiveSession();
        if (activeSession != null) {
            activeSession.closeAndClearTokenInformation();
        }

        SharedPreferencesTokenCachingStrategy tokenCache = new SharedPreferencesTokenCachingStrategy(getActivity());
        assertEquals(0, tokenCache.load().size());

        String token = "A token less unique than most";

        Intent intent = new Intent(Intent.ACTION_MAIN);
        intent.putExtras(getNativeLinkingExtras(token));

        SessionStatusCallbackRecorder statusRecorder = new SessionStatusCallbackRecorder();

        AccessToken accessToken = AccessToken.createFromNativeLinkingIntent(intent);
        assertNotNull(accessToken);
        Session session = Session.openActiveSessionWithAccessToken(getActivity(), accessToken, statusRecorder);
        assertEquals(session, Session.getActiveSession());

        statusRecorder.waitForCall(session, SessionState.OPENED, null);

        assertNotSame(0, tokenCache.load().size());

        assertEquals(token, session.getAccessToken());
        // Expiration time should be 3600s after now (allow 5s variation for test execution time)
        long delta = session.getExpirationDate().getTime() - new Date().getTime();
        assertTrue(Math.abs(delta - 3600 * 1000) < 5000);
        assertEquals(0, session.getPermissions().size());
        assertEquals(Utility.getMetadataApplicationId(getActivity()), session.getApplicationId());

        // Verify we get a close callback.
        session.close();
        statusRecorder.waitForCall(session, SessionState.CLOSED, null);

        // Wait a bit so we can fail if any unexpected calls arrive on the
        // recorder.
        stall(STRAY_CALLBACK_WAIT_MILLISECONDS);
        statusRecorder.close();
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testOpeningSessionWithPendingRequestResultsInExceptionCallback() {
        MockTokenCachingStrategy cache = new MockTokenCachingStrategy(null, 0);
        SessionStatusCallbackRecorder statusRecorder = new SessionStatusCallbackRecorder();
        ScriptedSession session = createScriptedSessionOnBlockerThread(cache);

        // Verify state with no token in cache
        assertEquals(SessionState.CREATED, session.getState());
        session.addPendingAuthorizeResult();

        session.openForRead(new Session.OpenRequest(getActivity()).setCallback(statusRecorder));
        session.openForRead(new Session.OpenRequest(getActivity()).setCallback(statusRecorder));
        statusRecorder.waitForCall(session, SessionState.OPENING, null);
        statusRecorder.waitForCall(session, SessionState.OPENING, new UnsupportedOperationException());

        stall(STRAY_CALLBACK_WAIT_MILLISECONDS);
        statusRecorder.close();
    }

    static IntentFilter getActiveSessionFilter(String... actions) {
        IntentFilter filter = new IntentFilter();

        for (String action : actions) {
            filter.addAction(action);
        }

        return filter;
    }

    static IntentFilter getActiveSessionAllFilter() {
        return getActiveSessionFilter(Session.ACTION_ACTIVE_SESSION_CLOSED, Session.ACTION_ACTIVE_SESSION_OPENED,
                Session.ACTION_ACTIVE_SESSION_SET, Session.ACTION_ACTIVE_SESSION_UNSET);
    }

    private void verifySessionHasToken(Session session, AccessToken token) {
        assertEquals(token.getToken(), session.getAccessToken());
        assertEquals(token.getExpires(), session.getExpirationDate());
        TestUtils.assertAtLeastExpectedPermissions(token.getPermissions(), session.getPermissions());
    }
}
