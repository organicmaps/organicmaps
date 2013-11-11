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

import android.content.ContentResolver;
import android.content.Context;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import com.facebook.android.BuildConfig;
import com.facebook.internal.Utility;
import com.facebook.model.GraphObject;
import com.facebook.internal.Validate;
import org.json.JSONException;
import org.json.JSONObject;

import java.lang.reflect.Field;
import java.util.*;
import java.util.concurrent.*;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * Allows some customization of sdk behavior.
 */
public final class Settings {
    private static final String TAG = Settings.class.getCanonicalName();
    private static final HashSet<LoggingBehavior> loggingBehaviors =
            new HashSet<LoggingBehavior>(Arrays.asList(LoggingBehavior.DEVELOPER_ERRORS));
    private static volatile Executor executor;
    private static volatile boolean shouldAutoPublishInstall;
    private static volatile String appVersion;
    private static final String FACEBOOK_COM = "facebook.com";
    private static volatile String facebookDomain = FACEBOOK_COM;

    private static final int DEFAULT_CORE_POOL_SIZE = 5;
    private static final int DEFAULT_MAXIMUM_POOL_SIZE = 128;
    private static final int DEFAULT_KEEP_ALIVE = 1;
    private static final Object LOCK = new Object();

    private static final Uri ATTRIBUTION_ID_CONTENT_URI =
            Uri.parse("content://com.facebook.katana.provider.AttributionIdProvider");
    private static final String ATTRIBUTION_ID_COLUMN_NAME = "aid";

    private static final String ATTRIBUTION_PREFERENCES = "com.facebook.sdk.attributionTracking";
    private static final String PUBLISH_ACTIVITY_PATH = "%s/activities";
    private static final String MOBILE_INSTALL_EVENT = "MOBILE_APP_INSTALL";
    private static final String ANALYTICS_EVENT = "event";
    private static final String ATTRIBUTION_KEY = "attribution";
    private static final String AUTO_PUBLISH = "auto_publish";

    private static final BlockingQueue<Runnable> DEFAULT_WORK_QUEUE = new LinkedBlockingQueue<Runnable>(10);

    private static final ThreadFactory DEFAULT_THREAD_FACTORY = new ThreadFactory() {
        private final AtomicInteger counter = new AtomicInteger(0);

        public Thread newThread(Runnable runnable) {
            return new Thread(runnable, "FacebookSdk #" + counter.incrementAndGet());
        }
    };

    /**
     * Certain logging behaviors are available for debugging beyond those that should be
     * enabled in production.
     *
     * Returns the types of extended logging that are currently enabled.
     *
     * @return a set containing enabled logging behaviors
     */
    public static final Set<LoggingBehavior> getLoggingBehaviors() {
        synchronized (loggingBehaviors) {
            return Collections.unmodifiableSet(new HashSet<LoggingBehavior>(loggingBehaviors));
        }
    }

    /**
     * Certain logging behaviors are available for debugging beyond those that should be
     * enabled in production.
     *
     * Enables a particular extended logging in the sdk.
     *
     * @param behavior
     *          The LoggingBehavior to enable
     */
    public static final void addLoggingBehavior(LoggingBehavior behavior) {
        synchronized (loggingBehaviors) {
            loggingBehaviors.add(behavior);
        }
    }

    /**
     * Certain logging behaviors are available for debugging beyond those that should be
     * enabled in production.
     *
     * Disables a particular extended logging behavior in the sdk.
     *
     * @param behavior
     *          The LoggingBehavior to disable
     */
    public static final void removeLoggingBehavior(LoggingBehavior behavior) {
        synchronized (loggingBehaviors) {
            loggingBehaviors.remove(behavior);
        }
    }

    /**
     * Certain logging behaviors are available for debugging beyond those that should be
     * enabled in production.
     *
     * Disables all extended logging behaviors.
     */
    public static final void clearLoggingBehaviors() {
        synchronized (loggingBehaviors) {
            loggingBehaviors.clear();
        }
    }

