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
import android.content.*;
import android.content.pm.ResolveInfo;
import android.os.*;
import android.support.v4.app.Fragment;
import android.support.v4.content.LocalBroadcastManager;
import android.text.TextUtils;
import android.util.Log;
import com.facebook.internal.*;
import com.facebook.model.GraphMultiResult;
import com.facebook.model.GraphObject;
import com.facebook.model.GraphObjectList;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.*;
import java.lang.ref.WeakReference;
import java.util.*;

/**
 * <p>
 * Session is used to authenticate a user and manage the user's session with
 * Facebook.
 * </p>
 * <p>
 * Sessions must be opened before they can be used to make a Request. When a
 * Session is created, it attempts to initialize itself from a TokenCachingStrategy.
 * Closing the session can optionally clear this cache.  The Session lifecycle
 * uses {@link SessionState SessionState} to indicate its state. Once a Session has
 * been closed, it can't be re-opened; a new Session must be created.
 * </p>
 * <p>
 * Instances of Session provide state change notification via a callback
 * interface, {@link Session.StatusCallback StatusCallback}.
 * </p>
 */
public class Session implements Serializable {
    private static final long serialVersionUID = 1L;

    /**
     * The logging tag used by Session.
     */
    public static final String TAG = Session.class.getCanonicalName();

    /**
     * The default activity code used for authorization.
     *
     * @see #openForRead(OpenRequest)
     *      open
     */
    public static final int DEFAULT_AUTHORIZE_ACTIVITY_CODE = 0xface;

    /**
     * If Session authorization fails and provides a web view error code, the
     * web view error code is stored in the Bundle returned from
     * {@link #getAuthorizationBundle getAuthorizationBundle} under this key.
     */
    public static final String WEB_VIEW_ERROR_CODE_KEY = "com.facebook.sdk.WebViewErrorCode";

    /**
     * If Session authorization fails and provides a failing url, the failing
     * url is stored in the Bundle returned from {@link #getAuthorizationBundle
     * getAuthorizationBundle} under this key.
     */
    public static final String WEB_VIEW_FAILING_URL_KEY = "com.facebook.sdk.FailingUrl";

    /**
     * The action used to indicate that the active session has been set. This should
     * be used as an action in an IntentFilter and BroadcastReceiver registered with
     * the {@link android.support.v4.content.LocalBroadcastManager}.
     */
    public static final String ACTION_ACTIVE_SESSION_SET = "com.facebook.sdk.ACTIVE_SESSION_SET";

    /**
     * The action used to indicate that the active session has been set to null. This should
     * be used as an action in an IntentFilter and BroadcastReceiver registered with
     * the {@link android.support.v4.content.LocalBroadcastManager}.
     */
    public static final String ACTION_ACTIVE_SESSION_UNSET = "com.facebook.sdk.ACTIVE_SESSION_UNSET";

    /**
     * The action used to indicate that the active session has been opened. This should
     * be used as an action in an IntentFilter and BroadcastReceiver registered with
     * the {@link android.support.v4.content.LocalBroadcastManager}.
     */
    public static final String ACTION_ACTIVE_SESSION_OPENED = "com.facebook.sdk.ACTIVE_SESSION_OPENED";

    /**
     * The action used to indicate that the active session has been closed. This should
     * be used as an action in an IntentFilter and BroadcastReceiver registered with
     * the {@link android.support.v4.content.LocalBroadcastManager}.
     */
    public static final String ACTION_ACTIVE_SESSION_CLOSED = "com.facebook.sdk.ACTIVE_SESSION_CLOSED";

    private static final Object STATIC_LOCK = new Object();
    private static Session activeSession;
    private static volatile Context staticContext;

    // Token extension constants
    private static final int TOKEN_EXTEND_THRESHOLD_SECONDS = 24 * 60 * 60; // 1
    // day
    private static final int TOKEN_EXTEND_RETRY_SECONDS = 60 * 60; // 1 hour

    private static final String SESSION_BUNDLE_SAVE_KEY = "com.facebook.sdk.Session.saveSessionKey";
    private static final String AUTH_BUNDLE_SAVE_KEY = "com.facebook.sdk.Session.authBundleKey";
    private static final String PUBLISH_PERMISSION_PREFIX = "publish";
    private static final String MANAGE_PERMISSION_PREFIX = "manage";

    private static final String BASIC_INFO_PERMISSION = "basic_info";

    @SuppressWarnings("serial")
    private static final Set<String> OTHER_PUBLISH_PERMISSIONS = new HashSet<String>() {{
        add("ads_management");
        add("create_event");
        add("rsvp_event");
    }};

    private String applicationId;
    private SessionState state;
    private AccessToken tokenInfo;
    private Date lastAttemptedTokenExtendDate = new Date(0);

    private AuthorizationRequest pendingAuthorizationRequest;
    private AuthorizationClient authorizationClient;

    // The following are not serialized with the Session object
    private volatile Bundle authorizationBundle;
    private final List<StatusCallback> callbacks;
    private Handler handler;
    private AutoPublishAsyncTask autoPublishAsyncTask;
    // This is the object that synchronizes access to state and tokenInfo
    private final Object lock = new Object();
    private TokenCachingStrategy tokenCachingStrategy;
    private volatile TokenRefreshRequest currentTokenRefreshRequest;
    private AppEventsLogger appEventsLogger;

    /**
     * Serialization proxy for the Session class. This is version 1 of
     * serialization. Future serializations may differ in format. This
     * class should not be modified. If serializations formats change,
     * create a new class SerializationProxyVx.
     */
    private static class SerializationProxyV1 implements Serializable {
        private static final long serialVersionUID = 7663436173185080063L;
        private final String applicationId;
        private final SessionState state;
        private final AccessToken tokenInfo;
        private final Date lastAttemptedTokenExtendDate;
        private final boolean shouldAutoPublish;
        private final AuthorizationRequest pendingAuthorizationRequest;

        SerializationProxyV1(String applicationId, SessionState state,
                AccessToken tokenInfo, Date lastAttemptedTokenExtendDate,
                boolean shouldAutoPublish, AuthorizationRequest pendingAuthorizationRequest) {
            this.applicationId = applicationId;
            this.state = state;
            this.tokenInfo = tokenInfo;
            this.lastAttemptedTokenExtendDate = lastAttemptedTokenExtendDate;
            this.shouldAutoPublish = shouldAutoPublish;
            this.pendingAuthorizationRequest = pendingAuthorizationRequest;
        }

        private Object readResolve() {
            return new Session(applicationId, state, tokenInfo,
                    lastAttemptedTokenExtendDate, shouldAutoPublish, pendingAuthorizationRequest);
        }
    }

    /**
     * Serialization proxy for the Session class. This is version 2 of
     * serialization. Future serializations may differ in format. This
     * class should not be modified. If serializations formats change,
     * create a new class SerializationProxyVx.
     */
    private static class SerializationProxyV2 implements Serializable {
        private static final long serialVersionUID = 7663436173185080064L;
        private final String applicationId;
        private final SessionState state;
        private final AccessToken tokenInfo;
        private final Date lastAttemptedTokenExtendDate;
        private final boolean shouldAutoPublish;
        private final AuthorizationRequest pendingAuthorizationRequest;
        private final Set<String> requestedPermissions;

        SerializationProxyV2(String applicationId, SessionState state,
                             AccessToken tokenInfo, Date lastAttemptedTokenExtendDate,
                             boolean shouldAutoPublish, AuthorizationRequest pendingAuthorizationRequest,
                             Set<String> requestedPermissions) {
            this.applicationId = applicationId;
            this.state = state;
            this.tokenInfo = tokenInfo;
            this.lastAttemptedTokenExtendDate = lastAttemptedTokenExtendDate;
            this.shouldAutoPublish = shouldAutoPublish;
            this.pendingAuthorizationRequest = pendingAuthorizationRequest;
            this.requestedPermissions = requestedPermissions;
        }

        private Object readResolve() {
            return new Session(applicationId, state, tokenInfo,
                    lastAttemptedTokenExtendDate, shouldAutoPublish, pendingAuthorizationRequest, requestedPermissions);
        }
    }

    /**
     * Used by version 1 of the serialization proxy, do not modify.
     */
    private Session(String applicationId, SessionState state,
            AccessToken tokenInfo, Date lastAttemptedTokenExtendDate,
            boolean shouldAutoPublish, AuthorizationRequest pendingAuthorizationRequest) {
        this.applicationId = applicationId;
        this.state = state;
        this.tokenInfo = tokenInfo;
        this.lastAttemptedTokenExtendDate = lastAttemptedTokenExtendDate;
        this.pendingAuthorizationRequest = pendingAuthorizationRequest;
        handler = new Handler(Looper.getMainLooper());
        currentTokenRefreshRequest = null;
        tokenCachingStrategy = null;
        callbacks = new ArrayList<StatusCallback>();
    }

    /**
     * Used by version 2 of the serialization proxy, do not modify.
     */
    private Session(String applicationId, SessionState state,
                    AccessToken tokenInfo, Date lastAttemptedTokenExtendDate,
                    boolean shouldAutoPublish, AuthorizationRequest pendingAuthorizationRequest,
                    Set<String> requestedPermissions) {
        this.applicationId = applicationId;
        this.state = state;
        this.tokenInfo = tokenInfo;
        this.lastAttemptedTokenExtendDate = lastAttemptedTokenExtendDate;
        this.pendingAuthorizationRequest = pendingAuthorizationRequest;
        handler = new Handler(Looper.getMainLooper());
        currentTokenRefreshRequest = null;
        tokenCachingStrategy = null;
        callbacks = new ArrayList<StatusCallback>();
    }

