/**
 * Copyright 2010-present Facebook
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

package com.facebook.android;

import android.Manifest;
import android.app.Activity;
import android.content.*;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.pm.ResolveInfo;
import android.content.pm.Signature;
import android.net.Uri;
import android.os.*;
import com.facebook.*;
import com.facebook.Session.StatusCallback;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.lang.ref.WeakReference;
import java.net.MalformedURLException;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

/**
 * THIS CLASS SHOULD BE CONSIDERED DEPRECATED.
 * <p/>
 * All public members of this class are intentionally deprecated.
 * New code should instead use
 * {@link Session} to manage session state,
 * {@link Request} to make API requests, and
 * {@link com.facebook.widget.WebDialog} to make dialog requests.
 * <p/>
 * Adding @Deprecated to this class causes warnings in other deprecated classes
 * that reference this one.  That is the only reason this entire class is not
 * deprecated.
 *
 * @devDocDeprecated
 */
public class Facebook {

    // Strings used in the authorization flow
    @Deprecated
    public static final String REDIRECT_URI = "fbconnect://success";
    @Deprecated
    public static final String CANCEL_URI = "fbconnect://cancel";
    @Deprecated
    public static final String TOKEN = "access_token";
    @Deprecated
    public static final String EXPIRES = "expires_in";
    @Deprecated
    public static final String SINGLE_SIGN_ON_DISABLED = "service_disabled";

    @Deprecated
    public static final Uri ATTRIBUTION_ID_CONTENT_URI =
        Uri.parse("content://com.facebook.katana.provider.AttributionIdProvider");
    @Deprecated
    public static final String ATTRIBUTION_ID_COLUMN_NAME = "aid";

    @Deprecated
    public static final int FORCE_DIALOG_AUTH = -1;

    private static final String LOGIN = "oauth";

    // Used as default activityCode by authorize(). See authorize() below.
    private static final int DEFAULT_AUTH_ACTIVITY_CODE = 32665;

    // Facebook server endpoints: may be modified in a subclass for testing
    @Deprecated
    protected static String DIALOG_BASE_URL = "https://m.facebook.com/dialog/";
    @Deprecated
    protected static String GRAPH_BASE_URL = "https://graph.facebook.com/";
    @Deprecated
    protected static String RESTSERVER_URL = "https://api.facebook.com/restserver.php";

    private final Object lock = new Object();

    private String accessToken = null;
    private long accessExpiresMillisecondsAfterEpoch = 0;
    private long lastAccessUpdateMillisecondsAfterEpoch = 0;
    private String mAppId;

    private Activity pendingAuthorizationActivity;
    private String[] pendingAuthorizationPermissions;
    private Session pendingOpeningSession;

    private volatile Session session; // must synchronize this.sync to write
    private boolean sessionInvalidated; // must synchronize this.sync to access
    private SetterTokenCachingStrategy tokenCache;
    private volatile Session userSetSession;

    // If the last time we extended the access token was more than 24 hours ago
    // we try to refresh the access token again.
    final private static long REFRESH_TOKEN_BARRIER = 24L * 60L * 60L * 1000L;

    /**
     * Constructor for Facebook object.
     * 
     * @param appId
     *            Your Facebook application ID. Found at
     *            www.facebook.com/developers/apps.php.
     */
    @Deprecated
    public Facebook(String appId) {
        if (appId == null) {
            throw new IllegalArgumentException("You must specify your application ID when instantiating "
                    + "a Facebook object. See README for details.");
        }
        mAppId = appId;
    }

    /**
     * Default authorize method. Grants only basic permissions.
     * <p/>
     * See authorize() below for @params.
     * <p/>
     * This method is deprecated.  See {@link Facebook} and {@link Session} for more info.
     */
    @Deprecated
    public void authorize(Activity activity, final DialogListener listener) {
        authorize(activity, new String[]{}, DEFAULT_AUTH_ACTIVITY_CODE, SessionLoginBehavior.SSO_WITH_FALLBACK,
                listener);
    }

    /**
     * Authorize method that grants custom permissions.
     * <p/>
     * See authorize() below for @params.
     * <p/>
     * This method is deprecated.  See {@link Facebook} and {@link Session} for more info.
     */
    @Deprecated
    public void authorize(Activity activity, String[] permissions, final DialogListener listener) {
        authorize(activity, permissions, DEFAULT_AUTH_ACTIVITY_CODE, SessionLoginBehavior.SSO_WITH_FALLBACK, listener);
    }

