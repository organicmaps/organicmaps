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
import android.content.Context;
import android.content.ComponentName;
import android.content.Intent;
import android.os.Bundle;
import android.support.v4.content.LocalBroadcastManager;
import android.util.Log;
import bolts.AppLinks;
import com.facebook.internal.AttributionIdentifiers;
import com.facebook.internal.Logger;
import com.facebook.internal.Utility;
import com.facebook.internal.Validate;
import com.facebook.model.GraphObject;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.*;
import java.math.BigDecimal;
import java.util.*;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ScheduledThreadPoolExecutor;
import java.util.concurrent.TimeUnit;


/**
 * <p>
 * The AppEventsLogger class allows the developer to log various types of events back to Facebook.  In order to log
 * events, the app must create an instance of this class via a {@link #newLogger newLogger} method, and then call
 * the various "log" methods off of that.
 * </p>
 * <p>
 * This client-side event logging is then available through Facebook App Insights
 * and for use with Facebook Ads conversion tracking and optimization.
 * </p>
 * <p>
 * The AppEventsLogger class has a few related roles:
 * </p>
 * <ul>
 * <li>
 * Logging predefined and application-defined events to Facebook App Insights with a
 * numeric value to sum across a large number of events, and an optional set of key/value
 * parameters that define "segments" for this event (e.g., 'purchaserStatus' : 'frequent', or
 * 'gamerLevel' : 'intermediate').  These events may also be used for ads conversion tracking,
 * optimization, and other ads related targeting in the future.
 * </li>
 * <li>
 * Methods that control the way in which events are flushed out to the Facebook servers.
 * </li>
 * </ul>
 * Here are some important characteristics of the logging mechanism provided by AppEventsLogger:
 * <ul>
 * <li>
 * Events are not sent immediately when logged.  They're cached and flushed out to the Facebook servers
 * in a number of situations:
 * <ul>
 * <li>when an event count threshold is passed (currently 100 logged events).</li>
 * <li>when a time threshold is passed (currently 60 seconds).</li>
 * <li>when an app has gone to background and is then brought back to the foreground.</li>
 * </ul>
 * <li>
 * Events will be accumulated when the app is in a disconnected state, and sent when the connection is
 * restored and one of the above 'flush' conditions are met.
 * </li>
 * <li>
 * The AppEventsLogger class is intended to be used from the thread it was created on.  Multiple AppEventsLoggers
 * may be created on other threads if desired.
 * </li>
 * <li>
 * The developer can call the setFlushBehavior method to force the flushing of events to only
 * occur on an explicit call to the `flush` method.
 * </li>
 * <li>
 * The developer can turn on console debug output for event logging and flushing to the server
 * Settings.addLoggingBehavior(LoggingBehavior.APP_EVENTS);
 * </li>
 * </ul>
 * Some things to note when logging events:
 * <ul>
 * <li>
 * There is a limit on the number of unique event names an app can use, on the order of 300.
 * </li>
 * <li>
 * There is a limit to the number of unique parameter names in the provided parameters that can
 * be used per event, on the order of 25.  This is not just for an individual call, but for all
 * invocations for that eventName.
 * </li>
 * <li>
 * Event names and parameter names (the keys in the NSDictionary) must be between 2 and 40 characters, and
 * must consist of alphanumeric characters, _, -, or spaces.
 * </li>
 * <li>
 * The length of each parameter value can be no more than on the order of 100 characters.
 * </li>
 * </ul>
 */
public class AppEventsLogger {
    // Enums

    /**
     * Controls when an AppEventsLogger sends log events to the server
     */
    public enum FlushBehavior {
        /**
         * Flush automatically: periodically (once a minute or after every 100 events), and always at app reactivation.
         * This is the default value.
         */
        AUTO,

        /**
         * Only flush when AppEventsLogger.flush() is explicitly invoked.
         */
        EXPLICIT_ONLY,
    }

    // Constants
    private static final String TAG = AppEventsLogger.class.getCanonicalName();

    private static final int NUM_LOG_EVENTS_TO_TRY_TO_FLUSH_AFTER = 100;
    private static final int FLUSH_PERIOD_IN_SECONDS = 60;
    private static final int APP_SUPPORTS_ATTRIBUTION_ID_RECHECK_PERIOD_IN_SECONDS = 60 * 60 * 24;
    private static final int FLUSH_APP_SESSION_INFO_IN_SECONDS = 30;

    private static final String SOURCE_APPLICATION_HAS_BEEN_SET_BY_THIS_INTENT = "_fbSourceApplicationHasBeenSet";

    // Instance member variables
    private final Context context;
    private final AccessTokenAppIdPair accessTokenAppId;

    private static Map<AccessTokenAppIdPair, SessionEventsState> stateMap =
            new ConcurrentHashMap<AccessTokenAppIdPair, SessionEventsState>();
    private static ScheduledThreadPoolExecutor backgroundExecutor;
    private static FlushBehavior flushBehavior = FlushBehavior.AUTO;
    private static boolean requestInFlight;
    private static Context applicationContext;
    private static Object staticLock = new Object();
    private static String hashedDeviceAndAppId;
    private static String sourceApplication;
    private static boolean isOpenedByApplink;

    // Rather than retaining Sessions, we extract the information we need and track app events by
    // application ID and access token (which may be null for Session-less calls). This avoids needing to
    // worry about Session lifecycle and also allows us to coalesce app events from different Sessions
    // that have the same access token/app ID.
    private static class AccessTokenAppIdPair implements Serializable {
        private static final long serialVersionUID = 1L;
        private final String accessToken;
        private final String applicationId;

        AccessTokenAppIdPair(Session session) {
            this(session.getAccessToken(), session.getApplicationId());
        }

        AccessTokenAppIdPair(String accessToken, String applicationId) {
            this.accessToken = Utility.isNullOrEmpty(accessToken) ? null : accessToken;
            this.applicationId = applicationId;
        }

        String getAccessToken() {
            return accessToken;
        }

        String getApplicationId() {
            return applicationId;
        }

        @Override
        public int hashCode() {
            return (accessToken == null ? 0 : accessToken.hashCode()) ^
                    (applicationId == null ? 0 : applicationId.hashCode());
        }

        @Override
        public boolean equals(Object o) {
            if (!(o instanceof AccessTokenAppIdPair)) {
                return false;
            }
            AccessTokenAppIdPair p = (AccessTokenAppIdPair) o;
            return Utility.areObjectsEqual(p.accessToken, accessToken) &&
                    Utility.areObjectsEqual(p.applicationId, applicationId);
        }

        private static class SerializationProxyV1 implements Serializable {
            private static final long serialVersionUID = -2488473066578201069L;
            private final String accessToken;
            private final String appId;

            private SerializationProxyV1(String accessToken, String appId) {
                this.accessToken = accessToken;
                this.appId = appId;
            }

