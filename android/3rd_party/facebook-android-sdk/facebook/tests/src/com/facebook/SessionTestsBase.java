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

import android.content.Context;
import android.os.Bundle;
import android.os.Looper;
import com.facebook.internal.Utility;

import java.util.*;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;

public class SessionTestsBase extends FacebookTestCase {
    public static final int DEFAULT_TIMEOUT_MILLISECONDS = 10 * 1000;
    static final int SIMULATED_WORKING_MILLISECONDS = 20;
    public static final int STRAY_CALLBACK_WAIT_MILLISECONDS = 50;

    public ScriptedSession createScriptedSessionOnBlockerThread(TokenCachingStrategy cachingStrategy) {
        return createScriptedSessionOnBlockerThread(Utility.getMetadataApplicationId(getActivity()), cachingStrategy);
    }

    ScriptedSession createScriptedSessionOnBlockerThread(final String applicationId,
            final TokenCachingStrategy cachingStrategy) {
        class MutableState {
            ScriptedSession session;
        }
        ;
        final MutableState mutable = new MutableState();

        runOnBlockerThread(new Runnable() {
            @Override
            public void run() {
                mutable.session = new ScriptedSession(getActivity(), applicationId, cachingStrategy);
            }
        }, true);

        return mutable.session;
    }

    public static void stall(int stallMsec) {
        try {
            Thread.sleep(stallMsec);
        } catch (InterruptedException e) {
            fail("InterruptedException while stalling");
        }
    }

    public class ScriptedSession extends Session {
        private static final long serialVersionUID = 1L;
        private final LinkedList<AuthorizeResult> pendingAuthorizations = new LinkedList<AuthorizeResult>();
        private AuthorizationRequest lastRequest;
        private AuthorizeResult currentAuthorization = null;

        public ScriptedSession(Context currentContext, String applicationId, TokenCachingStrategy tokenCachingStrategy) {
            super(currentContext, applicationId, tokenCachingStrategy);
        }

        public void addAuthorizeResult(String token, List<String> permissions, AccessTokenSource source) {
            addAuthorizeResult(AccessToken.createFromString(token, permissions, source));
        }

        public void addAuthorizeResult(AccessToken token) {
            pendingAuthorizations.add(new AuthorizeResult(token));
        }

        public void addAuthorizeResult(AccessToken token, List<String> permissions) {
            pendingAuthorizations.add(new AuthorizeResult(token, permissions));
        }

        public void addAuthorizeResult(AccessToken token, String... permissions) {
            pendingAuthorizations.add(new AuthorizeResult(token, Arrays.asList(permissions)));
        }

        public void addAuthorizeResult(Exception exception) {
            pendingAuthorizations.add(new AuthorizeResult(exception));
        }

        public void addPendingAuthorizeResult() {
            pendingAuthorizations.add(new AuthorizeResult());
        }

        public AuthorizationRequest getLastRequest() {
            return lastRequest;
        }

        public SessionDefaultAudience getLastRequestAudience() {
            return lastRequest.getDefaultAudience();
        }

        // Overrides authorize to return the next AuthorizeResult we added.
        @Override
        void authorize(final AuthorizationRequest request) {
            lastRequest = request;
            getActivity().runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    stall(SIMULATED_WORKING_MILLISECONDS);
                    currentAuthorization = pendingAuthorizations.poll();

                    if (currentAuthorization == null) {
                        fail("Missing call to addScriptedAuthorization");
                    }
                    if (!currentAuthorization.leaveAsPending) {
                        finishAuthOrReauth(currentAuthorization.token, currentAuthorization.exception);
                    }
                }
            });
        }

        private class AuthorizeResult {
            final AccessToken token;
            final Exception exception;
            final List<String> resultingPermissions;
            final boolean leaveAsPending;

            private AuthorizeResult(AccessToken token, Exception exception, List<String> permissions) {
                this.token = token;
                this.exception = exception;
                this.resultingPermissions = permissions;
                this.leaveAsPending = false;
            }

            private AuthorizeResult() {
                this.token = null;
                this.exception = null;
                this.resultingPermissions = null;
                this.leaveAsPending = true;
            }

            AuthorizeResult(AccessToken token, List<String> permissions) {
                this(token, null, permissions);
            }

            AuthorizeResult(AccessToken token) {
                this(token, null, null);
            }

            AuthorizeResult(Exception exception) {
                this(null, exception, null);
            }
        }
    }

    public static class SessionStatusCallbackRecorder implements Session.StatusCallback {
        private final BlockingQueue<Call> calls = new LinkedBlockingQueue<Call>();
        volatile boolean isClosed = false;

        public void waitForCall(Session session, SessionState state, Exception exception) {
            Call call = null;

            try {
                call = calls.poll(DEFAULT_TIMEOUT_MILLISECONDS, TimeUnit.MILLISECONDS);
                if (call == null) {
                    fail("Did not get a status callback within timeout.");
                }
            } catch (InterruptedException e) {
                fail("InterruptedException while waiting for status callback: " + e);
            }

            assertEquals(session, call.session);
            assertEquals(state, call.state);
            if (exception != null && call.exception != null) {
                assertEquals(exception.getClass(), call.exception.getClass());
            } else {
                // They should both be null if either of them is.
                assertTrue(exception == call.exception);
            }
        }

        public void close() {
            isClosed = true;
            assertEquals(0, calls.size());
        }

        @Override
        public void call(Session session, SessionState state, Exception exception) {
            Call call = new Call(session, state, exception);
            if (!calls.offer(call)) {
                fail("Test Error: Blocking queue ran out of capacity");
            }
            if (isClosed) {
                fail("Reauthorize callback called after closed");
            }
            assertEquals("Callback should run on main UI thread", Thread.currentThread(),
                    Looper.getMainLooper().getThread());
        }

        private static class Call {
            final Session session;
            final SessionState state;
            final Exception exception;

            Call(Session session, SessionState state, Exception exception) {
                this.session = session;
                this.state = state;
                this.exception = exception;
            }
        }

    }

    public static class MockTokenCachingStrategy extends TokenCachingStrategy {
        private final String token;
        private final long expires_in;
        private Bundle saved;

        MockTokenCachingStrategy() {
            this("FakeToken", DEFAULT_TIMEOUT_MILLISECONDS);
        }

        public MockTokenCachingStrategy(String token, long expires_in) {
            this.token = token;
            this.expires_in = expires_in;
            this.saved = null;
        }

        public Bundle getSavedState() {
            return saved;
        }

        @Override
        public Bundle load() {
            Bundle bundle = null;

            if (token != null) {
                bundle = new Bundle();

                TokenCachingStrategy.putToken(bundle, token);
                TokenCachingStrategy.putExpirationMilliseconds(bundle, System.currentTimeMillis() + expires_in);
            }

            return bundle;
        }

        @Override
        public void save(Bundle bundle) {
            this.saved = bundle;
        }

        @Override
        public void clear() {
            this.saved = null;
        }
    }

}