    /**
     * Certain logging behaviors are available for debugging beyond those that should be
     * enabled in production.
     *
     * Checks if a particular extended logging behavior is enabled.
     *
     * @param behavior
     *          The LoggingBehavior to check
     * @return whether behavior is enabled
     */
    public static final boolean isLoggingBehaviorEnabled(LoggingBehavior behavior) {
        synchronized (loggingBehaviors) {
            return BuildConfig.DEBUG && loggingBehaviors.contains(behavior);
        }
    }

    /**
     * Returns the Executor used by the SDK for non-AsyncTask background work.
     *
     * By default this uses AsyncTask Executor via reflection if the API level is high enough.
     * Otherwise this creates a new Executor with defaults similar to those used in AsyncTask.
     *
     * @return an Executor used by the SDK.  This will never be null.
     */
    public static Executor getExecutor() {
        synchronized (LOCK) {
            if (Settings.executor == null) {
                Executor executor = getAsyncTaskExecutor();
                if (executor == null) {
                    executor = new ThreadPoolExecutor(DEFAULT_CORE_POOL_SIZE, DEFAULT_MAXIMUM_POOL_SIZE,
                            DEFAULT_KEEP_ALIVE, TimeUnit.SECONDS, DEFAULT_WORK_QUEUE, DEFAULT_THREAD_FACTORY);
                }
                Settings.executor = executor;
            }
        }
        return Settings.executor;
    }

    /**
     * Sets the Executor used by the SDK for non-AsyncTask background work.
     *
     * @param executor
     *          the Executor to use; must not be null.
     */
    public static void setExecutor(Executor executor) {
        Validate.notNull(executor, "executor");
        synchronized (LOCK) {
            Settings.executor = executor;
        }
    }

    /**
     * Gets the base Facebook domain to use when making Web requests; in production code this will always be
     * "facebook.com".
     *
     * @return the Facebook domain
     */
    public static String getFacebookDomain() {
        return facebookDomain;
    }

    /**
     * Sets the base Facebook domain to use when making Web requests. This defaults to "facebook.com", but may
     * be overridden to, e.g., "beta.facebook.com" to direct requests at a different domain. This method should
     * never be called from production code.
     *
     * @param facebookDomain the base domain to use instead of "facebook.com"
     */
    public static void setFacebookDomain(String facebookDomain) {
        if (!BuildConfig.DEBUG) {
            Log.w(TAG, "WARNING: Calling setFacebookDomain from non-DEBUG code.");
        }

        Settings.facebookDomain = facebookDomain;
    }

    private static Executor getAsyncTaskExecutor() {
        Field executorField = null;
        try {
            executorField = AsyncTask.class.getField("THREAD_POOL_EXECUTOR");
        } catch (NoSuchFieldException e) {
            return null;
        }

        Object executorObject = null;
        try {
            executorObject = executorField.get(null);
        } catch (IllegalAccessException e) {
            return null;
        }

        if (executorObject == null) {
            return null;
        }

        if (!(executorObject instanceof Executor)) {
            return null;
        }

        return (Executor) executorObject;
    }

    /**
     * Manually publish install attribution to the Facebook graph.  Internally handles tracking repeat calls to prevent
     * multiple installs being published to the graph.
     * @param context the current Context
     * @param applicationId the fb application being published.
     *
     * This method is deprecated.  See {@link AppEventsLogger#activateApp(Context, String)} for more info.
     */
    @Deprecated
    public static void publishInstallAsync(final Context context, final String applicationId) {
       publishInstallAsync(context, applicationId, null);
    }