            private Object readResolve() {
                return new AccessTokenAppIdPair(accessToken, appId);
            }
        }

        private Object writeReplace() {
            return new SerializationProxyV1(accessToken, applicationId);
        }
    }

    /**
     * This method is deprecated.  Use {@link Settings#getLimitEventAndDataUsage(Context)} instead.
     */
    @Deprecated
    public static boolean getLimitEventUsage(Context context) {
        return Settings.getLimitEventAndDataUsage(context);
    }

    /**
     * This method is deprecated.  Use {@link Settings#setLimitEventAndDataUsage(Context, boolean)} instead.
     */
    @Deprecated
    public static void setLimitEventUsage(Context context, boolean limitEventUsage) {
        Settings.setLimitEventAndDataUsage(context, limitEventUsage);
    }

    /**
     * Notifies the events system that the app has launched & logs an activatedApp event.  Should be called whenever
     * your app becomes active, typically in the onResume() method of each long-running Activity of your app.
     *
     * Use this method if your application ID is stored in application metadata, otherwise see
     * {@link AppEventsLogger#activateApp(android.content.Context, String)}.
     *
     * @param context Used to access the applicationId and the attributionId for non-authenticated users.
     */
    public static void activateApp(Context context) {
        Settings.sdkInitialize(context);
        activateApp(context, Utility.getMetadataApplicationId(context));
    }

    /**
     * Notifies the events system that the app has launched & logs an activatedApp event.  Should be called whenever
     * your app becomes active, typically in the onResume() method of each long-running Activity of your app.
     *
     * @param context       Used to access the attributionId for non-authenticated users.
     * @param applicationId The specific applicationId to report the activation for.
     */
    @SuppressWarnings("deprecation")
    public static void activateApp(Context context, String applicationId) {
        if (context == null || applicationId == null) {
            throw new IllegalArgumentException("Both context and applicationId must be non-null");
        }

        if ((context instanceof Activity)) {
            setSourceApplication((Activity) context);
        } else {
          // If context is not an Activity, we cannot get intent nor calling activity.
          resetSourceApplication();
          Log.d(AppEventsLogger.class.getName(),
              "To set source application the context of activateApp must be an instance of Activity");
        }

        // activateApp supercedes publishInstall in the public API, so we need to explicitly invoke it, since the server
        // can't reliably infer install state for all conditions of an app activate.
        Settings.publishInstallAsync(context, applicationId, null);

        final AppEventsLogger logger = new AppEventsLogger(context, applicationId, null);
        final long eventTime = System.currentTimeMillis();
        final String sourceApplicationInfo = getSourceApplication();
        backgroundExecutor.execute(new Runnable() {
            @Override
            public void run() {
                logger.logAppSessionResumeEvent(eventTime, sourceApplicationInfo);
            }
        });
    }

    /**
     * Notifies the events system that the app has been deactivated (put in the background) and
     * tracks the application session information. Should be called whenever your app becomes
     * inactive, typically in the onPause() method of each long-running Activity of your app.
     *
     * Use this method if your application ID is stored in application metadata, otherwise see
     * {@link AppEventsLogger#deactivateApp(android.content.Context, String)}.
     *
     * @param context Used to access the applicationId and the attributionId for non-authenticated users.
     */
    public static void deactivateApp(Context context) {
        deactivateApp(context, Utility.getMetadataApplicationId(context));
    }

    /**
     * Notifies the events system that the app has been deactivated (put in the background) and
     * tracks the application session information. Should be called whenever your app becomes
     * inactive, typically in the onPause() method of each long-running Activity of your app.
     *
     * @param context       Used to access the attributionId for non-authenticated users.
     * @param applicationId The specific applicationId to track session information for.
     */
    public static void deactivateApp(Context context, String applicationId) {
        if (context == null || applicationId == null) {
            throw new IllegalArgumentException("Both context and applicationId must be non-null");
        }

        resetSourceApplication();

        final AppEventsLogger logger = new AppEventsLogger(context, applicationId, null);
        final long eventTime = System.currentTimeMillis();
        backgroundExecutor.execute(new Runnable() {
            @Override
            public void run() {
                logger.logAppSessionSuspendEvent(eventTime);
            }
        });
    }

    private void logAppSessionResumeEvent(long eventTime, String sourceApplicationInfo) {
        PersistedAppSessionInfo.onResume(applicationContext, accessTokenAppId, this, eventTime, sourceApplicationInfo);
    }

    private void logAppSessionSuspendEvent(long eventTime) {
        PersistedAppSessionInfo.onSuspend(applicationContext, accessTokenAppId, this, eventTime);
    }

    /**
     * Build an AppEventsLogger instance to log events through.  The Facebook app that these events are targeted at
     * comes from this application's metadata. The application ID used to log events will be determined from
     * the app ID specified in the package metadata.
     *
     * @param context Used to access the applicationId and the attributionId for non-authenticated users.
     * @return AppEventsLogger instance to invoke log* methods on.
     */
    public static AppEventsLogger newLogger(Context context) {
        return new AppEventsLogger(context, null, null);
    }

    /**
     * Build an AppEventsLogger instance to log events through.
     *
     * @param context Used to access the attributionId for non-authenticated users.
     * @param session Explicitly specified Session to log events against.  If null, the activeSession
     *                will be used if it's open, otherwise the logging will happen against the default
     *                app ID specified via the app ID specified in the package metadata.
     * @return AppEventsLogger instance to invoke log* methods on.
     */
    public static AppEventsLogger newLogger(Context context, Session session) {
        return new AppEventsLogger(context, null, session);
    }

    /**
     * Build an AppEventsLogger instance to log events through.
     *
     * @param context       Used to access the attributionId for non-authenticated users.
     * @param applicationId Explicitly specified Facebook applicationId to log events against.  If null, the default
     *                      app ID specified in the package metadata will be used.
     * @param session       Explicitly specified Session to log events against.  If null, the activeSession
     *                      will be used if it's open, otherwise the logging will happen against the specified
     *                      app ID.
     * @return AppEventsLogger instance to invoke log* methods on.
     */
    public static AppEventsLogger newLogger(Context context, String applicationId, Session session) {
        return new AppEventsLogger(context, applicationId, session);
    }

    /**
     * Build an AppEventsLogger instance to log events that are attributed to the application but not to
     * any particular Session.
     *
     * @param context       Used to access the attributionId for non-authenticated users.
     * @param applicationId Explicitly specified Facebook applicationId to log events against.  If null, the default
     *                      app ID specified
     *                      in the package metadata will be used.
     * @return AppEventsLogger instance to invoke log* methods on.
     */
    public static AppEventsLogger newLogger(Context context, String applicationId) {
        return new AppEventsLogger(context, applicationId, null);
    }

