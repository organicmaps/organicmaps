package com.facebook;

import android.os.Bundle;
import android.text.format.DateUtils;

import com.facebook.internal.Logger;

import java.io.Serializable;

class FacebookTimeSpentData implements Serializable {
    // Constants
    private static final long serialVersionUID = 1L;
    private static final String TAG = AppEventsLogger.class.getCanonicalName();
    private static final long FIRST_TIME_LOAD_RESUME_TIME = -1;
    private static final long INTERRUPTION_THRESHOLD_MILLISECONDS = 1000;
    private static final long NUM_MILLISECONDS_IDLE_TO_BE_NEW_SESSION =
            60 * DateUtils.SECOND_IN_MILLIS;
    private static final long APP_ACTIVATE_SUPPRESSION_PERIOD_IN_MILLISECONDS =
            5 * DateUtils.MINUTE_IN_MILLIS;

    // Should be kept in sync with the iOS sdk
    private static final long[] INACTIVE_SECONDS_QUANTA =
        new long[] {
            5 * DateUtils.MINUTE_IN_MILLIS,
            15 * DateUtils.MINUTE_IN_MILLIS,
            30 * DateUtils.MINUTE_IN_MILLIS,
            1 * DateUtils.HOUR_IN_MILLIS,
            6 * DateUtils.HOUR_IN_MILLIS,
            12 * DateUtils.HOUR_IN_MILLIS,
            1 * DateUtils.DAY_IN_MILLIS,
            2 * DateUtils.DAY_IN_MILLIS,
            3 * DateUtils.DAY_IN_MILLIS,
            7 * DateUtils.DAY_IN_MILLIS,
            14 * DateUtils.DAY_IN_MILLIS,
            21 * DateUtils.DAY_IN_MILLIS,
            28 * DateUtils.DAY_IN_MILLIS,
            60 * DateUtils.DAY_IN_MILLIS,
            90 * DateUtils.DAY_IN_MILLIS,
            120 * DateUtils.DAY_IN_MILLIS,
            150 * DateUtils.DAY_IN_MILLIS,
            180 * DateUtils.DAY_IN_MILLIS,
            365 * DateUtils.DAY_IN_MILLIS,
        };

    private boolean isWarmLaunch;
    private boolean isAppActive;
    private long lastActivateEventLoggedTime;

    // Member data that's persisted to disk
    private long lastResumeTime;
    private long lastSuspendTime;
    private long millisecondsSpentInSession;
    private int interruptionCount;
    private String firstOpenSourceApplication;

    /**
     * Serialization proxy for the FacebookTimeSpentData class. This is version 1 of
     * serialization. Future serializations may differ in format. This
     * class should not be modified. If serializations formats change,
     * create a new class SerializationProxyVx.
     */
    private static class SerializationProxyV1 implements Serializable {
        private static final long serialVersionUID = 6L;

        private final long lastResumeTime;
        private final long lastSuspendTime;
        private final long millisecondsSpentInSession;
        private final int interruptionCount;

        SerializationProxyV1(
            long lastResumeTime,
            long lastSuspendTime,
            long millisecondsSpentInSession,
            int interruptionCount
        ) {
            this.lastResumeTime = lastResumeTime;
            this.lastSuspendTime = lastSuspendTime;
            this.millisecondsSpentInSession = millisecondsSpentInSession;
            this.interruptionCount = interruptionCount;
        }

        private Object readResolve() {
            return new FacebookTimeSpentData(
                lastResumeTime,
                lastSuspendTime,
                millisecondsSpentInSession,
                interruptionCount);
        }
    }


    /**
     * Constructor to be used for V1 serialization only, DO NOT CHANGE.
     */
    private FacebookTimeSpentData(
            long lastResumeTime,
            long lastSuspendTime,
            long millisecondsSpentInSession,
            int interruptionCount

    ) {
        resetSession();
        this.lastResumeTime = lastResumeTime;
        this.lastSuspendTime = lastSuspendTime;
        this.millisecondsSpentInSession = millisecondsSpentInSession;
        this.interruptionCount = interruptionCount;
    }

    /**
     * Serialization proxy for the FacebookTimeSpentData class. This is version 2 of
     * serialization. Future serializations may differ in format. This
     * class should not be modified. If serializations formats change,
     * create a new class SerializationProxyVx.
     */
    private static class SerializationProxyV2 implements Serializable {
        private static final long serialVersionUID = 6L;

        private final long lastResumeTime;
        private final long lastSuspendTime;
        private final long millisecondsSpentInSession;
        private final int interruptionCount;
        private final String firstOpenSourceApplication;

        SerializationProxyV2(
                long lastResumeTime,
                long lastSuspendTime,
                long millisecondsSpentInSession,
                int interruptionCount,
                String firstOpenSourceApplication

        ) {
            this.lastResumeTime = lastResumeTime;
            this.lastSuspendTime = lastSuspendTime;
            this.millisecondsSpentInSession = millisecondsSpentInSession;
            this.interruptionCount = interruptionCount;
            this.firstOpenSourceApplication = firstOpenSourceApplication;
        }

        private Object readResolve() {
            return new FacebookTimeSpentData(
                    lastResumeTime,
                    lastSuspendTime,
                    millisecondsSpentInSession,
                    interruptionCount,
                    firstOpenSourceApplication);
        }
    }

    FacebookTimeSpentData() {
        resetSession();
    }