    /**
     * Manually publish install attribution to the Facebook graph.  Internally handles tracking repeat calls to prevent
     * multiple installs being published to the graph.
     * @param context the current Context
     * @param applicationId the fb application being published.
     * @param callback a callback to invoke with a Response object, carrying the server response, or an error.
     *
     * This method is deprecated.  See {@link AppEventsLogger#activateApp(Context, String)} for more info.
     */
    @Deprecated
    public static void publishInstallAsync(final Context context, final String applicationId,
        final Request.Callback callback) {
        // grab the application context ahead of time, since we will return to the caller immediately.
        final Context applicationContext = context.getApplicationContext();
        Settings.getExecutor().execute(new Runnable() {
            @Override
            public void run() {
                final Response response = Settings.publishInstallAndWaitForResponse(applicationContext, applicationId);
                if (callback != null) {
                    // invoke the callback on the main thread.
                    Handler handler = new Handler(Looper.getMainLooper());
                    handler.post(new Runnable() {
                        @Override
                        public void run() {
                            callback.onCompleted(response);
                        }
                    });
                }
            }
        });
    }

    /**
     * Sets whether opening a Session should automatically publish install attribution to the Facebook graph.
     *
     * @param shouldAutoPublishInstall true to automatically publish, false to not
     *
     * This method is deprecated.  See {@link AppEventsLogger#activateApp(Context, String)} for more info.
     */
    @Deprecated
    public static void setShouldAutoPublishInstall(boolean shouldAutoPublishInstall) {
        Settings.shouldAutoPublishInstall = shouldAutoPublishInstall;
    }

    /**
     * Gets whether opening a Session should automatically publish install attribution to the Facebook graph.
     *
     * @return true to automatically publish, false to not
     *
     * This method is deprecated.  See {@link AppEventsLogger#activateApp(Context, String)} for more info.
     */
    @Deprecated
    public static boolean getShouldAutoPublishInstall() {
        return shouldAutoPublishInstall;
    }

    /**
     * Manually publish install attribution to the Facebook graph.  Internally handles tracking repeat calls to prevent
     * multiple installs being published to the graph.
     * @param context the current Context
     * @param applicationId the fb application being published.
     * @return returns false on error.  Applications should retry until true is returned.  Safe to call again after
     * true is returned.
     *
     * This method is deprecated.  See {@link AppEventsLogger#activateApp(Context, String)} for more info.
     */
    @Deprecated
    public static boolean publishInstallAndWait(final Context context, final String applicationId) {
        Response response = publishInstallAndWaitForResponse(context, applicationId);
        return response != null && response.getError() == null;
    }

    /**
     * Manually publish install attribution to the Facebook graph.  Internally handles caching repeat calls to prevent
     * multiple installs being published to the graph.
     * @param context the current Context
     * @param applicationId the fb application being published.
     * @return returns a Response object, carrying the server response, or an error.
     *
     * This method is deprecated.  See {@link AppEventsLogger#activateApp(Context, String)} for more info.
     */
    @Deprecated
    public static Response publishInstallAndWaitForResponse(final Context context, final String applicationId) {
        return publishInstallAndWaitForResponse(context, applicationId, false);
    }