    /**
     * The action used to indicate that a flush of app events has occurred. This should
     * be used as an action in an IntentFilter and BroadcastReceiver registered with
     * the {@link android.support.v4.content.LocalBroadcastManager}.
     */
    public static final String ACTION_APP_EVENTS_FLUSHED = "com.facebook.sdk.APP_EVENTS_FLUSHED";

    public static final String APP_EVENTS_EXTRA_NUM_EVENTS_FLUSHED = "com.facebook.sdk.APP_EVENTS_NUM_EVENTS_FLUSHED";
    public static final String APP_EVENTS_EXTRA_FLUSH_RESULT = "com.facebook.sdk.APP_EVENTS_FLUSH_RESULT";

    /**
     * Access the behavior that AppEventsLogger uses to determine when to flush logged events to the server. This
     * setting applies to all instances of AppEventsLogger.
     *
     * @return specified flush behavior.
     */
    public static FlushBehavior getFlushBehavior() {
        synchronized (staticLock) {
            return flushBehavior;
        }
    }

    /**
     * Set the behavior that this AppEventsLogger uses to determine when to flush logged events to the server. This
     * setting applies to all instances of AppEventsLogger.
     *
     * @param flushBehavior the desired behavior.
     */
    public static void setFlushBehavior(FlushBehavior flushBehavior) {
        synchronized (staticLock) {
            AppEventsLogger.flushBehavior = flushBehavior;
        }
    }

    /**
     * Log an app event with the specified name.
     *
     * @param eventName eventName used to denote the event.  Choose amongst the EVENT_NAME_* constants in
     *                  {@link AppEventsConstants} when possible.  Or create your own if none of the EVENT_NAME_*
     *                  constants are applicable.
     *                  Event names should be 40 characters or less, alphanumeric, and can include spaces, underscores
     *                  or hyphens, but mustn't have a space or hyphen as the first character.  Any given app should
     *                  have no more than ~300 distinct event names.
     */
    public void logEvent(String eventName) {
        logEvent(eventName, null);
    }

    /**
     * Log an app event with the specified name and the supplied value.
     *
     * @param eventName  eventName used to denote the event.  Choose amongst the EVENT_NAME_* constants in
     *                   {@link AppEventsConstants} when possible.  Or create your own if none of the EVENT_NAME_*
     *                   constants are applicable.
     *                   Event names should be 40 characters or less, alphanumeric, and can include spaces, underscores
     *                   or hyphens, but mustn't have a space or hyphen as the first character.  Any given app should
     *                   have no more than ~300 distinct event names.
     *                   * @param eventName
     * @param valueToSum a value to associate with the event which will be summed up in Insights for across all
     *                   instances of the event, so that average values can be determined, etc.
     */
    public void logEvent(String eventName, double valueToSum) {
        logEvent(eventName, valueToSum, null);
    }

    /**
     * Log an app event with the specified name and set of parameters.
     *
     * @param eventName  eventName used to denote the event.  Choose amongst the EVENT_NAME_* constants in
     *                   {@link AppEventsConstants} when possible.  Or create your own if none of the EVENT_NAME_*
     *                   constants are applicable.
     *                   Event names should be 40 characters or less, alphanumeric, and can include spaces, underscores
     *                   or hyphens, but mustn't have a space or hyphen as the first character.  Any given app should
     *                   have no more than ~300 distinct event names.
     * @param parameters A Bundle of parameters to log with the event.  Insights will allow looking at the logs of these
     *                   events via different parameter values.  You can log on the order of 10 parameters with each
     *                   distinct eventName.  It's advisable to keep the number of unique values provided for each
     *                   parameter in the, at most, thousands.  As an example, don't attempt to provide a unique
     *                   parameter value for each unique user in your app.  You won't get meaningful aggregate reporting
     *                   on so many parameter values.  The values in the bundles should be Strings or numeric values.
     */
    public void logEvent(String eventName, Bundle parameters) {
        logEvent(eventName, null, parameters, false);
    }

    /**
     * Log an app event with the specified name, supplied value, and set of parameters.
     *
     * @param eventName  eventName used to denote the event.  Choose amongst the EVENT_NAME_* constants in
     *                   {@link AppEventsConstants} when possible.  Or create your own if none of the EVENT_NAME_*
     *                   constants are applicable.
     *                   Event names should be 40 characters or less, alphanumeric, and can include spaces, underscores
     *                   or hyphens, but mustn't have a space or hyphen as the first character.  Any given app should
     *                   have no more than ~300 distinct event names.
     * @param valueToSum a value to associate with the event which will be summed up in Insights for across all
     *                   instances of the event, so that average values can be determined, etc.
     * @param parameters A Bundle of parameters to log with the event.  Insights will allow looking at the logs of these
     *                   events via different parameter values.  You can log on the order of 10 parameters with each
     *                   distinct eventName.  It's advisable to keep the number of unique values provided for each
     *                   parameter in the, at most, thousands.  As an example, don't attempt to provide a unique
     *                   parameter value for each unique user in your app.  You won't get meaningful aggregate reporting
     *                   on so many parameter values.  The values in the bundles should be Strings or numeric values.
     */
    public void logEvent(String eventName, double valueToSum, Bundle parameters) {
        logEvent(eventName, valueToSum, parameters, false);
    }

    /**
     * Logs a purchase event with Facebook, in the specified amount and with the specified currency.
     *
     * @param purchaseAmount Amount of purchase, in the currency specified by the 'currency' parameter. This value
     *                       will be rounded to the thousandths place (e.g., 12.34567 becomes 12.346).
     * @param currency       Currency used to specify the amount.
     */
    public void logPurchase(BigDecimal purchaseAmount, Currency currency) {
        logPurchase(purchaseAmount, currency, null);
    }

    /**
     * Logs a purchase event with Facebook, in the specified amount and with the specified currency.  Additional
     * detail about the purchase can be passed in through the parameters bundle.
     *
     * @param purchaseAmount Amount of purchase, in the currency specified by the 'currency' parameter. This value
     *                       will be rounded to the thousandths place (e.g., 12.34567 becomes 12.346).
     * @param currency       Currency used to specify the amount.
     * @param parameters     Arbitrary additional information for describing this event.  Should have no more than
     *                       10 entries, and keys should be mostly consistent from one purchase event to the next.
     */
    public void logPurchase(BigDecimal purchaseAmount, Currency currency, Bundle parameters) {

        if (purchaseAmount == null) {
            notifyDeveloperError("purchaseAmount cannot be null");
            return;
        } else if (currency == null) {
            notifyDeveloperError("currency cannot be null");
            return;
        }

        if (parameters == null) {
            parameters = new Bundle();
        }
        parameters.putString(AppEventsConstants.EVENT_PARAM_CURRENCY, currency.getCurrencyCode());

        logEvent(AppEventsConstants.EVENT_NAME_PURCHASED, purchaseAmount.doubleValue(), parameters);
        eagerFlush();
    }

