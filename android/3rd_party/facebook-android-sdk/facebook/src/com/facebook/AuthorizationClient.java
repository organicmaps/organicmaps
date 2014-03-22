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

import android.Manifest;
import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.text.TextUtils;
import android.webkit.CookieSyncManager;
import com.facebook.android.R;
import com.facebook.internal.AnalyticsEvents;
import com.facebook.internal.NativeProtocol;
import com.facebook.internal.ServerProtocol;
import com.facebook.internal.Utility;
import com.facebook.model.GraphMultiResult;
import com.facebook.model.GraphObject;
import com.facebook.model.GraphObjectList;
import com.facebook.model.GraphUser;
import com.facebook.widget.WebDialog;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.Serializable;
import java.util.*;

class AuthorizationClient implements Serializable {
    private static final long serialVersionUID = 1L;
    private static final String TAG = "Facebook-AuthorizationClient";
    private static final String WEB_VIEW_AUTH_HANDLER_STORE =
            "com.facebook.AuthorizationClient.WebViewAuthHandler.TOKEN_STORE_KEY";
    private static final String WEB_VIEW_AUTH_HANDLER_TOKEN_KEY = "TOKEN";

    // Constants for logging login-related data. Some of these are only used by Session, but grouped here for
    // maintainability.
    private static final String EVENT_NAME_LOGIN_METHOD_START = "fb_mobile_login_method_start";
    private static final String EVENT_NAME_LOGIN_METHOD_COMPLETE = "fb_mobile_login_method_complete";
    private static final String EVENT_PARAM_METHOD_RESULT_SKIPPED = "skipped";
    static final String EVENT_NAME_LOGIN_START = "fb_mobile_login_start";
    static final String EVENT_NAME_LOGIN_COMPLETE = "fb_mobile_login_complete";
    // Note: to ensure stability of column mappings across the four different event types, we prepend a column
    // index to each name, and we log all columns with all events, even if they are empty.
    static final String EVENT_PARAM_AUTH_LOGGER_ID = "0_auth_logger_id";
    static final String EVENT_PARAM_TIMESTAMP = "1_timestamp_ms";
    static final String EVENT_PARAM_LOGIN_RESULT = "2_result";
    static final String EVENT_PARAM_METHOD = "3_method";
    static final String EVENT_PARAM_ERROR_CODE = "4_error_code";
    static final String EVENT_PARAM_ERROR_MESSAGE = "5_error_message";
    static final String EVENT_PARAM_EXTRAS = "6_extras";
    static final String EVENT_EXTRAS_TRY_LOGIN_ACTIVITY = "try_login_activity";
    static final String EVENT_EXTRAS_TRY_LEGACY = "try_legacy";
    static final String EVENT_EXTRAS_LOGIN_BEHAVIOR = "login_behavior";
    static final String EVENT_EXTRAS_REQUEST_CODE = "request_code";
    static final String EVENT_EXTRAS_IS_LEGACY = "is_legacy";
    static final String EVENT_EXTRAS_PERMISSIONS = "permissions";
    static final String EVENT_EXTRAS_DEFAULT_AUDIENCE = "default_audience";
    static final String EVENT_EXTRAS_MISSING_INTERNET_PERMISSION = "no_internet_permission";
    static final String EVENT_EXTRAS_NOT_TRIED = "not_tried";
    static final String EVENT_EXTRAS_NEW_PERMISSIONS = "new_permissions";
    static final String EVENT_EXTRAS_SERVICE_DISABLED = "service_disabled";
    static final String EVENT_EXTRAS_APP_CALL_ID = "call_id";
    static final String EVENT_EXTRAS_PROTOCOL_VERSION = "protocol_version";
    static final String EVENT_EXTRAS_WRITE_PRIVACY = "write_privacy";

    List<AuthHandler> handlersToTry;
    AuthHandler currentHandler;
    transient Context context;
    transient StartActivityDelegate startActivityDelegate;
    transient OnCompletedListener onCompletedListener;
    transient BackgroundProcessingListener backgroundProcessingListener;
    transient boolean checkedInternetPermission;
    AuthorizationRequest pendingRequest;
    Map<String, String> loggingExtras;
    private transient AppEventsLogger appEventsLogger;

    interface OnCompletedListener {
        void onCompleted(Result result);
    }

    interface BackgroundProcessingListener {
        void onBackgroundProcessingStarted();

        void onBackgroundProcessingStopped();
    }

    interface StartActivityDelegate {
        public void startActivityForResult(Intent intent, int requestCode);

        public Activity getActivityContext();
    }

    void setContext(final Context context) {
        this.context = context;
        // We rely on individual requests to tell us how to start an activity.
        startActivityDelegate = null;
    }