    /**
     * Initializes a new Session with the specified context.
     *
     * @param currentContext The Activity or Service creating this Session.
     */
    public Session(Context currentContext) {
        this(currentContext, null, null, true);
    }

    Session(Context context, String applicationId, TokenCachingStrategy tokenCachingStrategy) {
        this(context, applicationId, tokenCachingStrategy, true);
    }

    Session(Context context, String applicationId, TokenCachingStrategy tokenCachingStrategy,
            boolean loadTokenFromCache) {
        // if the application ID passed in is null, try to get it from the
        // meta-data in the manifest.
        if ((context != null) && (applicationId == null)) {
            applicationId = Utility.getMetadataApplicationId(context);
        }

        Validate.notNull(applicationId, "applicationId");

        initializeStaticContext(context);

        if (tokenCachingStrategy == null) {
            tokenCachingStrategy = new SharedPreferencesTokenCachingStrategy(staticContext);
        }

        this.applicationId = applicationId;
        this.tokenCachingStrategy = tokenCachingStrategy;
        this.state = SessionState.CREATED;
        this.pendingAuthorizationRequest = null;
        this.callbacks = new ArrayList<StatusCallback>();
        this.handler = new Handler(Looper.getMainLooper());

        Bundle tokenState = loadTokenFromCache ? tokenCachingStrategy.load() : null;
        if (TokenCachingStrategy.hasTokenInformation(tokenState)) {
            Date cachedExpirationDate = TokenCachingStrategy
                    .getDate(tokenState, TokenCachingStrategy.EXPIRATION_DATE_KEY);
            Date now = new Date();

            if ((cachedExpirationDate == null) || cachedExpirationDate.before(now)) {
                // If expired or we require new permissions, clear out the
                // current token cache.
                tokenCachingStrategy.clear();
                this.tokenInfo = AccessToken.createEmptyToken();
            } else {
                // Otherwise we have a valid token, so use it.
                this.tokenInfo = AccessToken.createFromCache(tokenState);
                this.state = SessionState.CREATED_TOKEN_LOADED;
            }
        } else {
            this.tokenInfo = AccessToken.createEmptyToken();
        }
    }

    /**
     * Returns a Bundle containing data that was returned from Facebook during
     * authorization.
     *
     * @return a Bundle containing data that was returned from Facebook during
     *         authorization.
     */
    public final Bundle getAuthorizationBundle() {
        synchronized (this.lock) {
            return this.authorizationBundle;
        }
    }

    /**
     * Returns a boolean indicating whether the session is opened.
     *
     * @return a boolean indicating whether the session is opened.
     */
    public final boolean isOpened() {
        synchronized (this.lock) {
            return this.state.isOpened();
        }
    }

    public final boolean isClosed() {
        synchronized (this.lock) {
            return this.state.isClosed();
        }
    }

    /**
     * Returns the current state of the Session.
     * See {@link SessionState} for details.
     *
     * @return the current state of the Session.
     */
    public final SessionState getState() {
        synchronized (this.lock) {
            return this.state;
        }
    }

    /**
     * Returns the application id associated with this Session.
     *
     * @return the application id associated with this Session.
     */
    public final String getApplicationId() {
        return this.applicationId;
    }

    /**
     * Returns the access token String.
     *
     * @return the access token String, or null if there is no access token
     */
    public final String getAccessToken() {
        synchronized (this.lock) {
            return (this.tokenInfo == null) ? null : this.tokenInfo.getToken();
        }
    }

    /**
     * <p>
     * Returns the Date at which the current token will expire.
     * </p>
     * <p>
     * Note that Session automatically attempts to extend the lifetime of Tokens
     * as needed when Facebook requests are made.
     * </p>
     *
     * @return the Date at which the current token will expire, or null if there is no access token
     */
    public final Date getExpirationDate() {
        synchronized (this.lock) {
            return (this.tokenInfo == null) ? null : this.tokenInfo.getExpires();
        }
    }

    /**
     * <p>
     * Returns the list of permissions associated with the session.
     * </p>
     * <p>
     * If there is a valid token, this represents the permissions granted by
     * that token. This can change during calls to
     * {@link #requestNewReadPermissions}
     * or {@link #requestNewPublishPermissions}.
     * </p>
     *
     * @return the list of permissions associated with the session, or null if there is no access token
     */
    public final List<String> getPermissions() {
        synchronized (this.lock) {
            return (this.tokenInfo == null) ? null : this.tokenInfo.getPermissions();
        }
    }

    /**
     * <p>
     *     Returns whether a particular permission has been granted
     * </p>
     *
     * @param permission The permission to check for
     * @return true if the permission is granted, false otherwise
     */
    public boolean isPermissionGranted(String permission) {
        List<String> grantedPermissions = getPermissions();
        if (grantedPermissions != null) {
            return grantedPermissions.contains(permission);
        }
        return false;
    }

    /**
     * <p>
     * Returns the list of permissions that have been requested in this session but not granted
     * </p>
     *
     * @return the list of requested permissions that have been declined
     */
    public final List<String> getDeclinedPermissions() {
        synchronized (this.lock) {
            return (this.tokenInfo == null) ? null : this.tokenInfo.getDeclinedPermissions();
        }
    }

    /**
     * <p>
     * Logs a user in to Facebook.
     * </p>
     * <p>
     * A session may not be used with {@link Request Request} and other classes
     * in the SDK until it is open. If, prior to calling open, the session is in
     * the {@link SessionState#CREATED_TOKEN_LOADED CREATED_TOKEN_LOADED}
     * state, and the requested permissions are a subset of the previously authorized
     * permissions, then the Session becomes usable immediately with no user interaction.
     * </p>
     * <p>
     * The permissions associated with the openRequest passed to this method must
     * be read permissions only (or null/empty). It is not allowed to pass publish
     * permissions to this method and will result in an exception being thrown.
     * </p>
     * <p>
     * Any open method must be called at most once, and cannot be called after the
     * Session is closed. Calling the method at an invalid time will result in
     * UnsuportedOperationException.
     * </p>
     *
     * @param openRequest the open request, can be null only if the Session is in the
     *                    {@link SessionState#CREATED_TOKEN_LOADED CREATED_TOKEN_LOADED} state
     * @throws FacebookException if any publish or manage permissions are requested
     */
    public final void openForRead(OpenRequest openRequest) {
        open(openRequest, SessionAuthorizationType.READ);
    }

    /**
     * <p>
     * Logs a user in to Facebook.
     * </p>
     * <p>
     * A session may not be used with {@link Request Request} and other classes
     * in the SDK until it is open. If, prior to calling open, the session is in
     * the {@link SessionState#CREATED_TOKEN_LOADED CREATED_TOKEN_LOADED}
     * state, and the requested permissions are a subset of the previously authorized
     * permissions, then the Session becomes usable immediately with no user interaction.
     * </p>
     * <p>
     * The permissions associated with the openRequest passed to this method must
     * be publish or manage permissions only and must be non-empty. Any read permissions
     * will result in a warning, and may fail during server-side authorization. Also, an application
     * must have at least basic read permissions prior to requesting publish permissions, so
     * this method should only be used if the application knows that the user has already granted
     * read permissions to the application; otherwise, openForRead should be used, followed by a
     * call to requestNewPublishPermissions. For more information on this flow, see
     * https://developers.facebook.com/docs/facebook-login/permissions/.
     * </p>
     * <p>
     * Any open method must be called at most once, and cannot be called after the
     * Session is closed. Calling the method at an invalid time will result in
     * UnsuportedOperationException.
     * </p>
     *
     * @param openRequest the open request, can be null only if the Session is in the
     *                    {@link SessionState#CREATED_TOKEN_LOADED CREATED_TOKEN_LOADED} state
     * @throws FacebookException if the passed in request is null or has no permissions set.
     */
    public final void openForPublish(OpenRequest openRequest) {
        open(openRequest, SessionAuthorizationType.PUBLISH);
    }

    /**
     * Opens a session based on an existing Facebook access token. This method should be used
     * only in instances where an application has previously obtained an access token and wishes
     * to import it into the Session/TokenCachingStrategy-based session-management system. An
     * example would be an application which previously did not use the Facebook SDK for Android
     * and implemented its own session-management scheme, but wishes to implement an upgrade path
     * for existing users so they do not need to log in again when upgrading to a version of
     * the app that uses the SDK.
     * <p/>
     * No validation is done that the token, token source, or permissions are actually valid.
     * It is the caller's responsibility to ensure that these accurately reflect the state of
     * the token that has been passed in, or calls to the Facebook API may fail.
     *
     * @param accessToken the access token obtained from Facebook
     * @param callback    a callback that will be called when the session status changes; may be null
     */
    public final void open(AccessToken accessToken, StatusCallback callback) {
        synchronized (this.lock) {
            if (pendingAuthorizationRequest != null) {
                throw new UnsupportedOperationException(
                        "Session: an attempt was made to open a session that has a pending request.");
            }

            if (state.isClosed()) {
                throw new UnsupportedOperationException(
                        "Session: an attempt was made to open a previously-closed session.");
            } else if (state != SessionState.CREATED && state != SessionState.CREATED_TOKEN_LOADED) {
                throw new UnsupportedOperationException(
                        "Session: an attempt was made to open an already opened session.");
            }

            if (callback != null) {
                addCallback(callback);
            }

            this.tokenInfo = accessToken;

            if (this.tokenCachingStrategy != null) {
                this.tokenCachingStrategy.save(accessToken.toCacheBundle());
            }

            final SessionState oldState = state;
            state = SessionState.OPENED;
            this.postStateChange(oldState, state, null);
        }

        autoPublishAsync();
    }