    /**
     * Full authorize method.
     * <p/>
     * Starts either an Activity or a dialog which prompts the user to log in to
     * Facebook and grant the requested permissions to the given application.
     * <p/>
     * This method will, when possible, use Facebook's single sign-on for
     * Android to obtain an access token. This involves proxying a call through
     * the Facebook for Android stand-alone application, which will handle the
     * authentication flow, and return an OAuth access token for making API
     * calls.
     * <p/>
     * Because this process will not be available for all users, if single
     * sign-on is not possible, this method will automatically fall back to the
     * OAuth 2.0 User-Agent flow. In this flow, the user credentials are handled
     * by Facebook in an embedded WebView, not by the client application. As
     * such, the dialog makes a network request and renders HTML content rather
     * than a native UI. The access token is retrieved from a redirect to a
     * special URL that the WebView handles.
     * <p/>
     * Note that User credentials could be handled natively using the OAuth 2.0
     * Username and Password Flow, but this is not supported by this SDK.
     * <p/>
     * See http://developers.facebook.com/docs/authentication/ and
     * http://wiki.oauth.net/OAuth-2 for more details.
     * <p/>
     * Note that this method is asynchronous and the callback will be invoked in
     * the original calling thread (not in a background thread).
     * <p/>
     * Also note that requests may be made to the API without calling authorize
     * first, in which case only public information is returned.
     * <p/>
     * IMPORTANT: Note that single sign-on authentication will not function
     * correctly if you do not include a call to the authorizeCallback() method
     * in your onActivityResult() function! Please see below for more
     * information. single sign-on may be disabled by passing FORCE_DIALOG_AUTH
     * as the activityCode parameter in your call to authorize().
     * <p/>
     * This method is deprecated.  See {@link Facebook} and {@link Session} for more info.
     *
     * @param activity
     *            The Android activity in which we want to display the
     *            authorization dialog.
     * @param permissions
     *            A list of permissions required for this application: e.g.
     *            "read_stream", "publish_stream", "offline_access", etc. see
     *            http://developers.facebook.com/docs/authentication/permissions
     *            This parameter should not be null -- if you do not require any
     *            permissions, then pass in an empty String array.
     * @param activityCode
     *            Single sign-on requires an activity result to be called back
     *            to the client application -- if you are waiting on other
     *            activities to return data, pass a custom activity code here to
     *            avoid collisions. If you would like to force the use of legacy
     *            dialog-based authorization, pass FORCE_DIALOG_AUTH for this
     *            parameter. Otherwise just omit this parameter and Facebook
     *            will use a suitable default. See
     *            http://developer.android.com/reference/android/
     *            app/Activity.html for more information.
     * @param listener
     *            Callback interface for notifying the calling application when
     *            the authentication dialog has completed, failed, or been
     *            canceled.
     */
    @Deprecated
    public void authorize(Activity activity, String[] permissions, int activityCode, final DialogListener listener) {
        SessionLoginBehavior behavior = (activityCode >= 0) ? SessionLoginBehavior.SSO_WITH_FALLBACK
                : SessionLoginBehavior.SUPPRESS_SSO;

        authorize(activity, permissions, activityCode, behavior, listener);
    }

    /**
     * Full authorize method.
     * 
     * Starts either an Activity or a dialog which prompts the user to log in to
     * Facebook and grant the requested permissions to the given application.
     * 
     * This method will, when possible, use Facebook's single sign-on for
     * Android to obtain an access token. This involves proxying a call through
     * the Facebook for Android stand-alone application, which will handle the
     * authentication flow, and return an OAuth access token for making API
     * calls.
     * 
     * Because this process will not be available for all users, if single
     * sign-on is not possible, this method will automatically fall back to the
     * OAuth 2.0 User-Agent flow. In this flow, the user credentials are handled
     * by Facebook in an embedded WebView, not by the client application. As
     * such, the dialog makes a network request and renders HTML content rather
     * than a native UI. The access token is retrieved from a redirect to a
     * special URL that the WebView handles.
     * 
     * Note that User credentials could be handled natively using the OAuth 2.0
     * Username and Password Flow, but this is not supported by this SDK.
     * 
     * See http://developers.facebook.com/docs/authentication/ and
     * http://wiki.oauth.net/OAuth-2 for more details.
     * 
     * Note that this method is asynchronous and the callback will be invoked in
     * the original calling thread (not in a background thread).
     * 
     * Also note that requests may be made to the API without calling authorize
     * first, in which case only public information is returned.
     * 
     * IMPORTANT: Note that single sign-on authentication will not function
     * correctly if you do not include a call to the authorizeCallback() method
     * in your onActivityResult() function! Please see below for more
     * information. single sign-on may be disabled by passing FORCE_DIALOG_AUTH
     * as the activityCode parameter in your call to authorize().
     * 
     * @param activity
     *            The Android activity in which we want to display the
     *            authorization dialog.
     * @param permissions
     *            A list of permissions required for this application: e.g.
     *            "read_stream", "publish_stream", "offline_access", etc. see
     *            http://developers.facebook.com/docs/authentication/permissions
     *            This parameter should not be null -- if you do not require any
     *            permissions, then pass in an empty String array.
     * @param activityCode
     *            Single sign-on requires an activity result to be called back
     *            to the client application -- if you are waiting on other
     *            activities to return data, pass a custom activity code here to
     *            avoid collisions. If you would like to force the use of legacy
     *            dialog-based authorization, pass FORCE_DIALOG_AUTH for this
     *            parameter. Otherwise just omit this parameter and Facebook
     *            will use a suitable default. See
     *            http://developer.android.com/reference/android/
     *            app/Activity.html for more information.
     * @param behavior
     *            The {@link SessionLoginBehavior SessionLoginBehavior} that
     *            specifies what behaviors should be attempted during
     *            authorization.
     * @param listener
     *            Callback interface for notifying the calling application when
     *            the authentication dialog has completed, failed, or been
     *            canceled.
     */
    private void authorize(Activity activity, String[] permissions, int activityCode,
                          SessionLoginBehavior behavior, final DialogListener listener) {
        checkUserSession("authorize");
        pendingOpeningSession = new Session.Builder(activity).
                setApplicationId(mAppId).
                setTokenCachingStrategy(getTokenCache()).
                build();
        pendingAuthorizationActivity = activity;
        pendingAuthorizationPermissions = (permissions != null) ? permissions : new String[0];

        StatusCallback callback = new StatusCallback() {
            @Override
            public void call(Session callbackSession, SessionState state, Exception exception) {
                // Invoke user-callback.
                onSessionCallback(callbackSession, state, exception, listener);
            }
        };

        Session.OpenRequest openRequest = new Session.OpenRequest(activity).
                setCallback(callback).
                setLoginBehavior(behavior).
                setRequestCode(activityCode).
                setPermissions(Arrays.asList(pendingAuthorizationPermissions));
        openSession(pendingOpeningSession, openRequest, pendingAuthorizationPermissions.length > 0);
    }