    static Response publishInstallAndWaitForResponse(
            final Context context,
            final String applicationId,
            final boolean isAutoPublish) {
        try {
            if (context == null || applicationId == null) {
                throw new IllegalArgumentException("Both context and applicationId must be non-null");
            }
            String attributionId = Settings.getAttributionId(context.getContentResolver());
            SharedPreferences preferences = context.getSharedPreferences(ATTRIBUTION_PREFERENCES, Context.MODE_PRIVATE);
            String pingKey = applicationId+"ping";
            String jsonKey = applicationId+"json";
            long lastPing = preferences.getLong(pingKey, 0);
            String lastResponseJSON = preferences.getString(jsonKey, null);

            // prevent auto publish from occurring if we have an explicit call.
            if (!isAutoPublish) {
                setShouldAutoPublishInstall(false);
            }

            GraphObject publishParams = GraphObject.Factory.create();
            publishParams.setProperty(ANALYTICS_EVENT, MOBILE_INSTALL_EVENT);
            publishParams.setProperty(ATTRIBUTION_KEY, attributionId);
            publishParams.setProperty(AUTO_PUBLISH, isAutoPublish);
            publishParams.setProperty("application_tracking_enabled", !AppEventsLogger.getLimitEventUsage(context));
            publishParams.setProperty("application_package_name", context.getPackageName());

            String publishUrl = String.format(PUBLISH_ACTIVITY_PATH, applicationId);
            Request publishRequest = Request.newPostRequest(null, publishUrl, publishParams, null);

            if (lastPing != 0) {
                GraphObject graphObject = null;
                try {
                    if (lastResponseJSON != null) {
                        graphObject = GraphObject.Factory.create(new JSONObject(lastResponseJSON));
                    }
                }
                catch (JSONException je) {
                    // return the default graph object if there is any problem reading the data.
                }
                if (graphObject == null) {
                    return Response.createResponsesFromString("true", null, new RequestBatch(publishRequest), true).get(0);
                } else {
                    return new Response(null, null, graphObject, true);
                }
            } else if (attributionId == null) {
                throw new FacebookException("No attribution id returned from the Facebook application");
            } else {

                if (!Utility.queryAppSettings(applicationId, false).supportsAttribution()) {
                    throw new FacebookException("Install attribution has been disabled on the server.");
                }

                Response publishResponse = publishRequest.executeAndWait();

                // denote success since no error threw from the post.
                SharedPreferences.Editor editor = preferences.edit();
                lastPing = System.currentTimeMillis();
                editor.putLong(pingKey, lastPing);

                // if we got an object response back, cache the string of the JSON.
                if (publishResponse.getGraphObject() != null &&
                    publishResponse.getGraphObject().getInnerJSONObject() != null) {
                    editor.putString(jsonKey, publishResponse.getGraphObject().getInnerJSONObject().toString());
                }
                editor.commit();

                return publishResponse;
            }
        } catch (Exception e) {
            // if there was an error, fall through to the failure case.
            Utility.logd("Facebook-publish", e);
            return new Response(null, null, new FacebookRequestError(null, e));
        }
    }

    /**
     * Acquire the current attribution id from the facebook app.
     * @return returns null if the facebook app is not present on the phone.
     */
    public static String getAttributionId(ContentResolver contentResolver) {
        try {
            String [] projection = {ATTRIBUTION_ID_COLUMN_NAME};
            Cursor c = contentResolver.query(ATTRIBUTION_ID_CONTENT_URI, projection, null, null, null);
            if (c == null || !c.moveToFirst()) {
                return null;
            }
            String attributionId = c.getString(c.getColumnIndex(ATTRIBUTION_ID_COLUMN_NAME));
            c.close();
            return attributionId;
        } catch (Exception e) {
            Log.d(TAG, "Caught unexpected exception in getAttributionId(): " + e.toString());
            return null;
        }
    }

    /**
     * Gets the application version to the provided string.
     * @return application version set via setAppVersion.
     */
    public static String getAppVersion() {
        return appVersion;
    }

    /**
     * Sets the application version to the provided string.  AppEventsLogger.logEvent calls logs its event with the
     * current app version, and App Insights allows breakdown of events by app version.
     *
     * @param appVersion  The version identifier of the Android app that events are being logged through.
     *                    Enables analysis and breakdown of logged events by app version.
     */
    public static void setAppVersion(String appVersion) {
        Settings.appVersion = appVersion;
    }

    /**
     * Gets the current version of the Facebook SDK for Android as a string.
     *
     * @return the current version of the SDK
     */
    public static String getSdkVersion() {
        return FacebookSdkVersion.BUILD;
    }

    /**
     * Gets the current Facebook migration bundle string; this string can be passed to Graph API
     * endpoints to specify a set of platform migrations that are explicitly turned on or off for
     * that call, in order to ensure compatibility between a given version of the SDK and the
     * Graph API.
     * @return the migration bundle supported by this version of the SDK
     */
    public static String getMigrationBundle() {
        return FacebookSdkVersion.MIGRATION_BUNDLE;
    }
}