    /**
     * <p>
     * Issues a request to add new read permissions to the Session.
     * </p>
     * <p>
     * If successful, this will update the set of permissions on this session to
     * match the newPermissions. If this fails, the Session remains unchanged.
     * </p>
     * <p>
     * The permissions associated with the newPermissionsRequest passed to this method must
     * be read permissions only (or null/empty). It is not allowed to pass publish
     * permissions to this method and will result in an exception being thrown.
     * </p>
     *
     * @param newPermissionsRequest the new permissions request
     */
    public final void requestNewReadPermissions(NewPermissionsRequest newPermissionsRequest) {
        requestNewPermissions(newPermissionsRequest, SessionAuthorizationType.READ);
    }

    /**
     * <p>
     * Issues a request to add new publish or manage permissions to the Session.
     * </p>
     * <p>
     * If successful, this will update the set of permissions on this session to
     * match the newPermissions. If this fails, the Session remains unchanged.
     * </p>
     * <p>
     * The permissions associated with the newPermissionsRequest passed to this method must
     * be publish or manage permissions only and must be non-empty. Any read permissions
     * will result in a warning, and may fail during server-side authorization.
     * </p>
     *
     * @param newPermissionsRequest the new permissions request
     */
    public final void requestNewPublishPermissions(NewPermissionsRequest newPermissionsRequest) {
        requestNewPermissions(newPermissionsRequest, SessionAuthorizationType.PUBLISH);
    }

    /**
     * <p>
     * Issues a request to refresh the permissions on the session.
     * </p>
     * <p>
     * If successful, this will update the permissions and call the app back with
     * {@link SessionState#OPENED_TOKEN_UPDATED}.  The session can then be queried to see the granted and declined
     * permissions.  If this fails because the user has removed the app, the session will close.
     * </p>
     */
    public final void refreshPermissions() {
        Request request = new Request(this, "me/permissions");
        request.setCallback(new Request.Callback() {
            @Override
            public void onCompleted(Response response) {
                PermissionsPair permissionsPair = handlePermissionResponse(response);
                if (permissionsPair != null) {
                    // Update our token with the refreshed permissions
                    synchronized (lock) {
                        tokenInfo = AccessToken.createFromTokenWithRefreshedPermissions(tokenInfo,
                                permissionsPair.getGrantedPermissions(), permissionsPair.getDeclinedPermissions());
                        postStateChange(state, SessionState.OPENED_TOKEN_UPDATED, null);
                    }
                }
            }
        });
        request.executeAsync();
    }

    /**
     * Internal helper class that is used to hold two different permission lists (granted and declined)
     */
    static class PermissionsPair {
        List<String> grantedPermissions;
        List<String> declinedPermissions;

        public PermissionsPair(List<String> grantedPermissions, List<String> declinedPermissions) {
            this.grantedPermissions = grantedPermissions;
            this.declinedPermissions = declinedPermissions;
        }

        public List<String> getGrantedPermissions() {
            return grantedPermissions;
        }

        public List<String> getDeclinedPermissions() {
            return declinedPermissions;
        }
    }
    /**
     * This parses a server response to a call to me/permissions.  It will return the list of granted permissions.
     * It will optionally update a session with the requested permissions.  It also handles the distinction between
     * 1.0 and 2.0 calls to the endpoint.
     *
     * @param response The server response
     * @return A list of granted permissions or null if an error
     */
    static PermissionsPair handlePermissionResponse(Response response) {
        if (response.getError() != null) {
            return null;
        }

        GraphMultiResult result = response.getGraphObjectAs(GraphMultiResult.class);
        if (result == null) {
            return null;
        }

        GraphObjectList<GraphObject> data = result.getData();
        if (data == null || data.size() == 0) {
            return null;
        }
        List<String> grantedPermissions = new ArrayList<String>(data.size());
        List<String> declinedPermissions = new ArrayList<String>(data.size());

        // Check if we are dealing with v2.0 or v1.0 behavior until the server is updated
        GraphObject firstObject = data.get(0);
        if (firstObject.getProperty("permission") != null) { // v2.0
            for (GraphObject graphObject : data) {
                String permission = (String) graphObject.getProperty("permission");
                if (permission.equals("installed")) {
                    continue;
                }
                String status = (String) graphObject.getProperty("status");
                if(status.equals("granted")) {
                    grantedPermissions.add(permission);
                } else if (status.equals("declined")) {
                    declinedPermissions.add(permission);
                }
            }
        } else { // v1.0
            Map<String, Object> permissionsMap = firstObject.asMap();
            for (Map.Entry<String, Object> entry : permissionsMap.entrySet()) {
                if (entry.getKey().equals("installed")) {
                    continue;
                }
                if ((Integer)entry.getValue() == 1) {
                    grantedPermissions.add(entry.getKey());
                }
            }
        }

        return new PermissionsPair(grantedPermissions, declinedPermissions);
    }

    /**
     * Provides an implementation for {@link Activity#onActivityResult
     * onActivityResult} that updates the Session based on information returned
     * during the authorization flow. The Activity that calls open or
     * requestNewPermissions should forward the resulting onActivityResult call here to
     * update the Session state based on the contents of the resultCode and
     * data.
     *
     * @param currentActivity The Activity that is forwarding the onActivityResult call.
     * @param requestCode     The requestCode parameter from the forwarded call. When this
     *                        onActivityResult occurs as part of Facebook authorization
     *                        flow, this value is the activityCode passed to open or
     *                        authorize.
     * @param resultCode      An int containing the resultCode parameter from the forwarded
     *                        call.
     * @param data            The Intent passed as the data parameter from the forwarded
     *                        call.
     * @return A boolean indicating whether the requestCode matched a pending
     *         authorization request for this Session.
     */
    public final boolean onActivityResult(Activity currentActivity, int requestCode, int resultCode, Intent data) {
        Validate.notNull(currentActivity, "currentActivity");

        initializeStaticContext(currentActivity);

        synchronized (lock) {
            if (pendingAuthorizationRequest == null || (requestCode != pendingAuthorizationRequest.getRequestCode())) {
                return false;
            }
        }

        Exception exception = null;
        AuthorizationClient.Result.Code code = AuthorizationClient.Result.Code.ERROR;

        if (data != null) {
            AuthorizationClient.Result result = (AuthorizationClient.Result) data.getSerializableExtra(
                    LoginActivity.RESULT_KEY);
            if (result != null) {
                // This came from LoginActivity.
                handleAuthorizationResult(resultCode, result);
                return true;
            } else if (authorizationClient != null) {
                // Delegate to the auth client.
                authorizationClient.onActivityResult(requestCode, resultCode, data);
                return true;
            }
        } else if (resultCode == Activity.RESULT_CANCELED) {
            exception = new FacebookOperationCanceledException("User canceled operation.");
            code = AuthorizationClient.Result.Code.CANCEL;
        }

        if (exception == null) {
            exception = new FacebookException("Unexpected call to Session.onActivityResult");
        }

        logAuthorizationComplete(code, null, exception);
        finishAuthOrReauth(null, exception);

        return true;
    }

    /**
     * Closes the local in-memory Session object, but does not clear the
     * persisted token cache.
     */
    public final void close() {
        synchronized (this.lock) {
            final SessionState oldState = this.state;

            switch (this.state) {
                case CREATED:
                case OPENING:
                    this.state = SessionState.CLOSED_LOGIN_FAILED;
                    postStateChange(oldState, this.state, new FacebookException(
                            "Log in attempt aborted."));
                    break;

                case CREATED_TOKEN_LOADED:
                case OPENED:
                case OPENED_TOKEN_UPDATED:
                    this.state = SessionState.CLOSED;
                    postStateChange(oldState, this.state, null);
                    break;

                case CLOSED:
                case CLOSED_LOGIN_FAILED:
                    break;
            }
        }
    }

    /**
     * Closes the local in-memory Session object and clears any persisted token
     * cache related to the Session.
     */
    public final void closeAndClearTokenInformation() {
        if (this.tokenCachingStrategy != null) {
            this.tokenCachingStrategy.clear();
        }
        Utility.clearFacebookCookies(staticContext);
        Utility.clearCaches(staticContext);
        close();
    }

    /**
     * Adds a callback that will be called when the state of this Session changes.
     *
     * @param callback the callback
     */
    public final void addCallback(StatusCallback callback) {
        synchronized (callbacks) {
            if (callback != null && !callbacks.contains(callback)) {
                callbacks.add(callback);
            }
        }
    }

    /**
     * Removes a StatusCallback from this Session.
     *
     * @param callback the callback
     */
    public final void removeCallback(StatusCallback callback) {
        synchronized (callbacks) {
            callbacks.remove(callback);
        }
    }

    @Override
    public String toString() {
        return new StringBuilder().append("{Session").append(" state:").append(this.state).append(", token:")
                .append((this.tokenInfo == null) ? "null" : this.tokenInfo).append(", appId:")
                .append((this.applicationId == null) ? "null" : this.applicationId).append("}").toString();
    }

    void extendTokenCompleted(Bundle bundle) {
        synchronized (this.lock) {
            final SessionState oldState = this.state;

            switch (this.state) {
                case OPENED:
                    this.state = SessionState.OPENED_TOKEN_UPDATED;
                    postStateChange(oldState, this.state, null);
                    break;
                case OPENED_TOKEN_UPDATED:
                    break;
                default:
                    // Silently ignore attempts to refresh token if we are not open
                    Log.d(TAG, "refreshToken ignored in state " + this.state);
                    return;
            }
            this.tokenInfo = AccessToken.createFromRefresh(this.tokenInfo, bundle);
            if (this.tokenCachingStrategy != null) {
                this.tokenCachingStrategy.save(this.tokenInfo.toCacheBundle());
            }
        }
    }