    /**
     * Explicitly flush any stored events to the server.  Implicit flushes may happen depending on the value
     * of getFlushBehavior.  This method allows for explicit, app invoked flushing.
     */
    public void flush() {
        flush(FlushReason.EXPLICIT);
    }

    /**
     * Call this when the consuming Activity/Fragment receives an onStop() callback in order to persist any
     * outstanding events to disk, so they may be flushed at a later time. The next flush (explicit or not)
     * will check for any outstanding events and, if present, include them in that flush. Note that this call
     * may trigger an I/O operation on the calling thread. Explicit use of this method is not necessary
     * if the consumer is making use of {@link UiLifecycleHelper}, which will take care of making the call
     * in its own onStop() callback.
     */
    public static void onContextStop() {
        PersistedEvents.persistEvents(applicationContext, stateMap);
    }

    boolean isValidForSession(Session session) {
        AccessTokenAppIdPair other = new AccessTokenAppIdPair(session);
        return accessTokenAppId.equals(other);
    }

    /**
     * This method is intended only for internal use by the Facebook SDK and other use is unsupported.
     */
    public void logSdkEvent(String eventName, Double valueToSum, Bundle parameters) {
        logEvent(eventName, valueToSum, parameters, true);
    }

    /**
     * Returns the app ID this logger was configured to log to.
     *
     * @return the Facebook app ID
     */
    public String getApplicationId() {
        return accessTokenAppId.getApplicationId();
    }

    //
    // Private implementation
    //

    @SuppressWarnings("UnusedDeclaration")
    private enum FlushReason {
        EXPLICIT,
        TIMER,
        SESSION_CHANGE,
        PERSISTED_EVENTS,
        EVENT_THRESHOLD,
        EAGER_FLUSHING_EVENT,
    }

    @SuppressWarnings("UnusedDeclaration")
    private enum FlushResult {
        SUCCESS,
        SERVER_ERROR,
        NO_CONNECTIVITY,
        UNKNOWN_ERROR
    }

    /**
     * Constructor is private, newLogger() methods should be used to build an instance.
     */
    private AppEventsLogger(Context context, String applicationId, Session session) {

        Validate.notNull(context, "context");
        this.context = context;

        if (session == null) {
            session = Session.getActiveSession();
        }

        // If we have a session and the appId passed is null or matches the session's app ID:
        if (session != null &&
                (applicationId == null || applicationId.equals(session.getApplicationId()))
                ) {
            accessTokenAppId = new AccessTokenAppIdPair(session);
        } else {
            // If no app ID passed, get it from the manifest:
            if (applicationId == null) {
                applicationId = Utility.getMetadataApplicationId(context);
            }
            accessTokenAppId = new AccessTokenAppIdPair(null, applicationId);
        }

        synchronized (staticLock) {

            if (hashedDeviceAndAppId == null) {
                hashedDeviceAndAppId = Utility.getHashedDeviceAndAppID(context, applicationId);
            }

            if (applicationContext == null) {
                applicationContext = context.getApplicationContext();
            }
        }

        initializeTimersIfNeeded();
    }

    private static void initializeTimersIfNeeded() {
        synchronized (staticLock) {
            if (backgroundExecutor != null) {
                return;
            }
            backgroundExecutor = new ScheduledThreadPoolExecutor(1);
        }

        final Runnable flushRunnable = new Runnable() {
            @Override
            public void run() {
                if (getFlushBehavior() != FlushBehavior.EXPLICIT_ONLY) {
                    flushAndWait(FlushReason.TIMER);
                }
            }
        };

        backgroundExecutor.scheduleAtFixedRate(
                flushRunnable,
                0,
                FLUSH_PERIOD_IN_SECONDS,
                TimeUnit.SECONDS
        );

        final Runnable attributionRecheckRunnable = new Runnable() {
            @Override
            public void run() {
                Set<String> applicationIds = new HashSet<String>();
                synchronized (staticLock) {
                    for (AccessTokenAppIdPair accessTokenAppId : stateMap.keySet()) {
                        applicationIds.add(accessTokenAppId.getApplicationId());
                    }
                }
                for (String applicationId : applicationIds) {
                    Utility.queryAppSettings(applicationId, true);
                }
            }
        };

        backgroundExecutor.scheduleAtFixedRate(
                attributionRecheckRunnable,
                0,
                APP_SUPPORTS_ATTRIBUTION_ID_RECHECK_PERIOD_IN_SECONDS,
                TimeUnit.SECONDS
        );
    }

    private void logEvent(String eventName, Double valueToSum, Bundle parameters, boolean isImplicitlyLogged) {
        AppEvent event = new AppEvent(this.context, eventName, valueToSum, parameters, isImplicitlyLogged);
        logEvent(context, event, accessTokenAppId);
    }

    private static void logEvent(final Context context,
                                 final AppEvent event,
                                 final AccessTokenAppIdPair accessTokenAppId) {
        Settings.getExecutor().execute(new Runnable() {
            @Override
            public void run() {
                SessionEventsState state = getSessionEventsState(context, accessTokenAppId);
                state.addEvent(event);
                flushIfNecessary();
            }
        });
    }

    static void eagerFlush() {
        if (getFlushBehavior() != FlushBehavior.EXPLICIT_ONLY) {
            flush(FlushReason.EAGER_FLUSHING_EVENT);
        }
    }

    private static void flushIfNecessary() {
        synchronized (staticLock) {
            if (getFlushBehavior() != FlushBehavior.EXPLICIT_ONLY) {
                if (getAccumulatedEventCount() > NUM_LOG_EVENTS_TO_TRY_TO_FLUSH_AFTER) {
                    flush(FlushReason.EVENT_THRESHOLD);
                }
            }
        }
    }

    private static int getAccumulatedEventCount() {
        synchronized (staticLock) {

            int result = 0;
            for (SessionEventsState state : stateMap.values()) {
                result += state.getAccumulatedEventCount();
            }
            return result;
        }
    }

    // Creates a new SessionEventsState if not already in the map.
    private static SessionEventsState getSessionEventsState(Context context, AccessTokenAppIdPair accessTokenAppId) {
        // Do this work outside of the lock to prevent deadlocks in implementation of
        //  AdvertisingIdClient.getAdvertisingIdInfo, because that implementation blocks waiting on the main thread,
        //  which may also grab this staticLock.
        SessionEventsState state = stateMap.get(accessTokenAppId);
        AttributionIdentifiers attributionIdentifiers = null;
        if (state == null) {
            // Retrieve attributionId, but we will only send it if attribution is supported for the app.
            attributionIdentifiers = AttributionIdentifiers.getAttributionIdentifiers(context);
        }

        synchronized (staticLock) {
            // Check state again while we're locked.
            state = stateMap.get(accessTokenAppId);
            if (state == null) {
                state = new SessionEventsState(attributionIdentifiers, context.getPackageName(), hashedDeviceAndAppId);
                stateMap.put(accessTokenAppId, state);
            }
            return state;
        }
    }

