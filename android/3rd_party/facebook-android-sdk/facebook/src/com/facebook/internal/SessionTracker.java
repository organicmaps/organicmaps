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

package com.facebook.internal;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.support.v4.content.LocalBroadcastManager;
import com.facebook.Session;
import com.facebook.SessionState;

/**
 * com.facebook.internal is solely for the use of other packages within the Facebook SDK for Android. Use of
 * any of the classes in this package is unsupported, and they may be modified or removed without warning at
 * any time.
 */
public class SessionTracker {

    private Session session;
    private final Session.StatusCallback callback;
    private final BroadcastReceiver receiver;
    private final LocalBroadcastManager broadcastManager;
    private boolean isTracking = false;

    /**
     * Constructs a SessionTracker to track the active Session object.
     * 
     * @param context the context object.
     * @param callback the callback to use whenever the active Session's 
     *                 state changes
     */
    public SessionTracker(Context context, Session.StatusCallback callback) {
        this(context, callback, null);
    }
    
    /**
     * Constructs a SessionTracker to track the Session object passed in.
     * If the Session is null, then it will track the active Session instead.
     * 
     * @param context the context object.
     * @param callback the callback to use whenever the Session's state changes
     * @param session the Session object to track
     */
    SessionTracker(Context context, Session.StatusCallback callback, Session session) {
        this(context, callback, session, true);
    }
    
    /**
     * Constructs a SessionTracker to track the Session object passed in.
     * If the Session is null, then it will track the active Session instead.
     * 
     * @param context the context object.
     * @param callback the callback to use whenever the Session's state changes
     * @param session the Session object to track
     * @param startTracking whether to start tracking the Session right away
     */
    public SessionTracker(Context context, Session.StatusCallback callback, Session session, boolean startTracking) {
        this.callback = new CallbackWrapper(callback);
        this.session = session;
        this.receiver = new ActiveSessionBroadcastReceiver();
        this.broadcastManager = LocalBroadcastManager.getInstance(context);

        if (startTracking) {
            startTracking();
        }
    }

    /**
     * Returns the current Session that's being tracked.
     * 
     * @return the current Session associated with this tracker
     */
    public Session getSession() {
        return (session == null) ? Session.getActiveSession() : session;
    }

    /**
     * Returns the current Session that's being tracked if it's open, 
     * otherwise returns null.
     * 
     * @return the current Session if it's open, otherwise returns null
     */
    public Session getOpenSession() {
        Session openSession = getSession();
        if (openSession != null && openSession.isOpened()) {
            return openSession;
        }
        return null;
    }

    /**
     * Set the Session object to track.
     * 
     * @param newSession the new Session object to track
     */
    public void setSession(Session newSession) {
        if (newSession == null) {
            if (session != null) {
                // We're current tracking a Session. Remove the callback
                // and start tracking the active Session.
                session.removeCallback(callback);
                session = null;
                addBroadcastReceiver();
                if (getSession() != null) {
                    getSession().addCallback(callback);
                }
            }
        } else {
            if (session == null) {
                // We're currently tracking the active Session, but will be
                // switching to tracking a different Session object.
                Session activeSession = Session.getActiveSession();
                if (activeSession != null) {
                    activeSession.removeCallback(callback);
                }
                broadcastManager.unregisterReceiver(receiver);
            } else {
                // We're currently tracking a Session, but are now switching 
                // to a new Session, so we remove the callback from the old 
                // Session, and add it to the new one.
                session.removeCallback(callback);
            }
            session = newSession;
            session.addCallback(callback);
        }
    }

    /**
     * Start tracking the Session (either active or the one given). 
     */
    public void startTracking() {
        if (isTracking) {
            return;
        }
        if (this.session == null) {
            addBroadcastReceiver();
        }        
        // if the session is not null, then add the callback to it right away
        if (getSession() != null) {
            getSession().addCallback(callback);
        }
        isTracking = true;
    }

    /**
     * Stop tracking the Session and remove any callbacks attached
     * to those sessions.
     */
    public void stopTracking() {
        if (!isTracking) {
            return;
        }
        Session session = getSession();
        if (session != null) {
            session.removeCallback(callback);
        }
        broadcastManager.unregisterReceiver(receiver);
        isTracking = false;
    }
    
    /**
     * Returns whether it's currently tracking the Session.
     * 
     * @return true if currently tracking the Session
     */
    public boolean isTracking() {
        return isTracking;
    }

    /**
     * Returns whether it's currently tracking the active Session.
     *
     * @return true if the currently tracked session is the active Session.
     */
    public boolean isTrackingActiveSession() {
        return session == null;
    }
    
    private void addBroadcastReceiver() {
        IntentFilter filter = new IntentFilter();
        filter.addAction(Session.ACTION_ACTIVE_SESSION_SET);
        filter.addAction(Session.ACTION_ACTIVE_SESSION_UNSET);
        
        // Add a broadcast receiver to listen to when the active Session
        // is set or unset, and add/remove our callback as appropriate    
        broadcastManager.registerReceiver(receiver, filter);
    }

    /**
     * The BroadcastReceiver implementation that either adds or removes the callback
     * from the active Session object as it's SET or UNSET.
     */
    private class ActiveSessionBroadcastReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (Session.ACTION_ACTIVE_SESSION_SET.equals(intent.getAction())) {
                Session session = Session.getActiveSession();
                if (session != null) {
                    session.addCallback(SessionTracker.this.callback);
                }
            }
        }
    }

    private class CallbackWrapper implements Session.StatusCallback {

        private final Session.StatusCallback wrapped;
        public CallbackWrapper(Session.StatusCallback wrapped) {
            this.wrapped = wrapped;
        }

        @Override
        public void call(Session session, SessionState state, Exception exception) {
            if (wrapped != null && isTracking()) {
                wrapped.call(session, state, exception);
            }
            // if we're not tracking the Active Session, and the current session
            // is closed, then start tracking the Active Session.
            if (session == SessionTracker.this.session && state.isClosed()) {
                setSession(null);
            }
        }
    }
}