    private Object writeReplace() {
        return new SerializationProxyV1(applicationId, state, tokenInfo,
                lastAttemptedTokenExtendDate, false, pendingAuthorizationRequest);
    }

    // have a readObject that throws to prevent spoofing
    private void readObject(ObjectInputStream stream) throws InvalidObjectException {
        throw new InvalidObjectException("Cannot readObject, serialization proxy required");
    }

    /**
     * Save the Session object into the supplied Bundle. This method is intended to be called from an
     * Activity or Fragment's onSaveInstanceState method in order to preserve Sessions across Activity lifecycle events.
     *
     * @param session the Session to save
     * @param bundle  the Bundle to save the Session to
     */
    public static final void saveSession(Session session, Bundle bundle) {
        if (bundle != null && session != null && !bundle.containsKey(SESSION_BUNDLE_SAVE_KEY)) {
            ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
            try {
                new ObjectOutputStream(outputStream).writeObject(session);
            } catch (IOException e) {
                throw new FacebookException("Unable to save session.", e);
            }
            bundle.putByteArray(SESSION_BUNDLE_SAVE_KEY, outputStream.toByteArray());
            bundle.putBundle(AUTH_BUNDLE_SAVE_KEY, session.authorizationBundle);
        }
    }

    /**
     * Restores the saved session from a Bundle, if any. Returns the restored Session or
     * null if it could not be restored. This method is intended to be called from an Activity or Fragment's
     * onCreate method when a Session has previously been saved into a Bundle via saveState to preserve a Session
     * across Activity lifecycle events.
     *
     * @param context         the Activity or Service creating the Session, must not be null
     * @param cachingStrategy the TokenCachingStrategy to use to load and store the token. If this is
     *                        null, a default token cachingStrategy that stores data in
     *                        SharedPreferences will be used
     * @param callback        the callback to notify for Session state changes, can be null
     * @param bundle          the bundle to restore the Session from
     * @return the restored Session, or null
     */
    public static final Session restoreSession(
            Context context, TokenCachingStrategy cachingStrategy, StatusCallback callback, Bundle bundle) {
        if (bundle == null) {
            return null;
        }
        byte[] data = bundle.getByteArray(SESSION_BUNDLE_SAVE_KEY);
        if (data != null) {
            ByteArrayInputStream is = new ByteArrayInputStream(data);
            try {
                Session session = (Session) (new ObjectInputStream(is)).readObject();
                initializeStaticContext(context);
                if (cachingStrategy != null) {
                    session.tokenCachingStrategy = cachingStrategy;
                } else {
                    session.tokenCachingStrategy = new SharedPreferencesTokenCachingStrategy(context);
                }
                if (callback != null) {
                    session.addCallback(callback);
                }
                session.authorizationBundle = bundle.getBundle(AUTH_BUNDLE_SAVE_KEY);
                return session;
            } catch (ClassNotFoundException e) {
                Log.w(TAG, "Unable to restore session", e);
            } catch (IOException e) {
                Log.w(TAG, "Unable to restore session.", e);
            }
        }
        return null;
    }


    /**
     * Returns the current active Session, or null if there is none.
     *
     * @return the current active Session, or null if there is none.
     */
    public static final Session getActiveSession() {
        synchronized (Session.STATIC_LOCK) {
            return Session.activeSession;
        }
    }

    /**
     * <p>
     * Sets the current active Session.
     * </p>
     * <p>
     * The active Session is used implicitly by predefined Request factory
     * methods as well as optionally by UI controls in the sdk.
     * </p>
     * <p>
     * It is legal to set this to null, or to a Session that is not yet open.
     * </p>
     *
     * @param session A Session to use as the active Session, or null to indicate
     *                that there is no active Session.
     */
    public static final void setActiveSession(Session session) {
        synchronized (Session.STATIC_LOCK) {
            if (session != Session.activeSession) {
                Session oldSession = Session.activeSession;

                if (oldSession != null) {
                    oldSession.close();
                }

                Session.activeSession = session;

                if (oldSession != null) {
                    postActiveSessionAction(Session.ACTION_ACTIVE_SESSION_UNSET);
                }

                if (session != null) {
                    postActiveSessionAction(Session.ACTION_ACTIVE_SESSION_SET);

                    if (session.isOpened()) {
                        postActiveSessionAction(Session.ACTION_ACTIVE_SESSION_OPENED);
                    }
                }
            }
        }
    }

    /**
     * If a cached token is available, creates and opens the session and makes it active without any user interaction,
     * otherwise this does nothing.
     *
     * @param context The Context creating this session
     * @return The new session or null if one could not be created
     */
    public static Session openActiveSessionFromCache(Context context) {
        return openActiveSession(context, false, null);
    }

    /**
     * If allowLoginUI is true, this will create a new Session, make it active, and
     * open it. If the default token cache is not available, then this will request
     * basic permissions. If the default token cache is available and cached tokens
     * are loaded, this will use the cached token and associated permissions.
     * <p/>
     * If allowedLoginUI is false, this will only create the active session and open
     * it if it requires no user interaction (i.e. the token cache is available and
     * there are cached tokens).
     *
     * @param activity     The Activity that is opening the new Session.
     * @param allowLoginUI if false, only sets the active session and opens it if it
     *                     does not require user interaction
     * @param callback     The {@link StatusCallback SessionStatusCallback} to
     *                     notify regarding Session state changes. May be null.
     * @return The new Session or null if one could not be created
     */
    public static Session openActiveSession(Activity activity, boolean allowLoginUI,
            StatusCallback callback) {
        return openActiveSession(activity, allowLoginUI, new OpenRequest(activity).setCallback(callback));
    }

    /**
     * If allowLoginUI is true, this will create a new Session, make it active, and
     * open it. If the default token cache is not available, then this will request
     * the permissions provided (and basic permissions of no permissions are provided).
     * If the default token cache is available and cached tokens are loaded, this will
     * use the cached token and associated permissions.
     * <p/>
     * If allowedLoginUI is false, this will only create the active session and open
     * it if it requires no user interaction (i.e. the token cache is available and
     * there are cached tokens).
     *
     * @param activity     The Activity that is opening the new Session.
     * @param allowLoginUI if false, only sets the active session and opens it if it
     *                     does not require user interaction
     * @param permissions  The permissions to request for this Session
     * @param callback     The {@link StatusCallback SessionStatusCallback} to
     *                     notify regarding Session state changes. May be null.
     * @return The new Session or null if one could not be created
     */
    public static Session openActiveSession(Activity activity, boolean allowLoginUI,
            List<String> permissions, StatusCallback callback) {
        return openActiveSession(
                activity, 
                allowLoginUI, 
                new OpenRequest(activity).setCallback(callback).setPermissions(permissions));
    }
    
    /**
     * If allowLoginUI is true, this will create a new Session, make it active, and
     * open it. If the default token cache is not available, then this will request
     * basic permissions. If the default token cache is available and cached tokens
     * are loaded, this will use the cached token and associated permissions.
     * <p/>
     * If allowedLoginUI is false, this will only create the active session and open
     * it if it requires no user interaction (i.e. the token cache is available and
     * there are cached tokens).
     *
     * @param context      The Activity or Service creating this Session
     * @param fragment     The Fragment that is opening the new Session.
     * @param allowLoginUI if false, only sets the active session and opens it if it
     *                     does not require user interaction
     * @param callback     The {@link StatusCallback SessionStatusCallback} to
     *                     notify regarding Session state changes.
     * @return The new Session or null if one could not be created
     */
    public static Session openActiveSession(Context context, Fragment fragment,
            boolean allowLoginUI, StatusCallback callback) {
        return openActiveSession(context, allowLoginUI, new OpenRequest(fragment).setCallback(callback));
    }
    
    /**
     * If allowLoginUI is true, this will create a new Session, make it active, and
     * open it. If the default token cache is not available, then this will request
     * the permissions provided (and basic permissions of no permissions are provided).
     * If the default token cache is available and cached tokens are loaded, this will
     * use the cached token and associated permissions.
     * <p/>
     * If allowedLoginUI is false, this will only create the active session and open
     * it if it requires no user interaction (i.e. the token cache is available and
     * there are cached tokens).
     *
     * @param context      The Activity or Service creating this Session
     * @param fragment     The Fragment that is opening the new Session.
     * @param allowLoginUI if false, only sets the active session and opens it if it
     *                     does not require user interaction
     * @param permissions  The permissions to request for this Session
     * @param callback     The {@link StatusCallback SessionStatusCallback} to
     *                     notify regarding Session state changes.
     * @return The new Session or null if one could not be created
     */
    public static Session openActiveSession(Context context, Fragment fragment,
            boolean allowLoginUI, List<String> permissions, StatusCallback callback) {
        return openActiveSession(
                context, 
                allowLoginUI, 
                new OpenRequest(fragment).setCallback(callback).setPermissions(permissions));
    }

    /**
     * Opens a session based on an existing Facebook access token, and also makes this session
     * the currently active session. This method should be used
     * only in instances where an application has previously obtained an access token and wishes
     * to import it into the Session/TokenCachingStrategy-based session-management system. A primary
     * example would be an application which previously did not use the Facebook SDK for Android
     * and implemented its own session-management scheme, but wishes to implement an upgrade path
     * for existing users so they do not need to log in again when upgrading to a version of
     * the app that uses the SDK. In general, this method will be called only once, when the app
     * detects that it has been upgraded -- after that, the usual Session lifecycle methods
     * should be used to manage the session and its associated token.
     * <p/>
     * No validation is done that the token, token source, or permissions are actually valid.
     * It is the caller's responsibility to ensure that these accurately reflect the state of
     * the token that has been passed in, or calls to the Facebook API may fail.
     *
     * @param context     the Context to use for creation the session
     * @param accessToken the access token obtained from Facebook
     * @param callback    a callback that will be called when the session status changes; may be null
     * @return The new Session or null if one could not be created
     */
    public static Session openActiveSessionWithAccessToken(Context context, AccessToken accessToken,
            StatusCallback callback) {
        Session session = new Session(context, null, null, false);

        setActiveSession(session);
        session.open(accessToken, callback);

        return session;
    }