    private static SessionEventsState getSessionEventsState(AccessTokenAppIdPair accessTokenAppId) {
        synchronized (staticLock) {
            return stateMap.get(accessTokenAppId);
        }
    }

    private static void flush(final FlushReason reason) {

        Settings.getExecutor().execute(new Runnable() {
            @Override
            public void run() {
                flushAndWait(reason);
            }
        });
    }

    private static void flushAndWait(final FlushReason reason) {

        Set<AccessTokenAppIdPair> keysToFlush;
        synchronized (staticLock) {
            if (requestInFlight) {
                return;
            }
            requestInFlight = true;
            keysToFlush = new HashSet<AccessTokenAppIdPair>(stateMap.keySet());
        }

        accumulatePersistedEvents();

        FlushStatistics flushResults = null;
        try {
            flushResults = buildAndExecuteRequests(reason, keysToFlush);
        } catch (Exception e) {
            Log.d(TAG, "Caught unexpected exception while flushing: " + e.toString());
        }

        synchronized (staticLock) {
            requestInFlight = false;
        }

        if (flushResults != null) {
            final Intent intent = new Intent(ACTION_APP_EVENTS_FLUSHED);
            intent.putExtra(APP_EVENTS_EXTRA_NUM_EVENTS_FLUSHED, flushResults.numEvents);
            intent.putExtra(APP_EVENTS_EXTRA_FLUSH_RESULT, flushResults.result);
            LocalBroadcastManager.getInstance(applicationContext).sendBroadcast(intent);
        }
    }

    private static FlushStatistics buildAndExecuteRequests(FlushReason reason, Set<AccessTokenAppIdPair> keysToFlush) {
        FlushStatistics flushResults = new FlushStatistics();

        boolean limitEventUsage = Settings.getLimitEventAndDataUsage(applicationContext);

        List<Request> requestsToExecute = new ArrayList<Request>();
        for (AccessTokenAppIdPair accessTokenAppId : keysToFlush) {
            SessionEventsState sessionEventsState = getSessionEventsState(accessTokenAppId);
            if (sessionEventsState == null) {
                continue;
            }

            Request request = buildRequestForSession(accessTokenAppId, sessionEventsState, limitEventUsage,
                    flushResults);
            if (request != null) {
                requestsToExecute.add(request);
            }
        }

        if (requestsToExecute.size() > 0) {
            Logger.log(LoggingBehavior.APP_EVENTS, TAG, "Flushing %d events due to %s.",
                    flushResults.numEvents,
                    reason.toString());

            for (Request request : requestsToExecute) {
                // Execute the request synchronously. Callbacks will take care of handling errors and updating
                // our final overall result.
                request.executeAndWait();
            }
            return flushResults;
        }

        return null;
    }

    private static class FlushStatistics {
        public int numEvents = 0;
        public FlushResult result = FlushResult.SUCCESS;
    }

    private static Request buildRequestForSession(final AccessTokenAppIdPair accessTokenAppId,
                                                  final SessionEventsState sessionEventsState,
                                                  final boolean limitEventUsage,
                                                  final FlushStatistics flushState) {
        String applicationId = accessTokenAppId.getApplicationId();

        Utility.FetchedAppSettings fetchedAppSettings = Utility.queryAppSettings(applicationId, false);

        final Request postRequest = Request.newPostRequest(
                null,
                String.format("%s/activities", applicationId),
                null,
                null);

        Bundle requestParameters = postRequest.getParameters();
        if (requestParameters == null) {
            requestParameters = new Bundle();
        }
        requestParameters.putString("access_token", accessTokenAppId.getAccessToken());
        postRequest.setParameters(requestParameters);

        int numEvents = sessionEventsState.populateRequest(postRequest, fetchedAppSettings.supportsImplicitLogging(),
                fetchedAppSettings.supportsAttribution(), limitEventUsage);
        if (numEvents == 0) {
            return null;
        }

        flushState.numEvents += numEvents;

        postRequest.setCallback(new Request.Callback() {
            @Override
            public void onCompleted(Response response) {
                handleResponse(accessTokenAppId, postRequest, response, sessionEventsState, flushState);
            }
        });

        return postRequest;
    }

    private static void handleResponse(AccessTokenAppIdPair accessTokenAppId, Request request, Response response,
                                       SessionEventsState sessionEventsState, FlushStatistics flushState) {
        FacebookRequestError error = response.getError();
        String resultDescription = "Success";

        FlushResult flushResult = FlushResult.SUCCESS;

        if (error != null) {
            final int NO_CONNECTIVITY_ERROR_CODE = -1;
            if (error.getErrorCode() == NO_CONNECTIVITY_ERROR_CODE) {
                resultDescription = "Failed: No Connectivity";
                flushResult = FlushResult.NO_CONNECTIVITY;
            } else {
                resultDescription = String.format("Failed:\n  Response: %s\n  Error %s",
                        response.toString(),
                        error.toString());
                flushResult = FlushResult.SERVER_ERROR;
            }
        }

        if (Settings.isLoggingBehaviorEnabled(LoggingBehavior.APP_EVENTS)) {
            String eventsJsonString = (String) request.getTag();
            String prettyPrintedEvents;

            try {
                JSONArray jsonArray = new JSONArray(eventsJsonString);
                prettyPrintedEvents = jsonArray.toString(2);
            } catch (JSONException exc) {
                prettyPrintedEvents = "<Can't encode events for debug logging>";
            }

            Logger.log(LoggingBehavior.APP_EVENTS, TAG,
                    "Flush completed\nParams: %s\n  Result: %s\n  Events JSON: %s",
                    request.getGraphObject().toString(),
                    resultDescription,
                    prettyPrintedEvents);
        }

        sessionEventsState.clearInFlightAndStats(error != null);

        if (flushResult == FlushResult.NO_CONNECTIVITY) {
            // We may call this for multiple requests in a batch, which is slightly inefficient since in principle
            // we could call it once for all failed requests, but the impact is likely to be minimal.
            // We don't call this for other server errors, because if an event failed because it was malformed, etc.,
            // continually retrying it will cause subsequent events to not be logged either.
            PersistedEvents.persistEvents(applicationContext, accessTokenAppId, sessionEventsState);
        }

        if (flushResult != FlushResult.SUCCESS) {
            // We assume that connectivity issues are more significant to report than server issues.
            if (flushState.result != FlushResult.NO_CONNECTIVITY) {
                flushState.result = flushResult;
            }
        }
    }