    void setContext(final Activity activity) {
        this.context = activity;

        // If we are used in the context of an activity, we will always use that activity to
        // call startActivityForResult.
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

    void startOrContinueAuth(AuthorizationRequest request) {
        if (getInProgress()) {
            continueAuth();
        } else {
            authorize(request);
        }
    }

    void authorize(AuthorizationRequest request) {
        if (request == null) {
            return;
        }

        if (pendingRequest != null) {
            throw new FacebookException("Attempted to authorize while a request is pending.");
        }

        if (request.needsNewTokenValidation() && !checkInternetPermission()) {
            // We're going to need INTERNET permission later and don't have it, so fail early.
            return;
        }
        pendingRequest = request;
        handlersToTry = getHandlerTypes(request);
        tryNextHandler();
    }

    void continueAuth() {
        if (pendingRequest == null || currentHandler == null) {
            throw new FacebookException("Attempted to continue authorization without a pending request.");
        }

        if (currentHandler.needsRestart()) {
            currentHandler.cancel();
            tryCurrentHandler();
        }
    }

    boolean getInProgress() {
        return pendingRequest != null && currentHandler != null;
    }

    void cancelCurrentHandler() {
        if (currentHandler != null) {
            currentHandler.cancel();
        }
    }

    boolean onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == pendingRequest.getRequestCode()) {
            return currentHandler.onActivityResult(requestCode, resultCode, data);
        }
        return false;
    }

    private List<AuthHandler> getHandlerTypes(AuthorizationRequest request) {
        ArrayList<AuthHandler> handlers = new ArrayList<AuthHandler>();

        final SessionLoginBehavior behavior = request.getLoginBehavior();
        if (behavior.allowsKatanaAuth()) {
            if (!request.isLegacy()) {
                handlers.add(new GetTokenAuthHandler());
                handlers.add(new KatanaLoginDialogAuthHandler());
            }
            handlers.add(new KatanaProxyAuthHandler());
        }

        if (behavior.allowsWebViewAuth()) {
            handlers.add(new WebViewAuthHandler());
        }

        return handlers;
    }

    boolean checkInternetPermission() {
        if (checkedInternetPermission) {
            return true;
        }

        int permissionCheck = checkPermission(Manifest.permission.INTERNET);
        if (permissionCheck != PackageManager.PERMISSION_GRANTED) {
            String errorType = context.getString(R.string.com_facebook_internet_permission_error_title);
            String errorDescription = context.getString(R.string.com_facebook_internet_permission_error_message);
            complete(Result.createErrorResult(pendingRequest, errorType, errorDescription));

            return false;
        }

        checkedInternetPermission = true;
        return true;
    }

    void tryNextHandler() {
        if (currentHandler != null) {
            logAuthorizationMethodComplete(currentHandler.getNameForLogging(), EVENT_PARAM_METHOD_RESULT_SKIPPED,
                    null, null, currentHandler.methodLoggingExtras);
        }

        while (handlersToTry != null && !handlersToTry.isEmpty()) {
            currentHandler = handlersToTry.remove(0);

            boolean started = tryCurrentHandler();

            if (started) {
                return;
            }
        }

        if (pendingRequest != null) {
            // We went through all handlers without successfully attempting an auth.
            completeWithFailure();
        }
    }

    private void completeWithFailure() {
        complete(Result.createErrorResult(pendingRequest, "Login attempt failed.", null));
    }

    private void addLoggingExtra(String key, String value, boolean accumulate) {
        if (loggingExtras == null) {
            loggingExtras = new HashMap<String, String>();
        }
        if (loggingExtras.containsKey(key) && accumulate) {
            value = loggingExtras.get(key) + "," + value;
        }
        loggingExtras.put(key, value);
    }

    boolean tryCurrentHandler() {
        if (currentHandler.needsInternetPermission() && !checkInternetPermission()) {
            addLoggingExtra(EVENT_EXTRAS_MISSING_INTERNET_PERMISSION, AppEventsConstants.EVENT_PARAM_VALUE_YES,
                    false);
            return false;
        }

        boolean tried = currentHandler.tryAuthorize(pendingRequest);
        if (tried) {
            logAuthorizationMethodStart(currentHandler.getNameForLogging());
        } else {
            // We didn't try it, so we don't get any other completion notification -- log that we skipped it.
            addLoggingExtra(EVENT_EXTRAS_NOT_TRIED, currentHandler.getNameForLogging(), true);
        }

        return tried;
    }

    void completeAndValidate(Result outcome) {
        // Do we need to validate a successful result (as in the case of a reauth)?
        if (outcome.token != null && pendingRequest.needsNewTokenValidation()) {
            validateSameFbidAndFinish(outcome);
        } else {
            // We're done, just notify the listener.
            complete(outcome);
        }
    }

    void complete(Result outcome) {
        // This might be null if, for some reason, none of the handlers were successfully tried (in which case
        // we already logged that).
        if (currentHandler != null) {
            logAuthorizationMethodComplete(currentHandler.getNameForLogging(), outcome,
                    currentHandler.methodLoggingExtras);
        }

        if (loggingExtras != null) {
            // Pass this back to the caller for logging at the aggregate level.
            outcome.loggingExtras = loggingExtras;
        }

        handlersToTry = null;
        currentHandler = null;
        pendingRequest = null;
        loggingExtras = null;

        notifyOnCompleteListener(outcome);
    }

    OnCompletedListener getOnCompletedListener() {
        return onCompletedListener;
    }

    void setOnCompletedListener(OnCompletedListener onCompletedListener) {
        this.onCompletedListener = onCompletedListener;
    }

    BackgroundProcessingListener getBackgroundProcessingListener() {
        return backgroundProcessingListener;
    }

    void setBackgroundProcessingListener(BackgroundProcessingListener backgroundProcessingListener) {
        this.backgroundProcessingListener = backgroundProcessingListener;
    }

    StartActivityDelegate getStartActivityDelegate() {
        if (startActivityDelegate != null) {
            return startActivityDelegate;
        } else if (pendingRequest != null) {
            // Wrap the request's delegate in our own.
            return new StartActivityDelegate() {
                @Override
                public void startActivityForResult(Intent intent, int requestCode) {
                    pendingRequest.getStartActivityDelegate().startActivityForResult(intent, requestCode);
                }

                @Override
                public Activity getActivityContext() {
                    return pendingRequest.getStartActivityDelegate().getActivityContext();
                }
            };
        }
        return null;
    }

    int checkPermission(String permission) {
        return context.checkCallingOrSelfPermission(permission);
    }

    void validateSameFbidAndFinish(Result pendingResult) {
        if (pendingResult.token == null) {
            throw new FacebookException("Can't validate without a token");
        }

        RequestBatch batch = createReauthValidationBatch(pendingResult);

        notifyBackgroundProcessingStart();

        batch.executeAsync();
    }

    RequestBatch createReauthValidationBatch(final Result pendingResult) {
        // We need to ensure that the token we got represents the same fbid as the old one. We issue
        // a "me" request using the current token, a "me" request using the new token, and a "me/permissions"
        // request using the current token to get the permissions of the user.

        final ArrayList<String> fbids = new ArrayList<String>();
        final ArrayList<String> tokenPermissions = new ArrayList<String>();
        final String newToken = pendingResult.token.getToken();

        Request.Callback meCallback = new Request.Callback() {
            @Override
            public void onCompleted(Response response) {
                try {
                    GraphUser user = response.getGraphObjectAs(GraphUser.class);
                    if (user != null) {
                        fbids.add(user.getId());
                    }
                } catch (Exception ex) {
                }
            }
        };

        String validateSameFbidAsToken = pendingRequest.getPreviousAccessToken();
        Request requestCurrentTokenMe = createGetProfileIdRequest(validateSameFbidAsToken);
        requestCurrentTokenMe.setCallback(meCallback);

        Request requestNewTokenMe = createGetProfileIdRequest(newToken);
        requestNewTokenMe.setCallback(meCallback);

        Request requestCurrentTokenPermissions = createGetPermissionsRequest(validateSameFbidAsToken);
        requestCurrentTokenPermissions.setCallback(new Request.Callback() {
            @Override
            public void onCompleted(Response response) {
                try {
                    GraphMultiResult result = response.getGraphObjectAs(GraphMultiResult.class);
                    if (result != null) {
                        GraphObjectList<GraphObject> data = result.getData();
                        if (data != null && data.size() == 1) {
                            GraphObject permissions = data.get(0);

                            // The keys are the permission names.
                            tokenPermissions.addAll(permissions.asMap().keySet());
                        }
                    }
                } catch (Exception ex) {
                }
            }
        });

        RequestBatch batch = new RequestBatch(requestCurrentTokenMe, requestNewTokenMe,
                requestCurrentTokenPermissions);
        batch.setBatchApplicationId(pendingRequest.getApplicationId());
        batch.addCallback(new RequestBatch.Callback() {
            @Override
            public void onBatchCompleted(RequestBatch batch) {
                try {
                    Result result = null;
                    if (fbids.size() == 2 && fbids.get(0) != null && fbids.get(1) != null &&
                            fbids.get(0).equals(fbids.get(1))) {
                        // Modify the token to have the right permission set.
                        AccessToken tokenWithPermissions = AccessToken
                                .createFromTokenWithRefreshedPermissions(pendingResult.token,
                                        tokenPermissions);
                        result = Result.createTokenResult(pendingRequest, tokenWithPermissions);
                    } else {
                        result = Result
                                .createErrorResult(pendingRequest, "User logged in as different Facebook user.", null);
                    }
                    complete(result);
                } catch (Exception ex) {
                    complete(Result.createErrorResult(pendingRequest, "Caught exception", ex.getMessage()));
                } finally {
                    notifyBackgroundProcessingStop();
                }
            }
        });

        return batch;
    }

    Request createGetPermissionsRequest(String accessToken) {
        Bundle parameters = new Bundle();
        parameters.putString("fields", "id");
        parameters.putString("access_token", accessToken);
        return new Request(null, "me/permissions", parameters, HttpMethod.GET, null);
    }

    Request createGetProfileIdRequest(String accessToken) {
        Bundle parameters = new Bundle();
        parameters.putString("fields", "id");
        parameters.putString("access_token", accessToken);
        return new Request(null, "me", parameters, HttpMethod.GET, null);
    }

    private AppEventsLogger getAppEventsLogger() {
        if (appEventsLogger == null || appEventsLogger.getApplicationId() != pendingRequest.getApplicationId()) {
            appEventsLogger = AppEventsLogger.newLogger(context, pendingRequest.getApplicationId());
        }
        return appEventsLogger;
    }

    private void notifyOnCompleteListener(Result outcome) {
        if (onCompletedListener != null) {
            onCompletedListener.onCompleted(outcome);
        }
    }

    private void notifyBackgroundProcessingStart() {
        if (backgroundProcessingListener != null) {
            backgroundProcessingListener.onBackgroundProcessingStarted();
        }
    }

    private void notifyBackgroundProcessingStop() {
        if (backgroundProcessingListener != null) {
            backgroundProcessingListener.onBackgroundProcessingStopped();
        }
    }

    private void logAuthorizationMethodStart(String method) {
        Bundle bundle = newAuthorizationLoggingBundle(pendingRequest.getAuthId());
        bundle.putLong(EVENT_PARAM_TIMESTAMP, System.currentTimeMillis());
        bundle.putString(EVENT_PARAM_METHOD, method);

        getAppEventsLogger().logSdkEvent(EVENT_NAME_LOGIN_METHOD_START, null, bundle);
    }

    private void logAuthorizationMethodComplete(String method, Result result, Map<String, String> loggingExtras) {
        logAuthorizationMethodComplete(method, result.code.getLoggingValue(), result.errorMessage, result.errorCode,
                loggingExtras);
    }

    private void logAuthorizationMethodComplete(String method, String result, String errorMessage, String errorCode,
            Map<String, String> loggingExtras) {
        Bundle bundle = null;
        if (pendingRequest == null) {
            // We don't expect this to happen, but if it does, log an event for diagnostic purposes.
            bundle = newAuthorizationLoggingBundle("");
            bundle.putString(EVENT_PARAM_LOGIN_RESULT, Result.Code.ERROR.getLoggingValue());
            bundle.putString(EVENT_PARAM_ERROR_MESSAGE,
                    "Unexpected call to logAuthorizationMethodComplete with null pendingRequest.");
        } else {
            bundle = newAuthorizationLoggingBundle(pendingRequest.getAuthId());
            if (result != null) {
                bundle.putString(EVENT_PARAM_LOGIN_RESULT, result);
            }
            if (errorMessage != null) {
                bundle.putString(EVENT_PARAM_ERROR_MESSAGE, errorMessage);
            }
            if (errorCode != null) {
                bundle.putString(EVENT_PARAM_ERROR_CODE, errorCode);
            }
            if (loggingExtras != null && !loggingExtras.isEmpty()) {
                JSONObject jsonObject = new JSONObject(loggingExtras);
                bundle.putString(EVENT_PARAM_EXTRAS, jsonObject.toString());
            }
        }
        bundle.putString(EVENT_PARAM_METHOD, method);
        bundle.putLong(EVENT_PARAM_TIMESTAMP, System.currentTimeMillis());

        getAppEventsLogger().logSdkEvent(EVENT_NAME_LOGIN_METHOD_COMPLETE, null, bundle);
    }

    static Bundle newAuthorizationLoggingBundle(String authLoggerId) {
        // We want to log all parameters for all events, to ensure stability of columns across different event types.
        Bundle bundle = new Bundle();
        bundle.putLong(EVENT_PARAM_TIMESTAMP, System.currentTimeMillis());
        bundle.putString(EVENT_PARAM_AUTH_LOGGER_ID, authLoggerId);
        bundle.putString(EVENT_PARAM_METHOD, "");
        bundle.putString(EVENT_PARAM_LOGIN_RESULT, "");
        bundle.putString(EVENT_PARAM_ERROR_MESSAGE, "");
        bundle.putString(EVENT_PARAM_ERROR_CODE, "");
        bundle.putString(EVENT_PARAM_EXTRAS, "");
        return bundle;
    }

    abstract class AuthHandler implements Serializable {
        private static final long serialVersionUID = 1L;

        Map<String, String> methodLoggingExtras;

        abstract boolean tryAuthorize(AuthorizationRequest request);
        abstract String getNameForLogging();

        boolean onActivityResult(int requestCode, int resultCode, Intent data) {
            return false;
        }

        boolean needsRestart() {
            return false;
        }

        boolean needsInternetPermission() {
            return false;
        }

        void cancel() {
        }

        protected void addLoggingExtra(String key, Object value) {
            if (methodLoggingExtras == null) {
                methodLoggingExtras = new HashMap<String, String>();
            }
            methodLoggingExtras.put(key, value == null ? null : value.toString());
        }
    }

    class WebViewAuthHandler extends AuthHandler {
        private static final long serialVersionUID = 1L;
        private transient WebDialog loginDialog;
        private String applicationId;
        private String e2e;

        @Override
        String getNameForLogging() {
            return "web_view";
        }

        @Override
        boolean needsRestart() {
            // Because we are presenting WebView UI within the current context, we need to explicitly
            // restart the process if the context goes away and is recreated.
            return true;
        }

        @Override
        boolean needsInternetPermission() {
            return true;
        }

        @Override
        void cancel() {
            if (loginDialog != null) {
                loginDialog.dismiss();
                loginDialog = null;
            }
        }

        @Override
        boolean tryAuthorize(final AuthorizationRequest request) {
            applicationId = request.getApplicationId();
            Bundle parameters = new Bundle();
            if (!Utility.isNullOrEmpty(request.getPermissions())) {
                String scope = TextUtils.join(",", request.getPermissions());
                parameters.putString(ServerProtocol.DIALOG_PARAM_SCOPE, scope);
                addLoggingExtra(ServerProtocol.DIALOG_PARAM_SCOPE, scope);
            }

            String previousToken = request.getPreviousAccessToken();
            if (!Utility.isNullOrEmpty(previousToken) && (previousToken.equals(loadCookieToken()))) {
                parameters.putString(ServerProtocol.DIALOG_PARAM_ACCESS_TOKEN, previousToken);
                // Don't log the actual access token, just its presence or absence.
                addLoggingExtra(ServerProtocol.DIALOG_PARAM_ACCESS_TOKEN, AppEventsConstants.EVENT_PARAM_VALUE_YES);
            } else {
                // The call to clear cookies will create the first instance of CookieSyncManager if necessary
                Utility.clearFacebookCookies(context);
                addLoggingExtra(ServerProtocol.DIALOG_PARAM_ACCESS_TOKEN, AppEventsConstants.EVENT_PARAM_VALUE_NO);
            }

            WebDialog.OnCompleteListener listener = new WebDialog.OnCompleteListener() {
                @Override
                public void onComplete(Bundle values, FacebookException error) {
                    onWebDialogComplete(request, values, error);
                }
            };

            e2e = getE2E();
            addLoggingExtra(ServerProtocol.DIALOG_PARAM_E2E, e2e);

            WebDialog.Builder builder =
                    new AuthDialogBuilder(getStartActivityDelegate().getActivityContext(), applicationId, parameters)
                            .setE2E(e2e)
                            .setOnCompleteListener(listener);
            loginDialog = builder.build();
            loginDialog.show();

            return true;
        }

        void onWebDialogComplete(AuthorizationRequest request, Bundle values,
                FacebookException error) {
            Result outcome;
            if (values != null) {
                // Actual e2e we got from the dialog should be used for logging.
                if (values.containsKey(ServerProtocol.DIALOG_PARAM_E2E)) {
                    e2e = values.getString(ServerProtocol.DIALOG_PARAM_E2E);
                }

                AccessToken token = AccessToken
                        .createFromWebBundle(request.getPermissions(), values, AccessTokenSource.WEB_VIEW);
                outcome = Result.createTokenResult(pendingRequest, token);

                // Ensure any cookies set by the dialog are saved
                // This is to work around a bug where CookieManager may fail to instantiate if CookieSyncManager
                // has never been created.
                CookieSyncManager syncManager = CookieSyncManager.createInstance(context);
                syncManager.sync();
                saveCookieToken(token.getToken());
            } else {
                if (error instanceof FacebookOperationCanceledException) {
                    outcome = Result.createCancelResult(pendingRequest, "User canceled log in.");
                } else {
                    // Something went wrong, don't log a completion event since it will skew timing results.
                    e2e = null;

                    String errorCode = null;
                    String errorMessage = error.getMessage();
                    if (error instanceof FacebookServiceException) {
                        FacebookRequestError requestError = ((FacebookServiceException)error).getRequestError();
                        errorCode = String.format("%d", requestError.getErrorCode());
                        errorMessage = requestError.toString();
                    }
                    outcome = Result.createErrorResult(pendingRequest, null, errorMessage, errorCode);
                }
            }

            if (!Utility.isNullOrEmpty(e2e)) {
                logWebLoginCompleted(applicationId, e2e);
            }

            completeAndValidate(outcome);
        }

        private void saveCookieToken(String token) {
            Context context = getStartActivityDelegate().getActivityContext();
            SharedPreferences sharedPreferences = context.getSharedPreferences(
                    WEB_VIEW_AUTH_HANDLER_STORE,
                    Context.MODE_PRIVATE);
            SharedPreferences.Editor editor = sharedPreferences.edit();
            editor.putString(WEB_VIEW_AUTH_HANDLER_TOKEN_KEY, token);
            if (!editor.commit()) {
                Utility.logd(TAG, "Could not update saved web view auth handler token.");
            }
        }

        private String loadCookieToken() {
            Context context = getStartActivityDelegate().getActivityContext();
            SharedPreferences sharedPreferences = context.getSharedPreferences(
                    WEB_VIEW_AUTH_HANDLER_STORE,
                    Context.MODE_PRIVATE);
            return sharedPreferences.getString(WEB_VIEW_AUTH_HANDLER_TOKEN_KEY, "");
        }
    }

    class GetTokenAuthHandler extends AuthHandler {
        private static final long serialVersionUID = 1L;
        private transient GetTokenClient getTokenClient;

        @Override
        String getNameForLogging() {
            return "get_token";
        }

        @Override
        void cancel() {
            if (getTokenClient != null) {
                getTokenClient.cancel();
                getTokenClient = null;
            }
        }

        @Override
        boolean needsRestart() {
            // if the getTokenClient is null, that means an orientation change has occurred, and we need
            // to recreate the GetTokenClient, so return true to indicate we need a restart
            return getTokenClient == null;
        }

        boolean tryAuthorize(final AuthorizationRequest request) {
            getTokenClient = new GetTokenClient(context, request.getApplicationId());
            if (!getTokenClient.start()) {
                return false;
            }

            notifyBackgroundProcessingStart();

            GetTokenClient.CompletedListener callback = new GetTokenClient.CompletedListener() {
                @Override
                public void completed(Bundle result) {
                    getTokenCompleted(request, result);
                }
            };

            getTokenClient.setCompletedListener(callback);
            return true;
        }

        void getTokenCompleted(AuthorizationRequest request, Bundle result) {
            getTokenClient = null;

            notifyBackgroundProcessingStop();

            if (result != null) {
                ArrayList<String> currentPermissions = result.getStringArrayList(NativeProtocol.EXTRA_PERMISSIONS);
                List<String> permissions = request.getPermissions();
                if ((currentPermissions != null) &&
                        ((permissions == null) || currentPermissions.containsAll(permissions))) {
                    // We got all the permissions we needed, so we can complete the auth now.
                    AccessToken token = AccessToken
                            .createFromNativeLogin(result, AccessTokenSource.FACEBOOK_APPLICATION_SERVICE);
                    Result outcome = Result.createTokenResult(pendingRequest, token);
                    completeAndValidate(outcome);
                    return;
                }

                // We didn't get all the permissions we wanted, so update the request with just the permissions
                // we still need.
                List<String> newPermissions = new ArrayList<String>();
                for (String permission : permissions) {
                    if (!currentPermissions.contains(permission)) {
                        newPermissions.add(permission);
                    }
                }
                if (!newPermissions.isEmpty()) {
                    addLoggingExtra(EVENT_EXTRAS_NEW_PERMISSIONS, TextUtils.join(",", newPermissions));
                }

                request.setPermissions(newPermissions);
            }

            tryNextHandler();
        }
    }

    abstract class KatanaAuthHandler extends AuthHandler {
        private static final long serialVersionUID = 1L;

        protected boolean tryIntent(Intent intent, int requestCode) {
            if (intent == null) {
                return false;
            }

            try {
                getStartActivityDelegate().startActivityForResult(intent, requestCode);
            } catch (ActivityNotFoundException e) {
                // We don't expect this to happen, since we've already validated the intent and bailed out before
                // now if it couldn't be resolved.
                return false;
            }

            return true;
        }
    }

    class KatanaLoginDialogAuthHandler extends KatanaAuthHandler {
        private static final long serialVersionUID = 1L;
        private String applicationId;
        private String callId;

        @Override
        String getNameForLogging() {
            return "katana_login_dialog";
        }

        @Override
        boolean tryAuthorize(AuthorizationRequest request) {
            applicationId = request.getApplicationId();

            Intent intent = NativeProtocol.createLoginDialog20121101Intent(context, request.getApplicationId(),
                    new ArrayList<String>(request.getPermissions()),
                    request.getDefaultAudience().getNativeProtocolAudience());
            if (intent == null) {
                return false;
            }

            callId = intent.getStringExtra(NativeProtocol.EXTRA_PROTOCOL_CALL_ID);

            addLoggingExtra(EVENT_EXTRAS_APP_CALL_ID, callId);
            addLoggingExtra(EVENT_EXTRAS_PROTOCOL_VERSION,
                    intent.getIntExtra(NativeProtocol.EXTRA_PROTOCOL_VERSION, 0));
            addLoggingExtra(EVENT_EXTRAS_PERMISSIONS,
                    TextUtils.join(",", intent.getStringArrayListExtra(NativeProtocol.EXTRA_PERMISSIONS)));
            addLoggingExtra(EVENT_EXTRAS_WRITE_PRIVACY, intent.getStringExtra(NativeProtocol.EXTRA_WRITE_PRIVACY));
            logEvent(AnalyticsEvents.EVENT_NATIVE_LOGIN_DIALOG_START,
                    AnalyticsEvents.PARAMETER_NATIVE_LOGIN_DIALOG_START_TIME, callId);

            return tryIntent(intent, request.getRequestCode());
        }

        @Override
        boolean onActivityResult(int requestCode, int resultCode, Intent data) {
            Result outcome;

            logEvent(AnalyticsEvents.EVENT_NATIVE_LOGIN_DIALOG_COMPLETE,
                    AnalyticsEvents.PARAMETER_NATIVE_LOGIN_DIALOG_COMPLETE_TIME, callId);

            if (data == null) {
                // This happens if the user presses 'Back'.
                outcome = Result.createCancelResult(pendingRequest, "Operation canceled");
            } else if (NativeProtocol.isServiceDisabledResult20121101(data)) {
                outcome = null;
            } else if (resultCode == Activity.RESULT_CANCELED) {
                outcome = createCancelOrErrorResult(pendingRequest, data);
            } else if (resultCode != Activity.RESULT_OK) {
                outcome = Result.createErrorResult(pendingRequest, "Unexpected resultCode from authorization.", null);
            } else {
                outcome = handleResultOk(data);
            }

            if (outcome != null) {
                completeAndValidate(outcome);
            } else {
                tryNextHandler();
            }

            return true;
        }

        private Result handleResultOk(Intent data) {
            Bundle extras = data.getExtras();
            String errorType = extras.getString(NativeProtocol.STATUS_ERROR_TYPE);
            if (errorType == null) {
                return Result.createTokenResult(pendingRequest,
                        AccessToken.createFromNativeLogin(extras, AccessTokenSource.FACEBOOK_APPLICATION_NATIVE));
            } else if (NativeProtocol.ERROR_SERVICE_DISABLED.equals(errorType)) {
                addLoggingExtra(EVENT_EXTRAS_SERVICE_DISABLED, AppEventsConstants.EVENT_PARAM_VALUE_YES);
                return null;
            } else {
                return createCancelOrErrorResult(pendingRequest, data);
            }
        }

        private Result createCancelOrErrorResult(AuthorizationRequest request, Intent data) {
            Bundle extras = data.getExtras();
            String errorType = extras.getString(NativeProtocol.STATUS_ERROR_TYPE);

            if (NativeProtocol.ERROR_USER_CANCELED.equals(errorType) ||
                    NativeProtocol.ERROR_PERMISSION_DENIED.equals(errorType)) {
                return Result.createCancelResult(request, data.getStringExtra(NativeProtocol.STATUS_ERROR_DESCRIPTION));
            } else {
                // See if we can get an error code out of the JSON.
                String errorJson = extras.getString(NativeProtocol.STATUS_ERROR_JSON);
                String errorCode = null;
                if (errorJson != null) {
                    try {
                        JSONObject jsonObject = new JSONObject(errorJson);
                        errorCode = jsonObject.getString("error_code");
                    } catch (JSONException e) {
                    }
                }
                return Result.createErrorResult(request, errorType,
                        data.getStringExtra(NativeProtocol.STATUS_ERROR_DESCRIPTION), errorCode);
            }
        }

        private void logEvent(String eventName, String timeParameter, String callId) {
            if (callId != null) {
                AppEventsLogger appEventsLogger = AppEventsLogger.newLogger(context, applicationId);
                Bundle parameters = new Bundle();
                parameters.putString(AnalyticsEvents.PARAMETER_APP_ID, applicationId);
                parameters.putString(AnalyticsEvents.PARAMETER_ACTION_ID, callId);
                parameters.putLong(timeParameter, System.currentTimeMillis());
                appEventsLogger.logSdkEvent(eventName, null, parameters);
            }
        }
    }

    class KatanaProxyAuthHandler extends KatanaAuthHandler {
        private static final long serialVersionUID = 1L;
        private String applicationId;

        @Override
        String getNameForLogging() {
            return "katana_proxy_auth";
        }

        @Override
        boolean tryAuthorize(AuthorizationRequest request) {
            applicationId = request.getApplicationId();

            String e2e = getE2E();
            Intent intent = NativeProtocol.createProxyAuthIntent(context, request.getApplicationId(),
                    request.getPermissions(), e2e);

            addLoggingExtra(ServerProtocol.DIALOG_PARAM_E2E, e2e);

            return tryIntent(intent, request.getRequestCode());
        }

        @Override
        boolean onActivityResult(int requestCode, int resultCode, Intent data) {
            // Handle stuff
            Result outcome;

            if (data == null) {
                // This happens if the user presses 'Back'.
                outcome = Result.createCancelResult(pendingRequest, "Operation canceled");
            } else if (resultCode == Activity.RESULT_CANCELED) {
                outcome = Result.createCancelResult(pendingRequest, data.getStringExtra("error"));
            } else if (resultCode != Activity.RESULT_OK) {
                outcome = Result.createErrorResult(pendingRequest, "Unexpected resultCode from authorization.", null);
            } else {
                outcome = handleResultOk(data);
            }

            if (outcome != null) {
                completeAndValidate(outcome);
            } else {
                tryNextHandler();
            }
            return true;
        }

        private Result handleResultOk(Intent data) {
            Bundle extras = data.getExtras();
            String error = extras.getString("error");
            if (error == null) {
                error = extras.getString("error_type");
            }
            String errorCode = extras.getString("error_code");
            String errorMessage = extras.getString("error_message");
            if (errorMessage == null) {
                errorMessage = extras.getString("error_description");
            }

            String e2e = extras.getString(NativeProtocol.FACEBOOK_PROXY_AUTH_E2E_KEY);
            if (!Utility.isNullOrEmpty(e2e)) {
                logWebLoginCompleted(applicationId, e2e);
            }

            if (error == null && errorCode == null && errorMessage == null) {
                AccessToken token = AccessToken.createFromWebBundle(pendingRequest.getPermissions(), extras,
                        AccessTokenSource.FACEBOOK_APPLICATION_WEB);
                return Result.createTokenResult(pendingRequest, token);
            } else if (ServerProtocol.errorsProxyAuthDisabled.contains(error)) {
                return null;
            } else if (ServerProtocol.errorsUserCanceled.contains(error)) {
                return Result.createCancelResult(pendingRequest, null);
            } else {
                return Result.createErrorResult(pendingRequest, error, errorMessage, errorCode);
            }
        }
    }

    private static String getE2E() {
        JSONObject e2e = new JSONObject();
        try {
            e2e.put("init", System.currentTimeMillis());
        } catch (JSONException e) {
        }
        return e2e.toString();
    }

    private void logWebLoginCompleted(String applicationId, String e2e) {
        AppEventsLogger appEventsLogger = AppEventsLogger.newLogger(context, applicationId);

        Bundle parameters = new Bundle();
        parameters.putString(AnalyticsEvents.PARAMETER_WEB_LOGIN_E2E, e2e);
        parameters.putLong(AnalyticsEvents.PARAMETER_WEB_LOGIN_SWITCHBACK_TIME, System.currentTimeMillis());
        parameters.putString(AnalyticsEvents.PARAMETER_APP_ID, applicationId);

        appEventsLogger.logSdkEvent(AnalyticsEvents.EVENT_WEB_LOGIN_COMPLETE, null, parameters);
    }

    static class AuthDialogBuilder extends WebDialog.Builder {
        private static final String OAUTH_DIALOG = "oauth";
        static final String REDIRECT_URI = "fbconnect://success";
        private String e2e;

        public AuthDialogBuilder(Context context, String applicationId, Bundle parameters) {
            super(context, applicationId, OAUTH_DIALOG, parameters);
        }

        public AuthDialogBuilder setE2E(String e2e) {
            this.e2e = e2e;
            return this;
        }

        @Override
        public WebDialog build() {
            Bundle parameters = getParameters();
            parameters.putString(ServerProtocol.DIALOG_PARAM_REDIRECT_URI, REDIRECT_URI);
            parameters.putString(ServerProtocol.DIALOG_PARAM_CLIENT_ID, getApplicationId());
            parameters.putString(ServerProtocol.DIALOG_PARAM_E2E, e2e);

            return new WebDialog(getContext(), OAUTH_DIALOG, parameters, getTheme(), getListener());
        }
    }

    static class AuthorizationRequest implements Serializable {
        private static final long serialVersionUID = 1L;

        private transient final StartActivityDelegate startActivityDelegate;
        private final SessionLoginBehavior loginBehavior;
        private final int requestCode;
        private boolean isLegacy = false;
        private List<String> permissions;
        private final SessionDefaultAudience defaultAudience;
        private final String applicationId;
        private final String previousAccessToken;
        private final String authId;

        AuthorizationRequest(SessionLoginBehavior loginBehavior, int requestCode, boolean isLegacy,
                List<String> permissions, SessionDefaultAudience defaultAudience, String applicationId,
                String validateSameFbidAsToken, StartActivityDelegate startActivityDelegate, String authId) {
            this.loginBehavior = loginBehavior;
            this.requestCode = requestCode;
            this.isLegacy = isLegacy;
            this.permissions = permissions;
            this.defaultAudience = defaultAudience;
            this.applicationId = applicationId;
            this.previousAccessToken = validateSameFbidAsToken;
            this.startActivityDelegate = startActivityDelegate;
            this.authId = authId;

        }

        StartActivityDelegate getStartActivityDelegate() {
            return startActivityDelegate;
        }

        List<String> getPermissions() {
            return permissions;
        }

        void setPermissions(List<String> permissions) {
            this.permissions = permissions;
        }

        SessionLoginBehavior getLoginBehavior() {
            return loginBehavior;
        }

        int getRequestCode() {
            return requestCode;
        }

        SessionDefaultAudience getDefaultAudience() {
            return defaultAudience;
        }

        String getApplicationId() {
            return applicationId;
        }

        boolean isLegacy() {
            return isLegacy;
        }

        void setIsLegacy(boolean isLegacy) {
            this.isLegacy = isLegacy;
        }

        String getPreviousAccessToken() {
            return previousAccessToken;
        }

        boolean needsNewTokenValidation() {
            return previousAccessToken != null && !isLegacy;
        }

        String getAuthId() {
            return authId;
        }
    }


    static class Result implements Serializable {
        private static final long serialVersionUID = 1L;

        enum Code {
            SUCCESS("success"),
            CANCEL("cancel"),
            ERROR("error");

            private final String loggingValue;

            Code(String loggingValue) {
                this.loggingValue = loggingValue;
            }

            // For consistency across platforms, we want to use specific string values when logging these results.
            String getLoggingValue() {
                return loggingValue;
            }
        }

        final Code code;
        final AccessToken token;
        final String errorMessage;
        final String errorCode;
        final AuthorizationRequest request;
        Map<String, String> loggingExtras;

        private Result(AuthorizationRequest request, Code code, AccessToken token, String errorMessage,
                String errorCode) {
            this.request = request;
            this.token = token;
            this.errorMessage = errorMessage;
            this.code = code;
            this.errorCode = errorCode;
        }

        static Result createTokenResult(AuthorizationRequest request, AccessToken token) {
            return new Result(request, Code.SUCCESS, token, null, null);
        }

        static Result createCancelResult(AuthorizationRequest request, String message) {
            return new Result(request, Code.CANCEL, null, message, null);
        }

        static Result createErrorResult(AuthorizationRequest request, String errorType, String errorDescription) {
            return createErrorResult(request, errorType, errorDescription, null);
        }

        static Result createErrorResult(AuthorizationRequest request, String errorType, String errorDescription,
                String errorCode) {
            String message = TextUtils.join(": ", Utility.asListNoNulls(errorType, errorDescription));
            return new Result(request, Code.ERROR, null, message, errorCode);
        }
    }
}