    private static Session openActiveSession(Context context, boolean allowLoginUI, OpenRequest openRequest) {
        Session session = new Builder(context).build();
        if (SessionState.CREATED_TOKEN_LOADED.equals(session.getState()) || allowLoginUI) {
            setActiveSession(session);
            session.openForRead(openRequest);
            return session;
        }
        return null;
    }

    static Context getStaticContext() {
        return staticContext;
    }

    static void initializeStaticContext(Context currentContext) {
        if ((currentContext != null) && (staticContext == null)) {
            Context applicationContext = currentContext.getApplicationContext();
            staticContext = (applicationContext != null) ? applicationContext : currentContext;
        }
    }

    void authorize(AuthorizationRequest request) {
        boolean started = false;

        request.setApplicationId(applicationId);

        autoPublishAsync();

        logAuthorizationStart();

        started = tryLoginActivity(request);

        pendingAuthorizationRequest.loggingExtras.put(AuthorizationClient.EVENT_EXTRAS_TRY_LOGIN_ACTIVITY,
                started ? AppEventsConstants.EVENT_PARAM_VALUE_YES : AppEventsConstants.EVENT_PARAM_VALUE_NO);

        if (!started && request.isLegacy) {
            pendingAuthorizationRequest.loggingExtras.put(AuthorizationClient.EVENT_EXTRAS_TRY_LEGACY,
                    AppEventsConstants.EVENT_PARAM_VALUE_YES);

            tryLegacyAuth(request);
            started = true;
        }

        if (!started) {
            synchronized (this.lock) {
                final SessionState oldState = this.state;

                switch (this.state) {
                    case CLOSED:
                    case CLOSED_LOGIN_FAILED:
                        return;

                    default:
                        this.state = SessionState.CLOSED_LOGIN_FAILED;

                        Exception exception = new FacebookException(
                                "Log in attempt failed: LoginActivity could not be started, and not legacy request");
                        logAuthorizationComplete(AuthorizationClient.Result.Code.ERROR, null, exception);
                        postStateChange(oldState, this.state, exception);
                }
            }
        }
    }

    private void open(OpenRequest openRequest, SessionAuthorizationType authType) {
        validatePermissions(openRequest, authType);
        validateLoginBehavior(openRequest);

        SessionState newState;
        synchronized (this.lock) {
            if (pendingAuthorizationRequest != null) {
                postStateChange(state, state, new UnsupportedOperationException(
                        "Session: an attempt was made to open a session that has a pending request."));
                return;
            }
            final SessionState oldState = this.state;

            switch (this.state) {
                case CREATED:
                    this.state = newState = SessionState.OPENING;
                    if (openRequest == null) {
                        throw new IllegalArgumentException("openRequest cannot be null when opening a new Session");
                    }
                    pendingAuthorizationRequest = openRequest;
                    break;
                case CREATED_TOKEN_LOADED:
                    if (openRequest != null && !Utility.isNullOrEmpty(openRequest.getPermissions())) {
                        if (!Utility.isSubset(openRequest.getPermissions(), getPermissions())) {
                            pendingAuthorizationRequest = openRequest;
                        }
                    }
                    if (pendingAuthorizationRequest == null) {
                        this.state = newState = SessionState.OPENED;
                    } else {
                        this.state = newState = SessionState.OPENING;
                    }
                    break;
                default:
                    throw new UnsupportedOperationException(
                            "Session: an attempt was made to open an already opened session.");
            }
            if (openRequest != null) {
                addCallback(openRequest.getCallback());
            }
            this.postStateChange(oldState, newState, null);
        }

        if (newState == SessionState.OPENING) {
            authorize(openRequest);
        }
    }

    private void requestNewPermissions(NewPermissionsRequest newPermissionsRequest, SessionAuthorizationType authType) {
        validatePermissions(newPermissionsRequest, authType);
        validateLoginBehavior(newPermissionsRequest);

        if (newPermissionsRequest != null) {
            synchronized (this.lock) {
                if (pendingAuthorizationRequest != null) {
                    throw new UnsupportedOperationException(
                            "Session: an attempt was made to request new permissions for a session that has a pending request.");
                }
                if (state.isOpened()) {
                    pendingAuthorizationRequest = newPermissionsRequest;
                } else if (state.isClosed()) {
                    throw new UnsupportedOperationException(
                            "Session: an attempt was made to request new permissions for a session that has been closed.");
                } else {
                    throw new UnsupportedOperationException(
                            "Session: an attempt was made to request new permissions for a session that is not currently open.");
                }
            }

            newPermissionsRequest.setValidateSameFbidAsToken(getAccessToken());
            addCallback(newPermissionsRequest.getCallback());
            authorize(newPermissionsRequest);
        }
    }

    private void validateLoginBehavior(AuthorizationRequest request) {
        if (request != null && !request.isLegacy) {
            Intent intent = new Intent();
            intent.setClass(getStaticContext(), LoginActivity.class);
            if (!resolveIntent(intent)) {
                throw new FacebookException(String.format(
                        "Cannot use SessionLoginBehavior %s when %s is not declared as an activity in AndroidManifest.xml",
                        request.getLoginBehavior(), LoginActivity.class.getName()));
            }
        }
    }

    private void validatePermissions(AuthorizationRequest request, SessionAuthorizationType authType) {
        if (request == null || Utility.isNullOrEmpty(request.getPermissions())) {
            if (SessionAuthorizationType.PUBLISH.equals(authType)) {
                throw new FacebookException("Cannot request publish or manage authorization with no permissions.");
            }
            return; // nothing to check
        }
        for (String permission : request.getPermissions()) {
            if (isPublishPermission(permission)) {
                if (SessionAuthorizationType.READ.equals(authType)) {
                    throw new FacebookException(
                            String.format(
                                    "Cannot pass a publish or manage permission (%s) to a request for read authorization",
                                    permission));
                }
            } else {
                if (SessionAuthorizationType.PUBLISH.equals(authType)) {
                    Log.w(TAG,
                            String.format(
                                    "Should not pass a read permission (%s) to a request for publish or manage authorization",
                                    permission));
                }
            }
        }
    }

    public static boolean isPublishPermission(String permission) {
        return permission != null &&
                (permission.startsWith(PUBLISH_PERMISSION_PREFIX) ||
                        permission.startsWith(MANAGE_PERMISSION_PREFIX) ||
                        OTHER_PUBLISH_PERMISSIONS.contains(permission));

    }

    private void handleAuthorizationResult(int resultCode, AuthorizationClient.Result result) {
        AccessToken newToken = null;
        Exception exception = null;
        if (resultCode == Activity.RESULT_OK) {
            if (result.code == AuthorizationClient.Result.Code.SUCCESS) {
                newToken = result.token;
            } else {
                exception = new FacebookAuthorizationException(result.errorMessage);
            }
        } else if (resultCode == Activity.RESULT_CANCELED) {
            exception = new FacebookOperationCanceledException(result.errorMessage);
        }

        logAuthorizationComplete(result.code, result.loggingExtras, exception);

        authorizationClient = null;
        finishAuthOrReauth(newToken, exception);
    }

    private void logAuthorizationStart() {
        Bundle bundle = AuthorizationClient.newAuthorizationLoggingBundle(pendingAuthorizationRequest.getAuthId());
        bundle.putLong(AuthorizationClient.EVENT_PARAM_TIMESTAMP, System.currentTimeMillis());

        // Log what we already know about the call in start event
        try {
            JSONObject extras = new JSONObject();
            extras.put(AuthorizationClient.EVENT_EXTRAS_LOGIN_BEHAVIOR,
                    pendingAuthorizationRequest.loginBehavior.toString());
            extras.put(AuthorizationClient.EVENT_EXTRAS_REQUEST_CODE, pendingAuthorizationRequest.requestCode);
            extras.put(AuthorizationClient.EVENT_EXTRAS_IS_LEGACY, pendingAuthorizationRequest.isLegacy);
            extras.put(AuthorizationClient.EVENT_EXTRAS_PERMISSIONS,
                    TextUtils.join(",", pendingAuthorizationRequest.permissions));
            extras.put(AuthorizationClient.EVENT_EXTRAS_DEFAULT_AUDIENCE,
                    pendingAuthorizationRequest.defaultAudience.toString());
            bundle.putString(AuthorizationClient.EVENT_PARAM_EXTRAS, extras.toString());
        } catch (JSONException e) {
        }

        AppEventsLogger logger = getAppEventsLogger();
        logger.logSdkEvent(AuthorizationClient.EVENT_NAME_LOGIN_START, null, bundle);
    }