    private static int accumulatePersistedEvents() {
        PersistedEvents persistedEvents = PersistedEvents.readAndClearStore(applicationContext);

        int result = 0;
        for (AccessTokenAppIdPair accessTokenAppId : persistedEvents.keySet()) {
            SessionEventsState sessionEventsState = getSessionEventsState(applicationContext, accessTokenAppId);

            List<AppEvent> events = persistedEvents.getEvents(accessTokenAppId);
            sessionEventsState.accumulatePersistedEvents(events);
            result += events.size();
        }

        return result;
    }

    /**
     * Invoke this method, rather than throwing an Exception, for situations where user/server input might reasonably
     * cause this to occur, and thus don't want an exception thrown at production time, but do want logging
     * notification.
     */
    private static void notifyDeveloperError(String message) {
        Logger.log(LoggingBehavior.DEVELOPER_ERRORS, "AppEvents", message);
    }

    /**
     * Source Application setters and getters
     */
    private static void setSourceApplication(Activity activity) {

        ComponentName callingApplication = activity.getCallingActivity();
        if (callingApplication != null) {
            String callingApplicationPackage = callingApplication.getPackageName();
            if (callingApplicationPackage.equals(activity.getPackageName())) {
                // open by own app.
                resetSourceApplication();
                return;
            }
            sourceApplication = callingApplicationPackage;
        }

        // Tap icon to open an app will still get the old intent if the activity was opened by an intent before.
        // Introduce an extra field in the intent to force clear the sourceApplication.
        Intent openIntent = activity.getIntent();
        if (openIntent == null || openIntent.getBooleanExtra(SOURCE_APPLICATION_HAS_BEEN_SET_BY_THIS_INTENT, false)) {
            resetSourceApplication();
            return;
        }

        Bundle applinkData = AppLinks.getAppLinkData(openIntent);

        if (applinkData == null) {
            resetSourceApplication();
            return;
        }

        isOpenedByApplink = true;

        Bundle applinkReferrerData = applinkData.getBundle("referer_app_link");

        if (applinkReferrerData == null) {
            sourceApplication = null;
            return;
        }

        String applinkReferrerPackage = applinkReferrerData.getString("package");
        sourceApplication = applinkReferrerPackage;

        // Mark this intent has been used to avoid use this intent again and again.
        openIntent.putExtra(SOURCE_APPLICATION_HAS_BEEN_SET_BY_THIS_INTENT, true);

        return;
    }

    static void setSourceApplication(String applicationPackage, boolean openByAppLink) {
        sourceApplication = applicationPackage;
        isOpenedByApplink = openByAppLink;
    }

    static String getSourceApplication() {
        String openType = "Unclassified";
        if (isOpenedByApplink) {
            openType = "Applink";
        }
        if (sourceApplication != null) {
            return openType + "(" + sourceApplication + ")";
        }
        return openType;
    }

    static void resetSourceApplication() {
        sourceApplication = null;
        isOpenedByApplink = false;
    }

    //
    // Deprecated Stuff
    //


    static class SessionEventsState {
        private List<AppEvent> accumulatedEvents = new ArrayList<AppEvent>();
        private List<AppEvent> inFlightEvents = new ArrayList<AppEvent>();
        private int numSkippedEventsDueToFullBuffer;
        private AttributionIdentifiers attributionIdentifiers;
        private String packageName;
        private String hashedDeviceAndAppId;

        public static final String EVENT_COUNT_KEY = "event_count";
        public static final String ENCODED_EVENTS_KEY = "encoded_events";
        public static final String NUM_SKIPPED_KEY = "num_skipped";

        private final int MAX_ACCUMULATED_LOG_EVENTS = 1000;

        public SessionEventsState(AttributionIdentifiers identifiers, String packageName, String hashedDeviceAndAppId) {
            this.attributionIdentifiers = identifiers;
            this.packageName = packageName;
            this.hashedDeviceAndAppId = hashedDeviceAndAppId;
        }

        // Synchronize here and in other methods on this class, because could be coming in from different
        // AppEventsLoggers on different threads pointing at the same session.
        public synchronized void addEvent(AppEvent event) {
            if (accumulatedEvents.size() + inFlightEvents.size() >= MAX_ACCUMULATED_LOG_EVENTS) {
                numSkippedEventsDueToFullBuffer++;
            } else {
                accumulatedEvents.add(event);
            }
        }

        public synchronized int getAccumulatedEventCount() {
            return accumulatedEvents.size();
        }

        public synchronized void clearInFlightAndStats(boolean moveToAccumulated) {
            if (moveToAccumulated) {
                accumulatedEvents.addAll(inFlightEvents);
            }
            inFlightEvents.clear();
            numSkippedEventsDueToFullBuffer = 0;
        }

        public int populateRequest(Request request, boolean includeImplicitEvents,
                                   boolean includeAttribution, boolean limitEventUsage) {

            int numSkipped;
            JSONArray jsonArray;
            synchronized (this) {
                numSkipped = numSkippedEventsDueToFullBuffer;

                // move all accumulated events to inFlight.
                inFlightEvents.addAll(accumulatedEvents);
                accumulatedEvents.clear();

                jsonArray = new JSONArray();
                for (AppEvent event : inFlightEvents) {
                    if (includeImplicitEvents || !event.getIsImplicit()) {
                        jsonArray.put(event.getJSONObject());
                    }
                }

                if (jsonArray.length() == 0) {
                    return 0;
                }
            }

            populateRequest(request, numSkipped, jsonArray, includeAttribution, limitEventUsage);
            return jsonArray.length();
        }

        public synchronized List<AppEvent> getEventsToPersist() {
            // We will only persist accumulated events, not ones currently in-flight. This means if an in-flight
            // request fails, those requests will not be persisted and thus might be lost if the process terminates
            // while the flush is in progress.
            List<AppEvent> result = accumulatedEvents;
            accumulatedEvents = new ArrayList<AppEvent>();
            return result;
        }

        public synchronized void accumulatePersistedEvents(List<AppEvent> events) {
            // We won't skip events due to a full buffer, since we already accumulated them once and persisted
            // them. But they will count against the buffer size when further events are accumulated.
            accumulatedEvents.addAll(events);
        }

        private void populateRequest(Request request, int numSkipped, JSONArray events, boolean includeAttribution,
                                     boolean limitEventUsage) {
            GraphObject publishParams = GraphObject.Factory.create();
            publishParams.setProperty("event", "CUSTOM_APP_EVENTS");

            if (numSkippedEventsDueToFullBuffer > 0) {
                publishParams.setProperty("num_skipped_events", numSkipped);
            }

            if (includeAttribution) {
                Utility.setAppEventAttributionParameters(publishParams, attributionIdentifiers,
                        hashedDeviceAndAppId, limitEventUsage);
            }

            // The code to get all the Extended info is safe but just in case we can wrap the whole
            // call in its own try/catch block since some of the things it does might cause
            // unexpected exceptions on rooted/funky devices:
            try {
                Utility.setAppEventExtendedDeviceInfoParameters(publishParams, applicationContext);
            } catch (Exception e) {
                // Swallow
            }

            publishParams.setProperty("application_package_name", packageName);

            request.setGraphObject(publishParams);

            Bundle requestParameters = request.getParameters();
            if (requestParameters == null) {
                requestParameters = new Bundle();
            }

            String jsonString = events.toString();
            if (jsonString != null) {
                requestParameters.putByteArray("custom_events_file", getStringAsByteArray(jsonString));
                request.setTag(jsonString);
            }
            request.setParameters(requestParameters);
        }