    private void openSession(Session session, Session.OpenRequest openRequest, boolean isPublish) {
        openRequest.setIsLegacy(true);
        if (isPublish) {
            session.openForPublish(openRequest);
        } else {
            session.openForRead(openRequest);
        }
    }

    @SuppressWarnings("deprecation")
    private void onSessionCallback(Session callbackSession, SessionState state, Exception exception,
            DialogListener listener) {
        Bundle extras = callbackSession.getAuthorizationBundle();

        if (state == SessionState.OPENED) {
            Session sessionToClose = null;

            synchronized (Facebook.this.lock) {
                if (callbackSession != Facebook.this.session) {
                    sessionToClose = Facebook.this.session;
                    Facebook.this.session = callbackSession;
                    Facebook.this.sessionInvalidated = false;
                }
            }

            if (sessionToClose != null) {
                sessionToClose.close();
            }

            listener.onComplete(extras);
        } else if (exception != null) {
            if (exception instanceof FacebookOperationCanceledException) {
                listener.onCancel();
            } else if ((exception instanceof FacebookAuthorizationException) && (extras != null)
                    && extras.containsKey(Session.WEB_VIEW_ERROR_CODE_KEY)
                    && extras.containsKey(Session.WEB_VIEW_FAILING_URL_KEY)) {
                DialogError error = new DialogError(exception.getMessage(),
                        extras.getInt(Session.WEB_VIEW_ERROR_CODE_KEY),
                        extras.getString(Session.WEB_VIEW_FAILING_URL_KEY));
                listener.onError(error);
            } else {
                FacebookError error = new FacebookError(exception.getMessage());
                listener.onFacebookError(error);
            }
        }
    }

    /**
     * Helper to validate a service intent by resolving and checking the
     * provider's package signature.
     * 
     * @param context
     * @param intent
     * @return true if the service intent resolution happens successfully and
     *         the signatures match.
     */
    private boolean validateServiceIntent(Context context, Intent intent) {
        ResolveInfo resolveInfo = context.getPackageManager().resolveService(intent, 0);
        if (resolveInfo == null) {
            return false;
        }

        return validateAppSignatureForPackage(context, resolveInfo.serviceInfo.packageName);
    }

    /**
     * Query the signature for the application that would be invoked by the
     * given intent and verify that it matches the FB application's signature.
     * 
     * @param context
     * @param packageName
     * @return true if the app's signature matches the expected signature.
     */
    private boolean validateAppSignatureForPackage(Context context, String packageName) {

        PackageInfo packageInfo;
        try {
            packageInfo = context.getPackageManager().getPackageInfo(packageName, PackageManager.GET_SIGNATURES);
        } catch (NameNotFoundException e) {
            return false;
        }

        for (Signature signature : packageInfo.signatures) {
            if (signature.toCharsString().equals(FB_APP_SIGNATURE)) {
                return true;
            }
        }
        return false;
    }

    /**
     * IMPORTANT: If you are using the deprecated authorize() method,
     * this method must be invoked at the top of the calling
     * activity's onActivityResult() function or Facebook authentication will
     * not function properly!
     * <p/>
     * If your calling activity does not currently implement onActivityResult(),
     * you must implement it and include a call to this method if you intend to
     * use the authorize() method in this SDK.
     * <p/>
     * For more information, see
     * http://developer.android.com/reference/android/app/
     * Activity.html#onActivityResult(int, int, android.content.Intent)
     * <p/>
     * This method is deprecated.  See {@link Facebook} and {@link Session} for more info.
     */
    @Deprecated
    public void authorizeCallback(int requestCode, int resultCode, Intent data) {
        checkUserSession("authorizeCallback");
        Session pending = this.pendingOpeningSession;
        if (pending != null) {
            if (pending.onActivityResult(this.pendingAuthorizationActivity, requestCode, resultCode, data)) {
                this.pendingOpeningSession = null;
                this.pendingAuthorizationActivity = null;
                this.pendingAuthorizationPermissions = null;
            }
        }
    }

    /**
     * Refresh OAuth access token method. Binds to Facebook for Android
     * stand-alone application application to refresh the access token. This
     * method tries to connect to the Facebook App which will handle the
     * authentication flow, and return a new OAuth access token. This method
     * will automatically replace the old token with a new one. Note that this
     * method is asynchronous and the callback will be invoked in the original
     * calling thread (not in a background thread).
     * <p/>
     * This method is deprecated.  See {@link Facebook} and {@link Session} for more info.
     *
     * @param context
     *            The Android Context that will be used to bind to the Facebook
     *            RefreshToken Service
     * @param serviceListener
     *            Callback interface for notifying the calling application when
     *            the refresh request has completed or failed (can be null). In
     *            case of a success a new token can be found inside the result
     *            Bundle under Facebook.ACCESS_TOKEN key.
     * @return true if the binding to the RefreshToken Service was created
     */
    @Deprecated
    public boolean extendAccessToken(Context context, ServiceListener serviceListener) {
        checkUserSession("extendAccessToken");
        Intent intent = new Intent();

        intent.setClassName("com.facebook.katana", "com.facebook.katana.platform.TokenRefreshService");

        // Verify that the application whose package name is
        // com.facebook.katana
        // has the expected FB app signature.
        if (!validateServiceIntent(context, intent)) {
            return false;
        }

        return context.bindService(intent, new TokenRefreshServiceConnection(context, serviceListener),
                Context.BIND_AUTO_CREATE);
    }

    /**
     * Calls extendAccessToken if shouldExtendAccessToken returns true.
     * <p/>
     * This method is deprecated.  See {@link Facebook} and {@link Session} for more info.
     *
     * @return the same value as extendAccessToken if the the token requires
     *         refreshing, true otherwise
     */
    @Deprecated
    public boolean extendAccessTokenIfNeeded(Context context, ServiceListener serviceListener) {
        checkUserSession("extendAccessTokenIfNeeded");
        if (shouldExtendAccessToken()) {
            return extendAccessToken(context, serviceListener);
        }
        return true;
    }