    private void logAuthorizationComplete(AuthorizationClient.Result.Code result, Map<String, String> resultExtras,
            Exception exception) {
        Bundle bundle = null;
        if (pendingAuthorizationRequest == null) {
            // We don't expect this to happen, but if it does, log an event for diagnostic purposes.
            bundle = AuthorizationClient.newAuthorizationLoggingBundle("");
            bundle.putString(AuthorizationClient.EVENT_PARAM_LOGIN_RESULT,
                    AuthorizationClient.Result.Code.ERROR.getLoggingValue());
            bundle.putString(AuthorizationClient.EVENT_PARAM_ERROR_MESSAGE,
                    "Unexpected call to logAuthorizationComplete with null pendingAuthorizationRequest.");
        } else {
            bundle = AuthorizationClient.newAuthorizationLoggingBundle(pendingAuthorizationRequest.getAuthId());
            if (result != null) {
                bundle.putString(AuthorizationClient.EVENT_PARAM_LOGIN_RESULT, result.getLoggingValue());
            }
            if (exception != null && exception.getMessage() != null) {
                bundle.putString(AuthorizationClient.EVENT_PARAM_ERROR_MESSAGE, exception.getMessage());
            }

            // Combine extras from the request and from the result.
            JSONObject jsonObject = null;
            if (pendingAuthorizationRequest.loggingExtras.isEmpty() == false) {
                jsonObject = new JSONObject(pendingAuthorizationRequest.loggingExtras);
            }
            if (resultExtras != null) {
                if (jsonObject == null) {
                    jsonObject = new JSONObject();
                }
                try {
                    for (Map.Entry<String, String> entry : resultExtras.entrySet()) {
                        jsonObject.put(entry.getKey(), entry.getValue());
                    }
                } catch (JSONException e) {
                }
            }
            if (jsonObject != null) {
                bundle.putString(AuthorizationClient.EVENT_PARAM_EXTRAS, jsonObject.toString());
            }
        }
        bundle.putLong(AuthorizationClient.EVENT_PARAM_TIMESTAMP, System.currentTimeMillis());

        AppEventsLogger logger = getAppEventsLogger();
        logger.logSdkEvent(AuthorizationClient.EVENT_NAME_LOGIN_COMPLETE, null, bundle);
    }

    private boolean tryLoginActivity(AuthorizationRequest request) {
        Intent intent = getLoginActivityIntent(request);

        if (!resolveIntent(intent)) {
            return false;
        }

        try {
            request.getStartActivityDelegate().startActivityForResult(intent, request.getRequestCode());
        } catch (ActivityNotFoundException e) {
            return false;
        }

        return true;
    }

    private boolean resolveIntent(Intent intent) {
        ResolveInfo resolveInfo = getStaticContext().getPackageManager().resolveActivity(intent, 0);
        if (resolveInfo == null) {
            return false;
        }
        return true;
    }

    private Intent getLoginActivityIntent(AuthorizationRequest request) {
        Intent intent = new Intent();
        intent.setClass(getStaticContext(), LoginActivity.class);
        intent.setAction(request.getLoginBehavior().toString());

        // Let LoginActivity populate extras appropriately
        AuthorizationClient.AuthorizationRequest authClientRequest = request.getAuthorizationClientRequest();
        Bundle extras = LoginActivity.populateIntentExtras(authClientRequest);
        intent.putExtras(extras);

        return intent;
    }

    private void tryLegacyAuth(final AuthorizationRequest request) {
        authorizationClient = new AuthorizationClient();
        authorizationClient.setOnCompletedListener(new AuthorizationClient.OnCompletedListener() {
            @Override
            public void onCompleted(AuthorizationClient.Result result) {
                int activityResult;
                if (result.code == AuthorizationClient.Result.Code.CANCEL) {
                    activityResult = Activity.RESULT_CANCELED;
                } else {
                    activityResult = Activity.RESULT_OK;
                }
                handleAuthorizationResult(activityResult, result);
            }
        });
        authorizationClient.setContext(getStaticContext());
        authorizationClient.startOrContinueAuth(request.getAuthorizationClientRequest());
    }

    void finishAuthOrReauth(AccessToken newToken, Exception exception) {
        // If the token we came up with is expired/invalid, then auth failed.
        if ((newToken != null) && newToken.isInvalid()) {
            newToken = null;
            exception = new FacebookException("Invalid access token.");
        }


        synchronized (this.lock) {
            switch (this.state) {
                case OPENING:
                    // This means we are authorizing for the first time in this Session.
                    finishAuthorization(newToken, exception);
                    break;

                case OPENED:
                case OPENED_TOKEN_UPDATED:
                    // This means we are reauthorizing.
                    finishReauthorization(newToken, exception);
                    break;

                case CREATED:
                case CREATED_TOKEN_LOADED:
                case CLOSED:
                case CLOSED_LOGIN_FAILED:
                    Log.d(TAG, "Unexpected call to finishAuthOrReauth in state " + this.state);
                    break;
            }
        }
    }

    private void finishAuthorization(AccessToken newToken, Exception exception) {
        final SessionState oldState = state;
        if (newToken != null) {
            tokenInfo = newToken;
            saveTokenToCache(newToken);

            state = SessionState.OPENED;
        } else if (exception != null) {
            state = SessionState.CLOSED_LOGIN_FAILED;
        }
        pendingAuthorizationRequest = null;
        postStateChange(oldState, state, exception);
    }

    private void finishReauthorization(final AccessToken newToken, Exception exception) {
        final SessionState oldState = state;

        if (newToken != null) {
            tokenInfo = newToken;
            saveTokenToCache(newToken);

            state = SessionState.OPENED_TOKEN_UPDATED;
        }

        pendingAuthorizationRequest = null;
        postStateChange(oldState, state, exception);
    }

    private void saveTokenToCache(AccessToken newToken) {
        if (newToken != null && tokenCachingStrategy != null) {
            tokenCachingStrategy.save(newToken.toCacheBundle());
        }
    }

    void postStateChange(final SessionState oldState, final SessionState newState, final Exception exception) {
        // When we request new permissions, we stay in SessionState.OPENED_TOKEN_UPDATED,
        // but we still want notifications of the state change since permissions are
        // different now.
        if ((oldState == newState) &&
                (oldState != SessionState.OPENED_TOKEN_UPDATED) &&
                (exception == null)) {
            return;
        }

        if (newState.isClosed()) {
            this.tokenInfo = AccessToken.createEmptyToken();
        }

        // Need to schedule the callbacks inside the same queue to preserve ordering.
        // Otherwise these callbacks could have been added to the queue before the SessionTracker
        // gets the ACTIVE_SESSION_SET action.
        Runnable runCallbacks = new Runnable() {
            public void run() {
                synchronized (callbacks) {
                    for (final StatusCallback callback : callbacks) {
                        Runnable closure = new Runnable() {
                            public void run() {
                                // This can be called inside a synchronized block.
                                callback.call(Session.this, newState, exception);
                            }
                        };

                        runWithHandlerOrExecutor(handler, closure);
                    }
                }
            }
        };
        runWithHandlerOrExecutor(handler, runCallbacks);

        if (this == Session.activeSession) {
            if (oldState.isOpened() != newState.isOpened()) {
                if (newState.isOpened()) {
                    postActiveSessionAction(Session.ACTION_ACTIVE_SESSION_OPENED);
                } else {
                    postActiveSessionAction(Session.ACTION_ACTIVE_SESSION_CLOSED);
                }
            }
        }
    }

    static void postActiveSessionAction(String action) {
        final Intent intent = new Intent(action);

        LocalBroadcastManager.getInstance(getStaticContext()).sendBroadcast(intent);
    }

    private static void runWithHandlerOrExecutor(Handler handler, Runnable runnable) {
        if (handler != null) {
            handler.post(runnable);
        } else {
            Settings.getExecutor().execute(runnable);
        }
    }

    void extendAccessTokenIfNeeded() {
        if (shouldExtendAccessToken()) {
            extendAccessToken();
        }
    }

    void extendAccessToken() {
        TokenRefreshRequest newTokenRefreshRequest = null;
        synchronized (this.lock) {
            if (currentTokenRefreshRequest == null) {
                newTokenRefreshRequest = new TokenRefreshRequest();
                currentTokenRefreshRequest = newTokenRefreshRequest;
            }
        }

        if (newTokenRefreshRequest != null) {
            newTokenRefreshRequest.bind();
        }
    }

    boolean shouldExtendAccessToken() {
        if (currentTokenRefreshRequest != null) {
            return false;
        }

        boolean result = false;

        Date now = new Date();

        if (state.isOpened() && tokenInfo.getSource().canExtendToken()
                && now.getTime() - lastAttemptedTokenExtendDate.getTime() > TOKEN_EXTEND_RETRY_SECONDS * 1000
                && now.getTime() - tokenInfo.getLastRefresh().getTime() > TOKEN_EXTEND_THRESHOLD_SECONDS * 1000) {
            result = true;
        }

        return result;
    }

    private AppEventsLogger getAppEventsLogger() {
        synchronized (lock) {
            if (appEventsLogger == null) {
                appEventsLogger = AppEventsLogger.newLogger(staticContext, applicationId);
            }
            return appEventsLogger;
        }
    }

    AccessToken getTokenInfo() {
        return tokenInfo;
    }

    void setTokenInfo(AccessToken tokenInfo) {
        this.tokenInfo = tokenInfo;
    }

    Date getLastAttemptedTokenExtendDate() {
        return lastAttemptedTokenExtendDate;
    }

    void setLastAttemptedTokenExtendDate(Date lastAttemptedTokenExtendDate) {
        this.lastAttemptedTokenExtendDate = lastAttemptedTokenExtendDate;
    }

    void setCurrentTokenRefreshRequest(TokenRefreshRequest request) {
        this.currentTokenRefreshRequest = request;
    }

    class TokenRefreshRequest implements ServiceConnection {

        final Messenger messageReceiver = new Messenger(
                new TokenRefreshRequestHandler(Session.this, this));

        Messenger messageSender = null;

