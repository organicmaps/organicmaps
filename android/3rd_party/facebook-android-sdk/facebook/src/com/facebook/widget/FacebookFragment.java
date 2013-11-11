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

import android.content.Intent;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import com.facebook.Session;
import com.facebook.SessionLoginBehavior;
import com.facebook.SessionState;
import com.facebook.internal.SessionAuthorizationType;
import com.facebook.internal.SessionTracker;

import java.util.Date;
import java.util.List;

/**
 * <p>Basic implementation of a Fragment that uses a Session to perform 
 * Single Sign On (SSO). This class is package private, and is not intended
 * to be consumed by external applications.</p>
 * 
 * <p>The method {@link android.support.v4.app.Fragment#onActivityResult} is
 * used to manage the session information, so if you override it in a subclass, 
 * be sure to call {@code super.onActivityResult}.</p>
 * 
 * <p>The methods in this class are not thread-safe.</p>
 */
class FacebookFragment extends Fragment {

    private SessionTracker sessionTracker;

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        sessionTracker = new SessionTracker(getActivity(), new DefaultSessionStatusCallback());
    }
    
    /**
     * Called when the activity that was launched exits. This method manages session
     * information when a session is opened. If this method is overridden in subclasses,
     * be sure to call {@code super.onActivityResult(...)} first.
     */
    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        sessionTracker.getSession().onActivityResult(this.getActivity(), requestCode, resultCode, data);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        sessionTracker.stopTracking();
    }

    /**
     * Use the supplied Session object instead of the active Session.
     *
     * @param newSession the Session object to use
     */
    public void setSession(Session newSession) {
        if (sessionTracker != null) {
            sessionTracker.setSession(newSession);
        }
    }

    // METHOD TO BE OVERRIDDEN
    
    /**
     * Called when the session state changes. Override this method to take action
     * on session state changes.
     * 
     * @param state the new state
     * @param exception any exceptions that occurred during the state change
     */
    protected void onSessionStateChange(SessionState state, Exception exception) {
    }

    // ACCESSORS (CANNOT BE OVERRIDDEN)
    
    /**
     * Gets the current Session.
     * 
     * @return the current Session object.
     */
    protected final Session getSession() {
        if (sessionTracker != null) {
            return sessionTracker.getSession();
        }
        return null;
    }

    /**
     * Determines whether the current session is open.
     * 
     * @return true if the current session is open
     */
    protected final boolean isSessionOpen() {
        if (sessionTracker != null) {
            return sessionTracker.getOpenSession() != null;
        }
        return false;
    }
    
    /**
     * Gets the current state of the session or null if no session has been created.
     * 
     * @return the current state of the session
     */
    protected final SessionState getSessionState() {
        if (sessionTracker != null) {
            Session currentSession = sessionTracker.getSession();
            return (currentSession != null) ? currentSession.getState() : null;
        }
        return null;
    }
    
    /**
     * Gets the access token associated with the current session or null if no 
     * session has been created.
     * 
     * @return the access token
     */
    protected final String getAccessToken() {
        if (sessionTracker != null) {
            Session currentSession = sessionTracker.getOpenSession();
            return (currentSession != null) ? currentSession.getAccessToken() : null;
        }
        return null;
    }

    /**
     * Gets the date at which the current session will expire or null if no session 
     * has been created.
     * 
     * @return the date at which the current session will expire
     */
    protected final Date getExpirationDate() {
        if (sessionTracker != null) {
            Session currentSession = sessionTracker.getOpenSession();
            return (currentSession != null) ? currentSession.getExpirationDate() : null;
        }
        return null;
    }
    
    /**
     * Closes the current session.
     */
    protected final void closeSession() {
        if (sessionTracker != null) {
            Session currentSession = sessionTracker.getOpenSession();
            if (currentSession != null) {
                currentSession.close();
            }
        }
    }
    
    /**
     * Closes the current session as well as clearing the token cache.
     */
    protected final void closeSessionAndClearTokenInformation() {
        if (sessionTracker != null) {
            Session currentSession = sessionTracker.getOpenSession();
            if (currentSession != null) {
                currentSession.closeAndClearTokenInformation();
            }
        }
    }
    
    /**
     * Gets the permissions associated with the current session or null if no session 
     * has been created.
     * 
     * @return the permissions associated with the current session
     */
    protected final List<String> getSessionPermissions() {
        if (sessionTracker != null) {
            Session currentSession = sessionTracker.getSession();
            return (currentSession != null) ? currentSession.getPermissions() : null;
        }
        return null;
    }

    /**
     * Opens a new session. This method will use the application id from
     * the associated meta-data value and an empty list of permissions.
     */
    protected final void openSession() {
        openSessionForRead(null, null);
    }

    /**
     * Opens a new session with read permissions. If either applicationID or permissions
     * is null, this method will default to using the values from the associated
     * meta-data value and an empty list respectively.
     *
     * @param applicationId the applicationID, can be null
     * @param permissions the permissions list, can be null
     */
    protected final void openSessionForRead(String applicationId, List<String> permissions) {
        openSessionForRead(applicationId, permissions, SessionLoginBehavior.SSO_WITH_FALLBACK,
                Session.DEFAULT_AUTHORIZE_ACTIVITY_CODE);
    }

    /**
     * Opens a new session with read permissions. If either applicationID or permissions
     * is null, this method will default to using the values from the associated
     * meta-data value and an empty list respectively.
     *
     * @param applicationId the applicationID, can be null
     * @param permissions the permissions list, can be null
     * @param behavior the login behavior to use with the session
     * @param activityCode the activity code to use for the SSO activity
     */
    protected final void openSessionForRead(String applicationId, List<String> permissions,
            SessionLoginBehavior behavior, int activityCode) {
        openSession(applicationId, permissions, behavior, activityCode, SessionAuthorizationType.READ);
    }

    /**
     * Opens a new session with publish permissions. If either applicationID is null,
     * this method will default to using the value from the associated
     * meta-data value. The permissions list cannot be null.
     *
     * @param applicationId the applicationID, can be null
     * @param permissions the permissions list, cannot be null
     */
    protected final void openSessionForPublish(String applicationId, List<String> permissions) {
        openSessionForPublish(applicationId, permissions, SessionLoginBehavior.SSO_WITH_FALLBACK,
                Session.DEFAULT_AUTHORIZE_ACTIVITY_CODE);
    }

    /**
     * Opens a new session with publish permissions. If either applicationID is null,
     * this method will default to using the value from the associated
     * meta-data value. The permissions list cannot be null.
     *
     * @param applicationId the applicationID, can be null
     * @param permissions the permissions list, cannot be null
     * @param behavior the login behavior to use with the session
     * @param activityCode the activity code to use for the SSO activity
     */
    protected final void openSessionForPublish(String applicationId, List<String> permissions,
            SessionLoginBehavior behavior, int activityCode) {
        openSession(applicationId, permissions, behavior, activityCode, SessionAuthorizationType.PUBLISH);
    }

    private void openSession(String applicationId, List<String> permissions,
            SessionLoginBehavior behavior, int activityCode, SessionAuthorizationType authType) {
        if (sessionTracker != null) {
            Session currentSession = sessionTracker.getSession();
            if (currentSession == null || currentSession.getState().isClosed()) {
                Session session = new Session.Builder(getActivity()).setApplicationId(applicationId).build();
                Session.setActiveSession(session);
                currentSession = session;
            }
            if (!currentSession.isOpened()) {
                Session.OpenRequest openRequest = new Session.OpenRequest(this).
                        setPermissions(permissions).
                        setLoginBehavior(behavior).
                        setRequestCode(activityCode);
                if (SessionAuthorizationType.PUBLISH.equals(authType)) {
                    currentSession.openForPublish(openRequest);
                } else {
                    currentSession.openForRead(openRequest);
                }
            }
        }
    }

    /**
     * The default callback implementation for the session.
     */
    private class DefaultSessionStatusCallback implements Session.StatusCallback {

        @Override
        public void call(Session session, 
                         SessionState state,
                         Exception exception) {
            FacebookFragment.this.onSessionStateChange(state, exception);
        }
        
    }
}
