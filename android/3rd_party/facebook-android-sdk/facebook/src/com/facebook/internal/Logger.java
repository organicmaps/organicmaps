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

import android.util.Log;
import com.facebook.LoggingBehavior;
import com.facebook.Settings;

import java.util.HashMap;
import java.util.Map;

/**
 * com.facebook.internal is solely for the use of other packages within the Facebook SDK for Android. Use of
 * any of the classes in this package is unsupported, and they may be modified or removed without warning at
 * any time.
 */
public class Logger {
    public static final String LOG_TAG_BASE = "FacebookSDK.";
    private static final HashMap<String, String> stringsToReplace = new HashMap<String, String>();

    private final LoggingBehavior behavior;
    private final String tag;
    private StringBuilder contents;
    private int priority = Log.DEBUG;

    // Note that the mapping of replaced strings is never emptied, so it should be used only for things that
    // are not expected to be too numerous, such as access tokens.
    public synchronized static void registerStringToReplace(String original, String replace) {
        stringsToReplace.put(original, replace);
    }

    public synchronized static void registerAccessToken(String accessToken) {
        if (Settings.isLoggingBehaviorEnabled(LoggingBehavior.INCLUDE_ACCESS_TOKENS) == false) {
            registerStringToReplace(accessToken, "ACCESS_TOKEN_REMOVED");
        }
    }

    public static void log(LoggingBehavior behavior, String tag, String string) {
        log(behavior, Log.DEBUG, tag, string);
    }

    public static void log(LoggingBehavior behavior, String tag, String format, Object... args) {
        if (Settings.isLoggingBehaviorEnabled(behavior)) {
            String string = String.format(format, args);
            log(behavior, Log.DEBUG, tag, string);
        }
    }

    public static void log(LoggingBehavior behavior, int priority, String tag, String string) {
        if (Settings.isLoggingBehaviorEnabled(behavior)) {
            string = replaceStrings(string);
            if (tag.startsWith(LOG_TAG_BASE) == false) {
                tag = LOG_TAG_BASE + tag;
            }
            Log.println(priority, tag, string);

            // Developer errors warrant special treatment by printing out a stack trace, to make both more noticeable,
            // and let the source of the problem be more easily pinpointed.
            if (behavior == LoggingBehavior.DEVELOPER_ERRORS) {
                (new Exception()).printStackTrace();
            }
        }
    }

    private synchronized static String replaceStrings(String string) {
        for (Map.Entry<String, String> entry : stringsToReplace.entrySet()) {
            string = string.replace(entry.getKey(), entry.getValue());
        }
        return string;
    }

    public Logger(LoggingBehavior behavior, String tag) {
        Validate.notNullOrEmpty(tag, "tag");

        this.behavior = behavior;
        this.tag = LOG_TAG_BASE + tag;
        this.contents = new StringBuilder();
    }

    public int getPriority() {
        return priority;
    }

    public void setPriority(int value) {
        Validate.oneOf(value, "value", Log.ASSERT, Log.DEBUG, Log.ERROR, Log.INFO, Log.VERBOSE, Log.WARN);

        priority = value;
    }

    public String getContents() {
        return replaceStrings(contents.toString());
    }

    // Writes the accumulated contents, then clears contents to start again.
    public void log() {
        logString(contents.toString());
        contents = new StringBuilder();
    }

    // Immediately logs a string, ignoring any accumulated contents, which are left unchanged.
    public void logString(String string) {
        log(behavior, priority, tag, string);
    }

    public void append(StringBuilder stringBuilder) {
        if (shouldLog()) {
            contents.append(stringBuilder);
        }
    }

    public void append(String string) {
        if (shouldLog()) {
            contents.append(string);
        }
    }

    public void append(String format, Object... args) {
        if (shouldLog()) {
            contents.append(String.format(format, args));
        }
    }

    public void appendKeyValue(String key, Object value) {
        append("  %s:\t%s\n", key, value);
    }

    private boolean shouldLog() {
        return Settings.isLoggingBehaviorEnabled(behavior);
    }
}