        public void bind() {
            Intent intent = NativeProtocol.createTokenRefreshIntent(getStaticContext());
            if (intent != null
                    && staticContext.bindService(intent, this, Context.BIND_AUTO_CREATE)) {
                setLastAttemptedTokenExtendDate(new Date());
            } else {
                cleanup();
            }
        }

        @Override
        public void onServiceConnected(ComponentName className, IBinder service) {
            messageSender = new Messenger(service);
            refreshToken();
        }

        @Override
        public void onServiceDisconnected(ComponentName arg) {
            cleanup();

            // We returned an error so there's no point in
            // keeping the binding open.
            staticContext.unbindService(TokenRefreshRequest.this);
        }

        private void cleanup() {
            if (currentTokenRefreshRequest == this) {
                currentTokenRefreshRequest = null;
            }
        }

        private void refreshToken() {
            Bundle requestData = new Bundle();
            requestData.putString(AccessToken.ACCESS_TOKEN_KEY, getTokenInfo().getToken());

            Message request = Message.obtain();
            request.setData(requestData);
            request.replyTo = messageReceiver;

            try {
                messageSender.send(request);
            } catch (RemoteException e) {
                cleanup();
            }
        }

    }

    // Creating a static Handler class to reduce the possibility of a memory leak.
    // Handler objects for the same thread all share a common Looper object, which they post messages
    // to and read from. As messages contain target Handler, as long as there are messages with target
    // handler in the message queue, the handler cannot be garbage collected. If handler is not static,
    // the instance of the containing class also cannot be garbage collected even if it is destroyed.
    static class TokenRefreshRequestHandler extends Handler {

        private WeakReference<Session> sessionWeakReference;
        private WeakReference<TokenRefreshRequest> refreshRequestWeakReference;

        TokenRefreshRequestHandler(Session session, TokenRefreshRequest refreshRequest) {
            super(Looper.getMainLooper());
            sessionWeakReference = new WeakReference<Session>(session);
            refreshRequestWeakReference = new WeakReference<TokenRefreshRequest>(refreshRequest);
        }

        @Override
        public void handleMessage(Message msg) {
            String token = msg.getData().getString(AccessToken.ACCESS_TOKEN_KEY);
            Session session = sessionWeakReference.get();

            if (session != null && token != null) {
                session.extendTokenCompleted(msg.getData());
            }

            TokenRefreshRequest request = refreshRequestWeakReference.get();
            if (request != null) {
                // The refreshToken function should be called rarely,
                // so there is no point in keeping the binding open.
                staticContext.unbindService(request);
                request.cleanup();
            }
        }
    }

    /**
     * Provides asynchronous notification of Session state changes.
     *
     * @see Session#open open
     */
    public interface StatusCallback {
        public void call(Session session, SessionState state, Exception exception);
    }

    @Override
    public int hashCode() {
        return 0;
    }

    @Override
    public boolean equals(Object otherObj) {
        if (!(otherObj instanceof Session)) {
            return false;
        }
        Session other = (Session) otherObj;

        return areEqual(other.applicationId, applicationId) &&
                areEqual(other.authorizationBundle, authorizationBundle) &&
                areEqual(other.state, state) &&
                areEqual(other.getExpirationDate(), getExpirationDate());
    }

    private static boolean areEqual(Object a, Object b) {
        if (a == null) {
            return b == null;
        } else {
            return a.equals(b);
        }
    }

    /**
     * Builder class used to create a Session.
     */
    public static final class Builder {
        private final Context context;
        private String applicationId;
        private TokenCachingStrategy tokenCachingStrategy;

        /**
         * Constructs a new Builder associated with the context.
         *
         * @param context the Activity or Service starting the Session
         */
        public Builder(Context context) {
            this.context = context;
        }

        /**
         * Sets the application id for the Session.
         *
         * @param applicationId the application id
         * @return the Builder instance
         */
        public Builder setApplicationId(final String applicationId) {
            this.applicationId = applicationId;
            return this;
        }

        /**
         * Sets the TokenCachingStrategy for the Session.
         *
         * @param tokenCachingStrategy the token cache to use
         * @return the Builder instance
         */
        public Builder setTokenCachingStrategy(final TokenCachingStrategy tokenCachingStrategy) {
            this.tokenCachingStrategy = tokenCachingStrategy;
            return this;
        }

        /**
         * Build the Session.
         *
         * @return a new Session
         */
        public Session build() {
            return new Session(context, applicationId, tokenCachingStrategy);
        }
    }

    interface StartActivityDelegate {
        public void startActivityForResult(Intent intent, int requestCode);

        public Activity getActivityContext();
    }

    @SuppressWarnings("deprecation")
    private void autoPublishAsync() {
        AutoPublishAsyncTask asyncTask = null;
        synchronized (this) {
            if (autoPublishAsyncTask == null && Settings.getShouldAutoPublishInstall()) {
                // copy the application id to guarantee thread safety against our container.
                String applicationId = Session.this.applicationId;

                // skip publish if we don't have an application id.
                if (applicationId != null) {
                    asyncTask = autoPublishAsyncTask = new AutoPublishAsyncTask(applicationId, staticContext);
                }
            }
        }

        if (asyncTask != null) {
            asyncTask.execute();
        }
    }

    /**
     * Async implementation to allow auto publishing to not block the ui thread.
     */
    private class AutoPublishAsyncTask extends AsyncTask<Void, Void, Void> {
        private final String mApplicationId;
        private final Context mApplicationContext;

        public AutoPublishAsyncTask(String applicationId, Context context) {
            mApplicationId = applicationId;
            mApplicationContext = context.getApplicationContext();
        }

        @Override
        protected Void doInBackground(Void... voids) {
            try {
                Settings.publishInstallAndWaitForResponse(mApplicationContext, mApplicationId, true);
            } catch (Exception e) {
                Utility.logd("Facebook-publish", e);
            }
            return null;
        }

        @Override
        protected void onPostExecute(Void result) {
            // always clear out the publisher to allow other invocations.
            synchronized (Session.this) {
                autoPublishAsyncTask = null;
            }
        }
    }

    /**
     * Base class for authorization requests {@link OpenRequest} and {@link NewPermissionsRequest}.
     */
    public static class AuthorizationRequest implements Serializable {

        private static final long serialVersionUID = 1L;

        private final StartActivityDelegate startActivityDelegate;
        private SessionLoginBehavior loginBehavior = SessionLoginBehavior.SSO_WITH_FALLBACK;
        private int requestCode = DEFAULT_AUTHORIZE_ACTIVITY_CODE;
        private StatusCallback statusCallback;
        private boolean isLegacy = false;
        private List<String> permissions = Collections.emptyList();
        private SessionDefaultAudience defaultAudience = SessionDefaultAudience.FRIENDS;
        private String applicationId;
        private String validateSameFbidAsToken;
        private final String authId = UUID.randomUUID().toString();
        private final Map<String, String> loggingExtras = new HashMap<String, String>();

        AuthorizationRequest(final Activity activity) {
            startActivityDelegate = new StartActivityDelegate() {
                @Override
                public void startActivityForResult(Intent intent, int requestCode) {
                    activity.startActivityForResult(intent, requestCode);
                }

                @Override
                public Activity getActivityContext() {
                    return activity;
                }
            };
        }

        AuthorizationRequest(final Fragment fragment) {
            startActivityDelegate = new StartActivityDelegate() {
                @Override
                public void startActivityForResult(Intent intent, int requestCode) {
                    fragment.startActivityForResult(intent, requestCode);
                }

                @Override
                public Activity getActivityContext() {
                    return fragment.getActivity();
                }
            };
        }

        /**
         * Constructor to be used for V1 serialization only, DO NOT CHANGE.
         */
        private AuthorizationRequest(SessionLoginBehavior loginBehavior, int requestCode,
                List<String> permissions, String defaultAudience, boolean isLegacy, String applicationId,
                String validateSameFbidAsToken) {
            startActivityDelegate = new StartActivityDelegate() {
                @Override
                public void startActivityForResult(Intent intent, int requestCode) {
                    throw new UnsupportedOperationException(
                            "Cannot create an AuthorizationRequest without a valid Activity or Fragment");
                }

                @Override
                public Activity getActivityContext() {
                    throw new UnsupportedOperationException(
                            "Cannot create an AuthorizationRequest without a valid Activity or Fragment");
                }
            };
            this.loginBehavior = loginBehavior;
            this.requestCode = requestCode;
            this.permissions = permissions;
            this.defaultAudience = SessionDefaultAudience.valueOf(defaultAudience);
            this.isLegacy = isLegacy;
            this.applicationId = applicationId;
            this.validateSameFbidAsToken = validateSameFbidAsToken;
        }

        /**
         * Used for backwards compatibility with Facebook.java only, DO NOT USE.
         *
         * @param isLegacy
         */
        public void setIsLegacy(boolean isLegacy) {
            this.isLegacy = isLegacy;
        }

        boolean isLegacy() {
            return isLegacy;
        }

        AuthorizationRequest setCallback(StatusCallback statusCallback) {
            this.statusCallback = statusCallback;
            return this;
        }

        StatusCallback getCallback() {
            return statusCallback;
        }

        AuthorizationRequest setLoginBehavior(SessionLoginBehavior loginBehavior) {
            if (loginBehavior != null) {
                this.loginBehavior = loginBehavior;
            }
            return this;
        }

        SessionLoginBehavior getLoginBehavior() {
            return loginBehavior;
        }

        AuthorizationRequest setRequestCode(int requestCode) {
            if (requestCode >= 0) {
                this.requestCode = requestCode;
            }
            return this;
        }

        int getRequestCode() {
            return requestCode;
        }

        AuthorizationRequest setPermissions(List<String> permissions) {
            if (permissions != null) {
                this.permissions = permissions;
            }
            return this;
        }