        private byte[] getStringAsByteArray(String jsonString) {
            byte[] jsonUtf8 = null;
            try {
                jsonUtf8 = jsonString.getBytes("UTF-8");
            } catch (UnsupportedEncodingException e) {
                // shouldn't happen, but just in case:
                Utility.logd("Encoding exception: ", e);
            }
            return jsonUtf8;
        }
    }

    static class AppEvent implements Serializable {
        private static final long serialVersionUID = 1L;

        private JSONObject jsonObject;
        private boolean isImplicit;
        private static final HashSet<String> validatedIdentifiers = new HashSet<String>();
        private String name;

        public AppEvent(
                Context context,
                String eventName,
                Double valueToSum,
                Bundle parameters,
                boolean isImplicitlyLogged
        ) {

            validateIdentifier(eventName);

            this.name = eventName;

            isImplicit = isImplicitlyLogged;
            jsonObject = new JSONObject();

            try {
                jsonObject.put("_eventName", eventName);
                jsonObject.put("_logTime", System.currentTimeMillis() / 1000);
                jsonObject.put("_ui", Utility.getActivityName(context));

                if (valueToSum != null) {
                    jsonObject.put("_valueToSum", valueToSum.doubleValue());
                }

                if (isImplicit) {
                    jsonObject.put("_implicitlyLogged", "1");
                }

                String appVersion = Settings.getAppVersion();
                if (appVersion != null) {
                    jsonObject.put("_appVersion", appVersion);
                }

                if (parameters != null) {
                    for (String key : parameters.keySet()) {

                        validateIdentifier(key);

                        Object value = parameters.get(key);
                        if (!(value instanceof String) && !(value instanceof Number)) {
                            throw new FacebookException(
                                    String.format(
                                            "Parameter value '%s' for key '%s' should be a string or a numeric type.",
                                            value,
                                            key)
                            );
                        }

                        jsonObject.put(key, value.toString());
                    }
                }

                if (!isImplicit) {
                    Logger.log(LoggingBehavior.APP_EVENTS, "AppEvents",
                            "Created app event '%s'", jsonObject.toString());
                }
            } catch (JSONException jsonException) {

                // If any of the above failed, just consider this an illegal event.
                Logger.log(LoggingBehavior.APP_EVENTS, "AppEvents",
                        "JSON encoding for app event failed: '%s'", jsonException.toString());
                jsonObject = null;

            }
        }

        public String getName() {
            return name;
        }

        private AppEvent(String jsonString, boolean isImplicit) throws JSONException {
            jsonObject = new JSONObject(jsonString);
            this.isImplicit = isImplicit;
        }

        public boolean getIsImplicit() {
            return isImplicit;
        }

        public JSONObject getJSONObject() {
            return jsonObject;
        }

        // throw exception if not valid.
        private void validateIdentifier(String identifier) {

            // Identifier should be 40 chars or less, and only have 0-9A-Za-z, underscore, hyphen, and space (but no
            // hyphen or space in the first position).
            final String regex = "^[0-9a-zA-Z_]+[0-9a-zA-Z _-]*$";

            final int MAX_IDENTIFIER_LENGTH = 40;
            if (identifier == null || identifier.length() == 0 || identifier.length() > MAX_IDENTIFIER_LENGTH) {
                if (identifier == null) {
                    identifier = "<None Provided>";
                }
                throw new FacebookException(
                    String.format("Identifier '%s' must be less than %d characters", identifier, MAX_IDENTIFIER_LENGTH)
                );
            }

            boolean alreadyValidated = false;
            synchronized (validatedIdentifiers) {
                alreadyValidated = validatedIdentifiers.contains(identifier);
            }

            if (!alreadyValidated) {
                if (identifier.matches(regex)) {
                    synchronized (validatedIdentifiers) {
                        validatedIdentifiers.add(identifier);
                    }
                } else {
                    throw new FacebookException(
                            String.format("Skipping event named '%s' due to illegal name - must be under 40 chars " +
                                            "and alphanumeric, _, - or space, and not start with a space or hyphen.",
                                    identifier
                            )
                    );
                }
            }

        }

        private static class SerializationProxyV1 implements Serializable {
            private static final long serialVersionUID = -2488473066578201069L;
            private final String jsonString;
            private final boolean isImplicit;

            private SerializationProxyV1(String jsonString, boolean isImplicit) {
                this.jsonString = jsonString;
                this.isImplicit = isImplicit;
            }

            private Object readResolve() throws JSONException {
                return new AppEvent(jsonString, isImplicit);
            }
        }

        private Object writeReplace() {
            return new SerializationProxyV1(jsonObject.toString(), isImplicit);
        }

        @Override
        public String toString() {
            return String.format("\"%s\", implicit: %b, json: %s", jsonObject.optString("_eventName"),
                    isImplicit, jsonObject.toString());
        }
    }

    static class PersistedAppSessionInfo {
        private static final String PERSISTED_SESSION_INFO_FILENAME =
                "AppEventsLogger.persistedsessioninfo";

        private static final Object staticLock = new Object();
        private static boolean hasChanges = false;
        private static boolean isLoaded = false;
        private static Map<AccessTokenAppIdPair, FacebookTimeSpentData> appSessionInfoMap;

        private static final Runnable appSessionInfoFlushRunnable = new Runnable() {
            @Override
            public void run() {
                PersistedAppSessionInfo.saveAppSessionInformation(applicationContext);
            }
        };

        @SuppressWarnings("unchecked")
        private static void restoreAppSessionInformation(Context context) {
            ObjectInputStream ois = null;

            synchronized (staticLock) {
                if (!isLoaded) {
                    try {
                        ois =
                                new ObjectInputStream(
                                        context.openFileInput(PERSISTED_SESSION_INFO_FILENAME));
                        appSessionInfoMap =
                                (HashMap<AccessTokenAppIdPair, FacebookTimeSpentData>) ois.readObject();
                        Logger.log(
                                LoggingBehavior.APP_EVENTS,
                                "AppEvents",
                                "App session info loaded");
                    } catch (FileNotFoundException fex) {
                    } catch (Exception e) {
                        Log.d(TAG, "Got unexpected exception: " + e.toString());
                    } finally {
                        Utility.closeQuietly(ois);
                        context.deleteFile(PERSISTED_SESSION_INFO_FILENAME);
                        if (appSessionInfoMap == null) {
                            appSessionInfoMap =
                                    new HashMap<AccessTokenAppIdPair, FacebookTimeSpentData>();
                        }
                        // Regardless of the outcome of the load, the session information cache
                        // is always deleted. Therefore, always treat the session information cache
                        // as loaded
                        isLoaded = true;
                        hasChanges = false;
                    }
                }
            }
        }

