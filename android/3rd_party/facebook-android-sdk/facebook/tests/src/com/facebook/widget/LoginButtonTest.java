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

import android.test.suitebuilder.annotation.LargeTest;
import android.test.suitebuilder.annotation.MediumTest;
import android.test.suitebuilder.annotation.SmallTest;
import com.facebook.*;
import com.facebook.widget.LoginButton;
import junit.framework.Assert;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.atomic.AtomicBoolean;

public class LoginButtonTest extends SessionTestsBase {

    static final int STRAY_CALLBACK_WAIT_MILLISECONDS = 50;

    @SmallTest
    @MediumTest
    @LargeTest
    public void testLoginButton() throws Throwable {
        MockTokenCachingStrategy cache = new MockTokenCachingStrategy(null, 0);
        final ScriptedSession session = new ScriptedSession(getActivity(), "SomeId", cache);
        SessionTestsBase.SessionStatusCallbackRecorder statusRecorder = new SessionTestsBase.SessionStatusCallbackRecorder();

        session.addAuthorizeResult("A token of thanks", new ArrayList<String>(), AccessTokenSource.TEST_USER);
        session.addCallback(statusRecorder);

        // Verify state with no token in cache
        Assert.assertEquals(SessionState.CREATED, session.getState());

        // Add another status recorder to ensure that the callback we hand to LoginButton
        // also gets called as expected. We expect to get the same order of calls as statusRecorder does.
        final SessionStatusCallbackRecorder loginButtonStatusRecorder = new SessionStatusCallbackRecorder();

        // Create the button. To get session status updates, we need to actually attach the
        // button to a window, which must be done on the UI thread.
        final LoginButton button = new LoginButton(getActivity());
        runAndBlockOnUiThread(0, new Runnable() {
            @Override
            public void run() {
                getActivity().setContentView(button);
                button.setSession(session);
                button.setSessionStatusCallback(loginButtonStatusRecorder);
                button.performClick();
            }
        });

        statusRecorder.waitForCall(session, SessionState.OPENING, null);
        loginButtonStatusRecorder.waitForCall(session, SessionState.OPENING, null);

        statusRecorder.waitForCall(session, SessionState.OPENED, null);
        loginButtonStatusRecorder.waitForCall(session, SessionState.OPENED, null);

        // Verify token information is cleared.
        session.closeAndClearTokenInformation();
        assertTrue(cache.getSavedState() == null);
        statusRecorder.waitForCall(session, SessionState.CLOSED, null);
        loginButtonStatusRecorder.waitForCall(session, SessionState.CLOSED, null);

        // Wait a bit so we can fail if any unexpected calls arrive on the
        // recorder.
        stall(STRAY_CALLBACK_WAIT_MILLISECONDS);
        statusRecorder.close();
    }

    @SmallTest
    @MediumTest
    @LargeTest
    // Tests that the onErrorListener gets called if there's an error
    public void testLoginFail() {
        MockTokenCachingStrategy cache = new MockTokenCachingStrategy(null, 0);
        ScriptedSession session = new ScriptedSession(getActivity(), "SomeId", cache);
        final Exception openException = new Exception("Open failed!");
        final AtomicBoolean clicked = new AtomicBoolean(false);

        // Verify state with no token in cache
        assertEquals(SessionState.CREATED, session.getState());

        final LoginButton button = new LoginButton(getActivity());
        LoginButton.OnErrorListener listener = new LoginButton.OnErrorListener() {
            @Override
            public void onError(FacebookException exception) {
                synchronized (this) {
                    assertEquals(exception.getCause().getMessage(), openException.getMessage());
                    clicked.set(true);
                    this.notifyAll();
                }
            }
        };
        button.setOnErrorListener(listener);
        button.setSession(session);
        session.addAuthorizeResult(openException);

        button.onAttachedToWindow();
        button.performClick();

        try {
            synchronized (listener) {
                listener.wait(DEFAULT_TIMEOUT_MILLISECONDS);
            }
        } catch (InterruptedException e) {
            fail("Interrupted during open");
        }

        if (!clicked.get()) {
            fail("Did not get exception");
        }
    }