    /**
     * Check if the access token requires refreshing.
     * <p/>
     * This method is deprecated.  See {@link Facebook} and {@link Session} for more info.
     *
     * @return true if the last time a new token was obtained was over 24 hours
     *         ago.
     */
    @Deprecated
    public boolean shouldExtendAccessToken() {
        checkUserSession("shouldExtendAccessToken");
        return isSessionValid()
                && (System.currentTimeMillis() - lastAccessUpdateMillisecondsAfterEpoch >= REFRESH_TOKEN_BARRIER);
    }

    /**
     * Handles connection to the token refresh service (this service is a part
     * of Facebook App).
     */
    private class TokenRefreshServiceConnection implements ServiceConnection {

        final Messenger messageReceiver = new Messenger(
                new TokenRefreshConnectionHandler(Facebook.this, this));

        final ServiceListener serviceListener;
        final Context applicationsContext;

        Messenger messageSender = null;

        public TokenRefreshServiceConnection(Context applicationsContext, ServiceListener serviceListener) {
            this.applicationsContext = applicationsContext;
            this.serviceListener = serviceListener;
        }

        @Override
        public void onServiceConnected(ComponentName className, IBinder service) {
            messageSender = new Messenger(service);
            refreshToken();
        }

        @Override
        public void onServiceDisconnected(ComponentName arg) {
            serviceListener.onError(new Error("Service disconnected"));
            // We returned an error so there's no point in
            // keeping the binding open.
            applicationsContext.unbindService(TokenRefreshServiceConnection.this);
        }

        private void refreshToken() {
            Bundle requestData = new Bundle();
            requestData.putString(TOKEN, accessToken);

            Message request = Message.obtain();
            request.setData(requestData);
            request.replyTo = messageReceiver;

            try {
                messageSender.send(request);
            } catch (RemoteException e) {
                serviceListener.onError(new Error("Service connection error"));
            }
        }
    }

    // Creating a static Handler class to reduce the possibility of a memory leak.
    // Handler objects for the same thread all share a common Looper object, which they post messages
    // to and read from. As messages contain target Handler, as long as there are messages with target
    // handler in the message queue, the handler cannot be garbage collected. If handler is not static,
    // the instance of the containing class also cannot be garbage collected even if it is destroyed.
    private static class TokenRefreshConnectionHandler extends Handler {
        WeakReference<Facebook> facebookWeakReference;
        WeakReference<TokenRefreshServiceConnection> connectionWeakReference;

        TokenRefreshConnectionHandler(Facebook facebook, TokenRefreshServiceConnection connection) {
            super();
            facebookWeakReference = new WeakReference<Facebook>(facebook);
            connectionWeakReference = new WeakReference<TokenRefreshServiceConnection>(connection);
        }

        @Override
        @SuppressWarnings("deprecation")
        public void handleMessage(Message msg) {
            Facebook facebook = facebookWeakReference.get();
            TokenRefreshServiceConnection connection = connectionWeakReference.get();
            if (facebook == null || connection == null) {
                return;
            }

            String token = msg.getData().getString(TOKEN);
            // Legacy functions in Facebook class (and ServiceListener implementors) expect expires_in in
            // milliseconds from epoch
            long expiresAtMsecFromEpoch = msg.getData().getLong(EXPIRES) * 1000L;

            if (token != null) {
                facebook.setAccessToken(token);
                facebook.setAccessExpires(expiresAtMsecFromEpoch);

                Session refreshSession = facebook.session;
                if (refreshSession != null) {
                    // Session.internalRefreshToken expects the original bundle with expires_in in seconds from
                    // epoch.
                    LegacyHelper.extendTokenCompleted(refreshSession, msg.getData());
                }

                if (connection.serviceListener != null) {
                    // To avoid confusion we should return the expiration time in
                    // the same format as the getAccessExpires() function - that
                    // is in milliseconds.
                    Bundle resultBundle = (Bundle) msg.getData().clone();
                    resultBundle.putLong(EXPIRES, expiresAtMsecFromEpoch);

                    connection.serviceListener.onComplete(resultBundle);
                }
            } else if (connection.serviceListener != null) { // extract errors only if
                // client wants them
                String error = msg.getData().getString("error");
                if (msg.getData().containsKey("error_code")) {
                    int errorCode = msg.getData().getInt("error_code");
                    connection.serviceListener.onFacebookError(new FacebookError(error, null, errorCode));
                } else {
                    connection.serviceListener.onError(new Error(error != null ? error : "Unknown service error"));
                }
            }

            // The refreshToken function should be called rarely,
            // so there is no point in keeping the binding open.
            connection.applicationsContext.unbindService(connection);
        }
    }

    /**
     * Invalidate the current user session by removing the access token in
     * memory, clearing the browser cookie, and calling auth.expireSession
     * through the API.
     * <p/>
     * Note that this method blocks waiting for a network response, so do not
     * call it in a UI thread.
     * <p/>
     * This method is deprecated.  See {@link Facebook} and {@link Session} for more info.
     *
     * @param context
     *            The Android context in which the logout should be called: it
     *            should be the same context in which the login occurred in
     *            order to clear any stored cookies
     * @throws IOException
     * @throws MalformedURLException
     * @return JSON string representation of the auth.expireSession response
     *         ("true" if successful)
     */
    @Deprecated
    public String logout(Context context) throws MalformedURLException, IOException {
        return logoutImpl(context);
    }