        static void saveAppSessionInformation(Context context) {
            ObjectOutputStream oos = null;

            synchronized (staticLock) {
                if (hasChanges) {
                    try {
                        oos = new ObjectOutputStream(
                                new BufferedOutputStream(
                                        context.openFileOutput(
                                                PERSISTED_SESSION_INFO_FILENAME,
                                                Context.MODE_PRIVATE)
                                )
                        );
                        oos.writeObject(appSessionInfoMap);
                        hasChanges = false;
                        Logger.log(LoggingBehavior.APP_EVENTS, "AppEvents", "App session info saved");
                    } catch (Exception e) {
                        Log.d(TAG, "Got unexpected exception: " + e.toString());
                    } finally {
                        Utility.closeQuietly(oos);
                    }
                }
            }
        }

        static void onResume(
                Context context,
                AccessTokenAppIdPair accessTokenAppId,
                AppEventsLogger logger,
                long eventTime,
                String sourceApplicationInfo
        ) {
            synchronized (staticLock) {
                FacebookTimeSpentData timeSpentData = getTimeSpentData(context, accessTokenAppId);
                timeSpentData.onResume(logger, eventTime, sourceApplicationInfo);
                onTimeSpentDataUpdate();
            }
        }

        static void onSuspend(
                Context context,
                AccessTokenAppIdPair accessTokenAppId,
                AppEventsLogger logger,
                long eventTime
        ) {
            synchronized (staticLock) {
                FacebookTimeSpentData timeSpentData = getTimeSpentData(context, accessTokenAppId);
                timeSpentData.onSuspend(logger, eventTime);
                onTimeSpentDataUpdate();
            }
        }

        private static FacebookTimeSpentData getTimeSpentData(
                Context context,
                AccessTokenAppIdPair accessTokenAppId
        ) {
            restoreAppSessionInformation(context);
            FacebookTimeSpentData result = null;

            result = appSessionInfoMap.get(accessTokenAppId);
            if (result == null) {
                result = new FacebookTimeSpentData();
                appSessionInfoMap.put(accessTokenAppId, result);
            }

            return result;
        }

        private static void onTimeSpentDataUpdate() {
            if (!hasChanges) {
                hasChanges = true;
                backgroundExecutor.schedule(
                        appSessionInfoFlushRunnable,
                        FLUSH_APP_SESSION_INFO_IN_SECONDS,
                        TimeUnit.SECONDS);
            }
        }
    }

    // Read/write operations are thread-safe/atomic across all instances of PersistedEvents, but modifications
    // to any individual instance are not thread-safe.
    static class PersistedEvents {
        static final String PERSISTED_EVENTS_FILENAME = "AppEventsLogger.persistedevents";

        private static Object staticLock = new Object();

        private Context context;
        private HashMap<AccessTokenAppIdPair, List<AppEvent>> persistedEvents =
                new HashMap<AccessTokenAppIdPair, List<AppEvent>>();

        private PersistedEvents(Context context) {
            this.context = context;
        }

        public static PersistedEvents readAndClearStore(Context context) {
            synchronized (staticLock) {
                PersistedEvents persistedEvents = new PersistedEvents(context);

                persistedEvents.readAndClearStore();

                return persistedEvents;
            }
        }

        public static void persistEvents(Context context, AccessTokenAppIdPair accessTokenAppId,
                                         SessionEventsState eventsToPersist) {
            Map<AccessTokenAppIdPair, SessionEventsState> map = new HashMap<AccessTokenAppIdPair, SessionEventsState>();
            map.put(accessTokenAppId, eventsToPersist);
            persistEvents(context, map);
        }

        public static void persistEvents(Context context,
                                         Map<AccessTokenAppIdPair, SessionEventsState> eventsToPersist) {
            synchronized (staticLock) {
                // Note that we don't track which instance of AppEventsLogger added a particular event to
                // SessionEventsState; when a particular Context is being destroyed, we'll persist all accumulated
                // events. More sophisticated tracking could be done to try to reduce unnecessary persisting of events,
                // but the overall number of events is not expected to be large.
                PersistedEvents persistedEvents = readAndClearStore(context);

                for (Map.Entry<AccessTokenAppIdPair, SessionEventsState> entry : eventsToPersist.entrySet()) {
                    List<AppEvent> events = entry.getValue().getEventsToPersist();
                    if (events.size() == 0) {
                        continue;
                    }

                    persistedEvents.addEvents(entry.getKey(), events);
                }

                persistedEvents.write();
            }
        }

        public Set<AccessTokenAppIdPair> keySet() {
            return persistedEvents.keySet();
        }

        public List<AppEvent> getEvents(AccessTokenAppIdPair accessTokenAppId) {
            return persistedEvents.get(accessTokenAppId);
        }

        private void write() {
            ObjectOutputStream oos = null;
            try {
                oos = new ObjectOutputStream(
                        new BufferedOutputStream(context.openFileOutput(PERSISTED_EVENTS_FILENAME, 0)));
                oos.writeObject(persistedEvents);
            } catch (Exception e) {
                Log.d(TAG, "Got unexpected exception: " + e.toString());
            } finally {
                Utility.closeQuietly(oos);
            }
        }

        private void readAndClearStore() {
            ObjectInputStream ois = null;
            try {
                ois = new ObjectInputStream(
                        new BufferedInputStream(context.openFileInput(PERSISTED_EVENTS_FILENAME)));

                @SuppressWarnings("unchecked")
                HashMap<AccessTokenAppIdPair, List<AppEvent>> obj =
                        (HashMap<AccessTokenAppIdPair, List<AppEvent>>) ois.readObject();

                // Note: We delete the store before we store the events; this means we'd prefer to lose some
                // events in the case of exception rather than potentially log them twice.
                context.getFileStreamPath(PERSISTED_EVENTS_FILENAME).delete();
                persistedEvents = obj;
            } catch (FileNotFoundException e) {
                // Expected if we never persisted any events.
            } catch (Exception e) {
                Log.d(TAG, "Got unexpected exception: " + e.toString());
            } finally {
                Utility.closeQuietly(ois);
            }
        }

        public void addEvents(AccessTokenAppIdPair accessTokenAppId, List<AppEvent> eventsToPersist) {
            if (!persistedEvents.containsKey(accessTokenAppId)) {
                persistedEvents.put(accessTokenAppId, new ArrayList<AppEvent>());
            }
            persistedEvents.get(accessTokenAppId).addAll(eventsToPersist);
        }
    }
}