    /**
     * Constructor to be used for V2 serialization only, DO NOT CHANGE.
     */
    private FacebookTimeSpentData(
        long lastResumeTime,
        long lastSuspendTime,
        long millisecondsSpentInSession,
        int interruptionCount,
        String firstOpenSourceApplication
    ) {
        resetSession();
        this.lastResumeTime = lastResumeTime;
        this.lastSuspendTime = lastSuspendTime;
        this.millisecondsSpentInSession = millisecondsSpentInSession;
        this.interruptionCount = interruptionCount;
        this.firstOpenSourceApplication = firstOpenSourceApplication;
    }

    private Object writeReplace() {
        return new SerializationProxyV2(
                lastResumeTime,
                lastSuspendTime,
                millisecondsSpentInSession,
                interruptionCount,
                firstOpenSourceApplication
        );
    }

    void onSuspend(AppEventsLogger logger, long eventTime) {
        if (!isAppActive) {
            Logger.log(LoggingBehavior.APP_EVENTS, TAG, "Suspend for inactive app");
            return;
        }

        long now = eventTime;
        long delta = (now - lastResumeTime);
        if (delta < 0) {
            Logger.log(LoggingBehavior.APP_EVENTS, TAG, "Clock skew detected");
            delta = 0;
        }
        millisecondsSpentInSession += delta;
        lastSuspendTime = now;
        isAppActive = false;
    }

    void onResume(AppEventsLogger logger, long eventTime, String sourceApplicationInfo) {
        long now = eventTime;

        // Retain old behavior for activated app event - log the event if the event hasn't
        // been logged in the previous suppression interval or this is a cold launch.
        // If this is a cold launch, always log the event. Otherwise, use the last
        // event log time to determine if the app activate should be suppressed or not.
        if (isColdLaunch() ||
            ((now - lastActivateEventLoggedTime) > APP_ACTIVATE_SUPPRESSION_PERIOD_IN_MILLISECONDS)) {
            Bundle eventParams = new Bundle();
            eventParams.putString(
                    AppEventsConstants.EVENT_PARAM_SOURCE_APPLICATION,
                    sourceApplicationInfo);
            logger.logEvent(AppEventsConstants.EVENT_NAME_ACTIVATED_APP, eventParams);
            lastActivateEventLoggedTime = now;
        }

        // If this is an application that's not calling onSuspend yet, log and return. We can't
        // track time spent for this application as there are no calls to onSuspend.
        if (isAppActive) {
          Logger.log(LoggingBehavior.APP_EVENTS, TAG, "Resume for active app");
          return;
        }

        long interruptionDurationMillis = wasSuspendedEver() ? now - lastSuspendTime : 0;
        if (interruptionDurationMillis < 0) {
          Logger.log(LoggingBehavior.APP_EVENTS, TAG, "Clock skew detected");
          interruptionDurationMillis = 0;
        }

        // If interruption duration is > new session threshold, then log old session
        // event and start a new session.
        if (interruptionDurationMillis > NUM_MILLISECONDS_IDLE_TO_BE_NEW_SESSION) {
            logAppDeactivatedEvent(logger, interruptionDurationMillis);
        } else {
            // We're not logging this resume event - check to see if this should count
            // as an interruption
            if (interruptionDurationMillis > INTERRUPTION_THRESHOLD_MILLISECONDS) {
                interruptionCount++;
            }
        }

        // Set source application only for the first resume of the timespent session.
        if (interruptionCount == 0) {
            firstOpenSourceApplication = sourceApplicationInfo;
        }

        lastResumeTime = now;
        isAppActive = true;
    }

    private void logAppDeactivatedEvent(AppEventsLogger logger,
                                        long interruptionDurationMillis) {
        // Log the old session information and clear the data
        Bundle eventParams = new Bundle();
        eventParams.putInt(
                AppEventsConstants.EVENT_NAME_SESSION_INTERRUPTIONS,
                interruptionCount);
        eventParams.putInt(
                AppEventsConstants.EVENT_NAME_TIME_BETWEEN_SESSIONS,
                getQuantaIndex(interruptionDurationMillis));
        eventParams.putString(
                AppEventsConstants.EVENT_PARAM_SOURCE_APPLICATION,
                firstOpenSourceApplication);
        logger.logEvent(
                AppEventsConstants.EVENT_NAME_DEACTIVATED_APP,
                (millisecondsSpentInSession/DateUtils.SECOND_IN_MILLIS),
                eventParams);
        resetSession();
    }

    private static int getQuantaIndex(long timeBetweenSessions) {
        int quantaIndex = 0;

        while (
            quantaIndex < INACTIVE_SECONDS_QUANTA.length &&
            INACTIVE_SECONDS_QUANTA[quantaIndex] < timeBetweenSessions
        ) {
            ++quantaIndex;
        }

        return quantaIndex;
    }

    private void resetSession() {
        isAppActive = false;
        lastResumeTime = FIRST_TIME_LOAD_RESUME_TIME;
        lastSuspendTime = FIRST_TIME_LOAD_RESUME_TIME;
        interruptionCount = 0;
        millisecondsSpentInSession = 0;
    }

    private boolean wasSuspendedEver() {
        return lastSuspendTime != FIRST_TIME_LOAD_RESUME_TIME;
    }

    private boolean isColdLaunch() {
        // On the very first call in the process lifecycle, this will always
        // return true. After that, it will always return false.
        boolean result = !isWarmLaunch;
        isWarmLaunch = true;
        return result;
    }
}