    String logoutImpl(Context context) throws MalformedURLException, IOException  {
        checkUserSession("logout");
        Bundle b = new Bundle();
        b.putString("method", "auth.expireSession");
        String response = request(b);

        long currentTimeMillis = System.currentTimeMillis();
        Session sessionToClose = null;

        synchronized (this.lock) {
            sessionToClose = session;

            session = null;
            accessToken = null;
            accessExpiresMillisecondsAfterEpoch = 0;
            lastAccessUpdateMillisecondsAfterEpoch = currentTimeMillis;
            sessionInvalidated = false;
        }

        if (sessionToClose != null) {
            sessionToClose.closeAndClearTokenInformation();
        }

        return response;
    }

    /**
     * Make a request to Facebook's old (pre-graph) API with the given
     * parameters. One of the parameter keys must be "method" and its value
     * should be a valid REST server API method.
     * <p/>
     * See http://developers.facebook.com/docs/reference/rest/
     * <p/>
     * Note that this method blocks waiting for a network response, so do not
     * call it in a UI thread.
     * <p/>
     * Example: <code>
     *  Bundle parameters = new Bundle();
     *  parameters.putString("method", "auth.expireSession");
     *  String response = request(parameters);
     * </code>
     * <p/>
     * This method is deprecated.  See {@link Facebook} and {@link Request} for more info.
     *
     * @param parameters
     *            Key-value pairs of parameters to the request. Refer to the
     *            documentation: one of the parameters must be "method".
     * @throws IOException
     *             if a network error occurs
     * @throws MalformedURLException
     *             if accessing an invalid endpoint
     * @throws IllegalArgumentException
     *             if one of the parameters is not "method"
     * @return JSON string representation of the response
     */
    @Deprecated
    public String request(Bundle parameters) throws MalformedURLException, IOException {
        if (!parameters.containsKey("method")) {
            throw new IllegalArgumentException("API method must be specified. "
                    + "(parameters must contain key \"method\" and value). See"
                    + " http://developers.facebook.com/docs/reference/rest/");
        }
        return requestImpl(null, parameters, "GET");
    }

    /**
     * Make a request to the Facebook Graph API without any parameters.
     * <p/>
     * See http://developers.facebook.com/docs/api
     * <p/>
     * Note that this method blocks waiting for a network response, so do not
     * call it in a UI thread.
     * <p/>
     * This method is deprecated.  See {@link Facebook} and {@link Request} for more info.
     *
     * @param graphPath
     *            Path to resource in the Facebook graph, e.g., to fetch data
     *            about the currently logged authenticated user, provide "me",
     *            which will fetch http://graph.facebook.com/me
     * @throws IOException
     * @throws MalformedURLException
     * @return JSON string representation of the response
     */
    @Deprecated
    public String request(String graphPath) throws MalformedURLException, IOException {
        return requestImpl(graphPath, new Bundle(), "GET");
    }

    /**
     * Make a request to the Facebook Graph API with the given string parameters
     * using an HTTP GET (default method).
     * <p/>
     * See http://developers.facebook.com/docs/api
     * <p/>
     * Note that this method blocks waiting for a network response, so do not
     * call it in a UI thread.
     * <p/>
     * This method is deprecated.  See {@link Facebook} and {@link Request} for more info.
     *
     * @param graphPath
     *            Path to resource in the Facebook graph, e.g., to fetch data
     *            about the currently logged authenticated user, provide "me",
     *            which will fetch http://graph.facebook.com/me
     * @param parameters
     *            key-value string parameters, e.g. the path "search" with
     *            parameters "q" : "facebook" would produce a query for the
     *            following graph resource:
     *            https://graph.facebook.com/search?q=facebook
     * @throws IOException
     * @throws MalformedURLException
     * @return JSON string representation of the response
     */
    @Deprecated
    public String request(String graphPath, Bundle parameters) throws MalformedURLException, IOException {
        return requestImpl(graphPath, parameters, "GET");
    }

    /**
     * Synchronously make a request to the Facebook Graph API with the given
     * HTTP method and string parameters. Note that binary data parameters (e.g.
     * pictures) are not yet supported by this helper function.
     * <p/>
     * See http://developers.facebook.com/docs/api
     * <p/>
     * Note that this method blocks waiting for a network response, so do not
     * call it in a UI thread.
     * <p/>
     * This method is deprecated.  See {@link Facebook} and {@link Request} for more info.
     *
     * @param graphPath
     *            Path to resource in the Facebook graph, e.g., to fetch data
     *            about the currently logged authenticated user, provide "me",
     *            which will fetch http://graph.facebook.com/me
     * @param params
     *            Key-value string parameters, e.g. the path "search" with
     *            parameters {"q" : "facebook"} would produce a query for the
     *            following graph resource:
     *            https://graph.facebook.com/search?q=facebook
     * @param httpMethod
     *            http verb, e.g. "GET", "POST", "DELETE"
     * @throws IOException
     * @throws MalformedURLException
     * @return JSON string representation of the response
     */
    @Deprecated
    public String request(String graphPath, Bundle params, String httpMethod) throws FileNotFoundException,
            MalformedURLException, IOException {
        return requestImpl(graphPath, params, httpMethod);
    }

    // Internal call to avoid deprecated warnings.
    @SuppressWarnings("deprecation")
    String requestImpl(String graphPath, Bundle params, String httpMethod) throws FileNotFoundException,
            MalformedURLException, IOException {
        params.putString("format", "json");
        if (isSessionValid()) {
            params.putString(TOKEN, getAccessToken());
        }
        String url = (graphPath != null) ? GRAPH_BASE_URL + graphPath : RESTSERVER_URL;
        return Util.openUrl(url, httpMethod, params);
    }