        AuthorizationRequest setPermissions(String... permissions) {
            return setPermissions(Arrays.asList(permissions));
        }

        List<String> getPermissions() {
            return permissions;
        }

        AuthorizationRequest setDefaultAudience(SessionDefaultAudience defaultAudience) {
            if (defaultAudience != null) {
                this.defaultAudience = defaultAudience;
            }
            return this;
        }

        SessionDefaultAudience getDefaultAudience() {
            return defaultAudience;
        }

        StartActivityDelegate getStartActivityDelegate() {
            return startActivityDelegate;
        }

        String getApplicationId() {
            return applicationId;
        }

        void setApplicationId(String applicationId) {
            this.applicationId = applicationId;
        }

        String getValidateSameFbidAsToken() {
            return validateSameFbidAsToken;
        }

        void setValidateSameFbidAsToken(String validateSameFbidAsToken) {
            this.validateSameFbidAsToken = validateSameFbidAsToken;
        }

        String getAuthId() {
            return authId;
        }

        AuthorizationClient.AuthorizationRequest getAuthorizationClientRequest() {
            AuthorizationClient.StartActivityDelegate delegate = new AuthorizationClient.StartActivityDelegate() {
                @Override
                public void startActivityForResult(Intent intent, int requestCode) {
                    startActivityDelegate.startActivityForResult(intent, requestCode);
                }

                @Override
                public Activity getActivityContext() {
                    return startActivityDelegate.getActivityContext();
                }
            };
            return new AuthorizationClient.AuthorizationRequest(loginBehavior, requestCode, isLegacy,
                    permissions, defaultAudience, applicationId, validateSameFbidAsToken, delegate, authId);
        }

        // package private so subclasses can use it
        Object writeReplace() {
            return new AuthRequestSerializationProxyV1(
                    loginBehavior, requestCode, permissions, defaultAudience.name(), isLegacy, applicationId, validateSameFbidAsToken);
        }

        // have a readObject that throws to prevent spoofing; must be private so serializer will call it (will be
        // called automatically prior to any base class)
        private void readObject(ObjectInputStream stream) throws InvalidObjectException {
            throw new InvalidObjectException("Cannot readObject, serialization proxy required");
        }

        private static class AuthRequestSerializationProxyV1 implements Serializable {
            private static final long serialVersionUID = -8748347685113614927L;
            private final SessionLoginBehavior loginBehavior;
            private final int requestCode;
            private boolean isLegacy;
            private final List<String> permissions;
            private final String defaultAudience;
            private final String applicationId;
            private final String validateSameFbidAsToken;

            private AuthRequestSerializationProxyV1(SessionLoginBehavior loginBehavior,
                    int requestCode, List<String> permissions, String defaultAudience, boolean isLegacy,
                    String applicationId, String validateSameFbidAsToken) {
                this.loginBehavior = loginBehavior;
                this.requestCode = requestCode;
                this.permissions = permissions;
                this.defaultAudience = defaultAudience;
                this.isLegacy = isLegacy;
                this.applicationId = applicationId;
                this.validateSameFbidAsToken = validateSameFbidAsToken;
            }

            private Object readResolve() {
                return new AuthorizationRequest(loginBehavior, requestCode, permissions, defaultAudience, isLegacy,
                        applicationId, validateSameFbidAsToken);
            }
        }
    }

    /**
     * A request used to open a Session.
     */
    public static final class OpenRequest extends AuthorizationRequest {
        private static final long serialVersionUID = 1L;

        /**
         * Constructs an OpenRequest.
         *
         * @param activity the Activity to use to open the Session
         */
        public OpenRequest(Activity activity) {
            super(activity);
        }

        /**
         * Constructs an OpenRequest.
         *
         * @param fragment the Fragment to use to open the Session
         */
        public OpenRequest(Fragment fragment) {
            super(fragment);
        }

        /**
         * Sets the StatusCallback for the OpenRequest.
         *
         * @param statusCallback The {@link StatusCallback SessionStatusCallback} to
         *                       notify regarding Session state changes.
         * @return the OpenRequest object to allow for chaining
         */
        public final OpenRequest setCallback(StatusCallback statusCallback) {
            super.setCallback(statusCallback);
            return this;
        }

        /**
         * Sets the login behavior for the OpenRequest.
         *
         * @param loginBehavior The {@link SessionLoginBehavior SessionLoginBehavior} that
         *                      specifies what behaviors should be attempted during
         *                      authorization.
         * @return the OpenRequest object to allow for chaining
         */
        public final OpenRequest setLoginBehavior(SessionLoginBehavior loginBehavior) {
            super.setLoginBehavior(loginBehavior);
            return this;
        }

        /**
         * Sets the request code for the OpenRequest.
         *
         * @param requestCode An integer that identifies this request. This integer will be used
         *                    as the request code in {@link Activity#onActivityResult
         *                    onActivityResult}. This integer should be >= 0. If a value < 0 is
         *                    passed in, then a default value will be used.
         * @return the OpenRequest object to allow for chaining
         */
        public final OpenRequest setRequestCode(int requestCode) {
            super.setRequestCode(requestCode);
            return this;
        }

        /**
         * Sets the permissions for the OpenRequest.
         *
         * @param permissions A List&lt;String&gt; representing the permissions to request
         *                    during the authentication flow. A null or empty List
         *                    represents basic permissions.
         * @return the OpenRequest object to allow for chaining
         */
        public final OpenRequest setPermissions(List<String> permissions) {
            super.setPermissions(permissions);
            return this;
        }

        /**
         * Sets the permissions for the OpenRequest.
         *
         * @param permissions the permissions to request during the authentication flow.
         * @return the OpenRequest object to allow for chaining
         */
        public final OpenRequest setPermissions(String... permissions) {
            super.setPermissions(permissions);
            return this;
        }

        /**
         * Sets the defaultAudience for the OpenRequest.
         * <p/>
         * This is only used during Native login using a sufficiently recent facebook app.
         *
         * @param defaultAudience A SessionDefaultAudience representing the default audience setting to request.
         * @return the OpenRequest object to allow for chaining
         */
        public final OpenRequest setDefaultAudience(SessionDefaultAudience defaultAudience) {
            super.setDefaultAudience(defaultAudience);
            return this;
        }
    }

    /**
     * A request to be used to request new permissions for a Session.
     */
    public static final class NewPermissionsRequest extends AuthorizationRequest {
        private static final long serialVersionUID = 1L;

        /**
         * Constructs a NewPermissionsRequest.
         *
         * @param activity    the Activity used to issue the request
         * @param permissions additional permissions to request
         */
        public NewPermissionsRequest(Activity activity, List<String> permissions) {
            super(activity);
            setPermissions(permissions);
        }

        /**
         * Constructs a NewPermissionsRequest.
         *
         * @param fragment    the Fragment used to issue the request
         * @param permissions additional permissions to request
         */
        public NewPermissionsRequest(Fragment fragment, List<String> permissions) {
            super(fragment);
            setPermissions(permissions);
        }

        /**
         * Constructs a NewPermissionsRequest.
         *
         * @param activity    the Activity used to issue the request
         * @param permissions additional permissions to request
         */
        public NewPermissionsRequest(Activity activity, String... permissions) {
            super(activity);
            setPermissions(permissions);
        }

        /**
         * Constructs a NewPermissionsRequest.
         *
         * @param fragment    the Fragment used to issue the request
         * @param permissions additional permissions to request
         */
        public NewPermissionsRequest(Fragment fragment, String... permissions) {
            super(fragment);
            setPermissions(permissions);
        }

        /**
         * Sets the StatusCallback for the NewPermissionsRequest. Note that once the request is made, this callback
         * will be added to the session, and will receive all future state changes on the session.
         *
         * @param statusCallback The {@link StatusCallback SessionStatusCallback} to
         *                       notify regarding Session state changes.
         * @return the NewPermissionsRequest object to allow for chaining
         */
        public final NewPermissionsRequest setCallback(StatusCallback statusCallback) {
            super.setCallback(statusCallback);
            return this;
        }

        /**
         * Sets the login behavior for the NewPermissionsRequest.
         *
         * @param loginBehavior The {@link SessionLoginBehavior SessionLoginBehavior} that
         *                      specifies what behaviors should be attempted during
         *                      authorization.
         * @return the NewPermissionsRequest object to allow for chaining
         */
        public final NewPermissionsRequest setLoginBehavior(SessionLoginBehavior loginBehavior) {
            super.setLoginBehavior(loginBehavior);
            return this;
        }

        /**
         * Sets the request code for the NewPermissionsRequest.
         *
         * @param requestCode An integer that identifies this request. This integer will be used
         *                    as the request code in {@link Activity#onActivityResult
         *                    onActivityResult}. This integer should be >= 0. If a value < 0 is
         *                    passed in, then a default value will be used.
         * @return the NewPermissionsRequest object to allow for chaining
         */
        public final NewPermissionsRequest setRequestCode(int requestCode) {
            super.setRequestCode(requestCode);
            return this;
        }

        /**
         * Sets the defaultAudience for the OpenRequest.
         *
         * @param defaultAudience A SessionDefaultAudience representing the default audience setting to request.
         * @return the NewPermissionsRequest object to allow for chaining
         */
        public final NewPermissionsRequest setDefaultAudience(SessionDefaultAudience defaultAudience) {
            super.setDefaultAudience(defaultAudience);
            return this;
        }

        @Override
        AuthorizationClient.AuthorizationRequest getAuthorizationClientRequest() {
            AuthorizationClient.AuthorizationRequest request = super.getAuthorizationClientRequest();
            request.setRerequest(true);
            return request;
        }
    }
}