    @SmallTest
    @MediumTest
    @LargeTest
    // Tests that the onErrorListener does NOT get called if there's a callback set.
    public void testLoginFail2() {
        MockTokenCachingStrategy cache = new MockTokenCachingStrategy(null, 0);
        ScriptedSession session = new ScriptedSession(getActivity(), "SomeId", cache);
        final Exception openException = new Exception("Open failed!");
        final AtomicBoolean clicked = new AtomicBoolean(false);

        // Verify state with no token in cache
        assertEquals(SessionState.CREATED, session.getState());

        final LoginButton button = new LoginButton(getActivity());
        Session.StatusCallback callback = new Session.StatusCallback() {
            @Override
            public void call(Session session, SessionState state, Exception exception) {
                if (exception != null) {
                    synchronized (this) {
                        assertEquals(exception.getMessage(), openException.getMessage());
                        clicked.set(true);
                        this.notifyAll();
                    }
                }
            }
        };
        button.setSessionStatusCallback(callback);
        button.setOnErrorListener(new LoginButton.OnErrorListener() {
            @Override
            public void onError(FacebookException exception) {
                synchronized (this) {
                    fail("Should not be in here");
                    this.notifyAll();
                }
            }
        });
        button.setSession(session);
        session.addAuthorizeResult(openException);

        button.onAttachedToWindow();
        button.performClick();

        try {
            synchronized (callback) {
                callback.wait(DEFAULT_TIMEOUT_MILLISECONDS);
            }
        } catch (InterruptedException e) {
            fail("Interrupted during open");
        }

        if (!clicked.get()) {
            fail("Did not get exception");
        }
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCanAddReadPermissions() {
        MockTokenCachingStrategy cache = new MockTokenCachingStrategy(null, 0);
        ScriptedSession session = new ScriptedSession(getActivity(), "SomeId", cache);
        SessionTestsBase.SessionStatusCallbackRecorder statusRecorder = new SessionTestsBase.SessionStatusCallbackRecorder();

        // Verify state with no token in cache
        assertEquals(SessionState.CREATED, session.getState());

        final LoginButton button = new LoginButton(getActivity());
        button.setSession(session);
        button.setReadPermissions("read_permission", "read_another");
        session.addAuthorizeResult("A token of thanks", new ArrayList<String>(), AccessTokenSource.TEST_USER);
        session.addCallback(statusRecorder);

        button.performClick();

        statusRecorder.waitForCall(session, SessionState.OPENING, null);
        statusRecorder.waitForCall(session, SessionState.OPENED, null);

        // Verify token information is cleared.
        session.closeAndClearTokenInformation();
        assertTrue(cache.getSavedState() == null);
        statusRecorder.waitForCall(session, SessionState.CLOSED, null);

        // Wait a bit so we can fail if any unexpected calls arrive on the
        // recorder.
        stall(STRAY_CALLBACK_WAIT_MILLISECONDS);
        statusRecorder.close();
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCanAddPublishPermissions() {
        MockTokenCachingStrategy cache = new MockTokenCachingStrategy(null, 0);
        ScriptedSession session = new ScriptedSession(getActivity(), "SomeId", cache);
        SessionTestsBase.SessionStatusCallbackRecorder statusRecorder =
                new SessionTestsBase.SessionStatusCallbackRecorder();

        // Verify state with no token in cache
        assertEquals(SessionState.CREATED, session.getState());

        final LoginButton button = new LoginButton(getActivity());
        button.setSession(session);
        button.setPublishPermissions("publish_permission", "publish_another");
        session.addAuthorizeResult("A token of thanks", new ArrayList<String>(), AccessTokenSource.TEST_USER);
        session.addCallback(statusRecorder);

        button.performClick();

        statusRecorder.waitForCall(session, SessionState.OPENING, null);
        statusRecorder.waitForCall(session, SessionState.OPENED, null);

        // Verify token information is cleared.
        session.closeAndClearTokenInformation();
        assertTrue(cache.getSavedState() == null);
        statusRecorder.waitForCall(session, SessionState.CLOSED, null);

        // Wait a bit so we can fail if any unexpected calls arrive on the
        // recorder.
        stall(STRAY_CALLBACK_WAIT_MILLISECONDS);
        statusRecorder.close();
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCantAddReadThenPublishPermissions() {
        final LoginButton button = new LoginButton(getActivity());
        button.setReadPermissions("read_permission", "read_another");
        try {
            button.setPublishPermissions("read_permission", "read_a_third");
            fail("Should not be able to reach here");
        } catch (Exception e) {
            assertTrue(e instanceof UnsupportedOperationException);
        }
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCantAddPublishThenReadPermissions() {
        final LoginButton button = new LoginButton(getActivity());
        button.setPublishPermissions("publish_permission", "publish_another");
        try {
            button.setReadPermissions("publish_permission", "publish_a_third");
            fail("Should not be able to reach here");
        } catch (Exception e) {
            assertTrue(e instanceof UnsupportedOperationException);
        }
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCanAddReadThenPublishPermissionsWithClear() {
        final LoginButton button = new LoginButton(getActivity());
        button.setReadPermissions("read_permission", "read_another");
        button.clearPermissions();
        button.setPublishPermissions("publish_permission", "publish_another");
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCantAddMorePermissionsToOpenSession() {
        MockTokenCachingStrategy cache = new MockTokenCachingStrategy(null, 0);
        ScriptedSession session = new ScriptedSession(getActivity(), "SomeId", cache);
        SessionTestsBase.SessionStatusCallbackRecorder statusRecorder =
                new SessionTestsBase.SessionStatusCallbackRecorder();

        // Verify state with no token in cache
        assertEquals(SessionState.CREATED, session.getState());

        final LoginButton button = new LoginButton(getActivity());
        button.setSession(session);
        session.addAuthorizeResult("A token of thanks",
                Arrays.asList(new String[] {"read_permission", "read_another"}), AccessTokenSource.TEST_USER);
        session.addCallback(statusRecorder);

        button.performClick();

        statusRecorder.waitForCall(session, SessionState.OPENING, null);
        statusRecorder.waitForCall(session, SessionState.OPENED, null);

        // this should be fine
        button.setReadPermissions("read_permission", "read_another");

        button.setReadPermissions("read_permission", "read_a_third");
        List<String> permissions = button.getPermissions();
        assertTrue(permissions.contains("read_permission"));
        assertTrue(permissions.contains("read_another"));
        assertFalse(permissions.contains("read_a_third"));

        // Verify token information is cleared.
        session.closeAndClearTokenInformation();
        assertTrue(cache.getSavedState() == null);
        statusRecorder.waitForCall(session, SessionState.CLOSED, null);

        // Wait a bit so we can fail if any unexpected calls arrive on the
        // recorder.
        stall(STRAY_CALLBACK_WAIT_MILLISECONDS);
        statusRecorder.close();
    }

    @SmallTest
    @MediumTest
    @LargeTest
    public void testCanSetDefaultAudience() {
        SessionTestsBase.MockTokenCachingStrategy cache = new SessionTestsBase.MockTokenCachingStrategy(null, 0);
        ScriptedSession session = new ScriptedSession(getActivity(), "SomeId", cache);
        SessionTestsBase.SessionStatusCallbackRecorder statusRecorder =
                new SessionTestsBase.SessionStatusCallbackRecorder();

        // Verify state with no token in cache
        assertEquals(SessionState.CREATED, session.getState());

        final LoginButton button = new LoginButton(getActivity());
        button.setSession(session);
        button.setPublishPermissions("publish_permission", "publish_another");
        button.setDefaultAudience(SessionDefaultAudience.FRIENDS);
        session.addAuthorizeResult("A token of thanks", new ArrayList<String>(), AccessTokenSource.TEST_USER);
        session.addCallback(statusRecorder);

        button.performClick();

        statusRecorder.waitForCall(session, SessionState.OPENING, null);
        statusRecorder.waitForCall(session, SessionState.OPENED, null);

        assertNotNull(session.getLastRequest());
        assertEquals(SessionDefaultAudience.FRIENDS, session.getLastRequestAudience());

        // Verify token information is cleared.
        session.closeAndClearTokenInformation();
        assertTrue(cache.getSavedState() == null);
        statusRecorder.waitForCall(session, SessionState.CLOSED, null);

        // Wait a bit so we can fail if any unexpected calls arrive on the
        // recorder.
        stall(STRAY_CALLBACK_WAIT_MILLISECONDS);
        statusRecorder.close();
    }

}