    /**
     * Generate a UI dialog for the request action in the given Android context.
     * <p/>
     * Note that this method is asynchronous and the callback will be invoked in
     * the original calling thread (not in a background thread).
     *
     * This method is deprecated. See {@link com.facebook.widget.WebDialog}.
     *
     * @param context
     *            The Android context in which we will generate this dialog.
     * @param action
     *            String representation of the desired method: e.g. "login",
     *            "stream.publish", ...
     * @param listener
     *            Callback interface to notify the application when the dialog
     *            has completed.
     */
    @Deprecated
    public void dialog(Context context, String action, DialogListener listener) {
        dialog(context, action, new Bundle(), listener);
    }

    /**
     * Generate a UI dialog for the request action in the given Android context
     * with the provided parameters.
     * <p/>
     * Note that this method is asynchronous and the callback will be invoked in
     * the original calling thread (not in a background thread).
     *
     * This method is deprecated. See {@link com.facebook.widget.WebDialog}.
     * 
     * @param context
     *            The Android context in which we will generate this dialog.
     * @param action
     *            String representation of the desired method: e.g. "feed" ...
     * @param parameters
     *            String key-value pairs to be passed as URL parameters.
     * @param listener
     *            Callback interface to notify the application when the dialog
     *            has completed.
     */
    @Deprecated
    public void dialog(Context context, String action, Bundle parameters, final DialogListener listener) {
        parameters.putString("display", "touch");
        parameters.putString("redirect_uri", REDIRECT_URI);

        if (action.equals(LOGIN)) {
            parameters.putString("type", "user_agent");
            parameters.putString("client_id", mAppId);
        } else {
            parameters.putString("app_id", mAppId);
            // We do not want to add an access token when displaying the auth dialog.
            if (isSessionValid()) {
                parameters.putString(TOKEN, getAccessToken());
            }
        }

        if (context.checkCallingOrSelfPermission(Manifest.permission.INTERNET) != PackageManager.PERMISSION_GRANTED) {
            Util.showAlert(context, "Error", "Application requires permission to access the Internet");
        } else {
            new FbDialog(context, action, parameters, listener).show();
        }
    }

    /**
     * Returns whether the current access token is valid
     *
     * @return boolean - whether this object has an non-expired session token
     */
    @Deprecated
    public boolean isSessionValid() {
        return (getAccessToken() != null)
                && ((getAccessExpires() == 0) || (System.currentTimeMillis() < getAccessExpires()));
    }

    /**
     * Allows the user to set a Session for the Facebook class to use.
     * If a Session is set here, then one should not use the authorize, logout,
     * or extendAccessToken methods which alter the Session object since that may
     * result in undefined behavior. Using those methods after setting the
     * session here will result in exceptions being thrown.
     *
     * @param session the Session object to use, cannot be null
     */
    @Deprecated
    public void setSession(Session session) {
        if (session == null) {
            throw new IllegalArgumentException("session cannot be null");
        }
        synchronized (this.lock) {
            this.userSetSession = session;
        }
    }

    private void checkUserSession(String methodName) {
        if (userSetSession != null) {
            throw new UnsupportedOperationException(
                    String.format("Cannot call %s after setSession has been called.", methodName));
        }
    }

    /**
     * Get the underlying Session object to use with 3.0 api.
     * 
     * @return Session - underlying session
     */
    @Deprecated
    public final Session getSession() {
        while (true) {
            String cachedToken = null;
            Session oldSession = null;

            synchronized (this.lock) {
                if (userSetSession != null) {
                    return userSetSession;
                }
                if ((session != null) || !sessionInvalidated) {
                    return session;
                }

                cachedToken = accessToken;
                oldSession = session;
            }

            if (cachedToken == null) {
                return null;
            }

            // At this point we do not have a valid session, but mAccessToken is
            // non-null.
            // So we can try building a session based on that.
            List<String> permissions;
            if (oldSession != null) {
                permissions = oldSession.getPermissions();
            } else if (pendingAuthorizationPermissions != null) {
                permissions = Arrays.asList(pendingAuthorizationPermissions);
            } else {
                permissions = Collections.<String>emptyList();
            }

            Session newSession = new Session.Builder(pendingAuthorizationActivity).
                    setApplicationId(mAppId).
                    setTokenCachingStrategy(getTokenCache()).
                    build();
            if (newSession.getState() != SessionState.CREATED_TOKEN_LOADED) {
                return null;
            }
            Session.OpenRequest openRequest =
                    new Session.OpenRequest(pendingAuthorizationActivity).setPermissions(permissions);
            openSession(newSession, openRequest, !permissions.isEmpty());

            Session invalidatedSession = null;
            Session returnSession = null;

            synchronized (this.lock) {
                if (sessionInvalidated || (session == null)) {
                    invalidatedSession = session;
                    returnSession = session = newSession;
                    sessionInvalidated = false;
                }
            }

            if (invalidatedSession != null) {
                invalidatedSession.close();
            }

            if (returnSession != null) {
                return returnSession;
            }
            // Else token state changed between the synchronized blocks, so
            // retry..
        }
    }

    /**
     * Retrieve the OAuth 2.0 access token for API access: treat with care.
     * Returns null if no session exists.
     *
     * @return String - access token
     */
    @Deprecated
    public String getAccessToken() {
        Session s = getSession();
        if (s != null) {
            return s.getAccessToken();
        } else {
            return null;
        }
    }

    /**
     * Retrieve the current session's expiration time (in milliseconds since
     * Unix epoch), or 0 if the session doesn't expire or doesn't exist.
     *
     * @return long - session expiration time
     */
    @Deprecated
    public long getAccessExpires() {
        Session s = getSession();
        if (s != null) {
            return s.getExpirationDate().getTime();
        } else {
            return accessExpiresMillisecondsAfterEpoch;
        }
    }

    /**
     * Retrieve the last time the token was updated (in milliseconds since
     * the Unix epoch), or 0 if the token has not been set.
     *
     * @return long - timestamp of the last token update.
     */
    @Deprecated
    public long getLastAccessUpdate() {
        return lastAccessUpdateMillisecondsAfterEpoch;
    }

    /**
     * Restore the token, expiration time, and last update time from cached values.
     * These should be values obtained from getAccessToken(), getAccessExpires, and
     * getLastAccessUpdate() respectively.
     * <p/>
     * This method is deprecated.  See {@link Facebook} and {@link Session} for more info.
     *
     * @param accessToken - access token
     * @param accessExpires - access token expiration time
     * @param lastAccessUpdate - timestamp of the last token update
     */
    @Deprecated
    public void setTokenFromCache(String accessToken, long accessExpires, long lastAccessUpdate) {
        checkUserSession("setTokenFromCache");
        synchronized (this.lock) {
            this.accessToken = accessToken;
            accessExpiresMillisecondsAfterEpoch = accessExpires;
            lastAccessUpdateMillisecondsAfterEpoch = lastAccessUpdate;
        }
    }

    /**
     * Set the OAuth 2.0 access token for API access.
     * <p/>
     * This method is deprecated.  See {@link Facebook} and {@link Session} for more info.
     *
     * @param token
     *            - access token
     */
    @Deprecated
    public void setAccessToken(String token) {
        checkUserSession("setAccessToken");
        synchronized (this.lock) {
            accessToken = token;
            lastAccessUpdateMillisecondsAfterEpoch = System.currentTimeMillis();
            sessionInvalidated = true;
        }
    }

    /**
     * Set the current session's expiration time (in milliseconds since Unix
     * epoch), or 0 if the session doesn't expire.
     * <p/>
     * This method is deprecated.  See {@link Facebook} and {@link Session} for more info.
     *
     * @param timestampInMsec
     *            - timestamp in milliseconds
     */
    @Deprecated
    public void setAccessExpires(long timestampInMsec) {
        checkUserSession("setAccessExpires");
        synchronized (this.lock) {
            accessExpiresMillisecondsAfterEpoch = timestampInMsec;
            lastAccessUpdateMillisecondsAfterEpoch = System.currentTimeMillis();
            sessionInvalidated = true;
        }
    }

    /**
     * Set the current session's duration (in seconds since Unix epoch), or "0"
     * if session doesn't expire.
     * <p/>
     * This method is deprecated.  See {@link Facebook} and {@link Session} for more info.
     *
     * @param expiresInSecsFromNow
     *            - duration in seconds (or 0 if the session doesn't expire)
     */
    @Deprecated
    public void setAccessExpiresIn(String expiresInSecsFromNow) {
        checkUserSession("setAccessExpiresIn");
        if (expiresInSecsFromNow != null) {
            long expires = expiresInSecsFromNow.equals("0") ? 0 : System.currentTimeMillis()
                    + Long.parseLong(expiresInSecsFromNow) * 1000L;
            setAccessExpires(expires);
        }
    }

    /**
     * This method is deprecated.  See {@link Facebook} and {@link Session} for more info.
     *
     * @return the String representing application ID
     */
    @Deprecated
    public String getAppId() {
        return mAppId;
    }

    /**
     * This method is deprecated.  See {@link Facebook} and {@link Session} for more info.
     *
     * @param appId the String representing the application ID
     */
    @Deprecated
    public void setAppId(String appId) {
        checkUserSession("setAppId");
        synchronized (this.lock) {
            mAppId = appId;
            sessionInvalidated = true;
        }
    }

    private TokenCachingStrategy getTokenCache() {
        // Intentionally not volatile/synchronized--it is okay if we race to
        // create more than one of these.
        if (tokenCache == null) {
            tokenCache = new SetterTokenCachingStrategy();
        }
        return tokenCache;
    }

    private static String[] stringArray(List<String> list) {
        int size = (list != null) ? list.size() : 0;
        String[] array = new String[size];

        if (list != null) {
            for (int i = 0; i < array.length; i++) {
                array[i] = list.get(i);
            }
        }

        return array;
    }

    private static List<String> stringList(String[] array) {
        if (array != null) {
            return Arrays.asList(array);
        } else {
            return Collections.emptyList();
        }
    }

    private class SetterTokenCachingStrategy extends TokenCachingStrategy {

        @Override
        public Bundle load() {
            Bundle bundle = new Bundle();

            if (accessToken != null) {
                TokenCachingStrategy.putToken(bundle, accessToken);
                TokenCachingStrategy.putExpirationMilliseconds(bundle, accessExpiresMillisecondsAfterEpoch);
                TokenCachingStrategy.putPermissions(bundle, stringList(pendingAuthorizationPermissions));
                TokenCachingStrategy.putSource(bundle, AccessTokenSource.WEB_VIEW);
                TokenCachingStrategy.putLastRefreshMilliseconds(bundle, lastAccessUpdateMillisecondsAfterEpoch);
            }

            return bundle;
        }

        @Override
        public void save(Bundle bundle) {
            accessToken = TokenCachingStrategy.getToken(bundle);
            accessExpiresMillisecondsAfterEpoch = TokenCachingStrategy.getExpirationMilliseconds(bundle);
            pendingAuthorizationPermissions = stringArray(TokenCachingStrategy.getPermissions(bundle));
            lastAccessUpdateMillisecondsAfterEpoch = TokenCachingStrategy.getLastRefreshMilliseconds(bundle);
        }

        @Override
        public void clear() {
            accessToken = null;
        }
    }

    /**
     * Get Attribution ID for app install conversion tracking.
     * <p/>
     * This method is deprecated.  See {@link Facebook} and {@link Settings} for more info.
     *
     * @param contentResolver
     * @return Attribution ID that will be used for conversion tracking. It will be null only if
     *         the user has not installed or logged in to the Facebook app.
     */
    @Deprecated
    public static String getAttributionId(ContentResolver contentResolver) {
        return Settings.getAttributionId(contentResolver);
    }

    /**
     * Get the auto install publish setting.  If true, an install event will be published during authorize(), unless
     * it has occurred previously or the app does not have install attribution enabled on the application's developer
     * config page.
     * <p/>
     * This method is deprecated.  See {@link Facebook} and {@link Settings} for more info.
     *
     * @return a Boolean indicating whether installation of the app should be auto-published.
     */
    @Deprecated
    public boolean getShouldAutoPublishInstall() {
        return Settings.getShouldAutoPublishInstall();
    }

    /**
     * Sets whether auto publishing of installs will occur.
     * <p/>
     * This method is deprecated.  See {@link Facebook} and {@link Settings} for more info.
     *
     * @param value a Boolean indicating whether installation of the app should be auto-published.
     */
    @Deprecated
    public void setShouldAutoPublishInstall(boolean value) {
        Settings.setShouldAutoPublishInstall(value);
    }

    /**
     * Manually publish install attribution to the Facebook graph.  Internally handles tracking repeat calls to prevent
     * multiple installs being published to the graph.
     * <p/>
     * This method is deprecated.  See {@link Facebook} and {@link Settings} for more info.
     *
     * @param context the current Android context
     * @return Always false.  Earlier versions of the API returned true if it was no longer necessary to call.
     * Apps should ignore this value, but for compatibility we will return false to ensure repeat calls (and the
     * underlying code will prevent duplicate network traffic).
     */
    @Deprecated
    public boolean publishInstall(final Context context) {
        Settings.publishInstallAsync(context, mAppId);
        return false;
    }

    /**
     * Callback interface for dialog requests.
     * <p/>
     * THIS CLASS SHOULD BE CONSIDERED DEPRECATED.
     * <p/>
     * All public members of this class are intentionally deprecated.
     * New code should instead use
     * {@link com.facebook.widget.WebDialog}
     * <p/>
     * Adding @Deprecated to this class causes warnings in other deprecated classes
     * that reference this one.  That is the only reason this entire class is not
     * deprecated.
     *
     * @devDocDeprecated
     */
    public static interface DialogListener {

        /**
         * Called when a dialog completes.
         * 
         * Executed by the thread that initiated the dialog.
         * 
         * @param values
         *            Key-value string pairs extracted from the response.
         */
        public void onComplete(Bundle values);

        /**
         * Called when a Facebook responds to a dialog with an error.
         * 
         * Executed by the thread that initiated the dialog.
         * 
         */
        public void onFacebookError(FacebookError e);

        /**
         * Called when a dialog has an error.
         * 
         * Executed by the thread that initiated the dialog.
         * 
         */
        public void onError(DialogError e);

        /**
         * Called when a dialog is canceled by the user.
         * 
         * Executed by the thread that initiated the dialog.
         * 
         */
        public void onCancel();

    }

    /**
     * Callback interface for service requests.
     * <p/>
     * THIS CLASS SHOULD BE CONSIDERED DEPRECATED.
     * <p/>
     * All public members of this class are intentionally deprecated.
     * New code should instead use
     * {@link Session} to manage session state.
     * <p/>
     * Adding @Deprecated to this class causes warnings in other deprecated classes
     * that reference this one.  That is the only reason this entire class is not
     * deprecated.
     *
     * @devDocDeprecated
     */
    public static interface ServiceListener {

        /**
         * Called when a service request completes.
         * 
         * @param values
         *            Key-value string pairs extracted from the response.
         */
        public void onComplete(Bundle values);

        /**
         * Called when a Facebook server responds to the request with an error.
         */
        public void onFacebookError(FacebookError e);

        /**
         * Called when a Facebook Service responds to the request with an error.
         */
        public void onError(Error e);

    }

    @Deprecated
    public static final String FB_APP_SIGNATURE =
        "30820268308201d102044a9c4610300d06092a864886f70d0101040500307a310"
        + "b3009060355040613025553310b30090603550408130243413112301006035504"
        + "07130950616c6f20416c746f31183016060355040a130f46616365626f6f6b204"
        + "d6f62696c653111300f060355040b130846616365626f6f6b311d301b06035504"
        + "03131446616365626f6f6b20436f72706f726174696f6e3020170d30393038333"
        + "13231353231365a180f32303530303932353231353231365a307a310b30090603"
        + "55040613025553310b30090603550408130243413112301006035504071309506"
        + "16c6f20416c746f31183016060355040a130f46616365626f6f6b204d6f62696c"
        + "653111300f060355040b130846616365626f6f6b311d301b06035504031314466"
        + "16365626f6f6b20436f72706f726174696f6e30819f300d06092a864886f70d01"
        + "0101050003818d0030818902818100c207d51df8eb8c97d93ba0c8c1002c928fa"
        + "b00dc1b42fca5e66e99cc3023ed2d214d822bc59e8e35ddcf5f44c7ae8ade50d7"
        + "e0c434f500e6c131f4a2834f987fc46406115de2018ebbb0d5a3c261bd97581cc"
        + "fef76afc7135a6d59e8855ecd7eacc8f8737e794c60a761c536b72b11fac8e603"
        + "f5da1a2d54aa103b8a13c0dbc10203010001300d06092a864886f70d010104050"
        + "0038181005ee9be8bcbb250648d3b741290a82a1c9dc2e76a0af2f2228f1d9f9c"
        + "4007529c446a70175c5a900d5141812866db46be6559e2141616483998211f4a6"
        + "73149fb2232a10d247663b26a9031e15f84bc1c74d141ff98a02d76f85b2c8ab2"
        + "571b6469b232d8e768a7f7ca04f7abe4a775615916c07940656b58717457b42bd"
        + "928a2";

}
