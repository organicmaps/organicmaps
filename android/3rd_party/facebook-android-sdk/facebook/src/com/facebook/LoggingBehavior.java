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

/**
 * Specifies different categories of logging messages that can be generated.
 *
 * @see Settings#addLoggingBehavior(LoggingBehavior)
 */
public enum LoggingBehavior {
    /**
     * Indicates that HTTP requests and a summary of responses should be logged.
     */
    REQUESTS,
    /**
     * Indicates that access tokens should be logged as part of the request logging; normally they are not.
     */
    INCLUDE_ACCESS_TOKENS,
    /**
     * Indicates that the entire raw HTTP response for each request should be logged.
     */
    INCLUDE_RAW_RESPONSES,
    /**
     * Indicates that cache operations should be logged.
     */
    CACHE,
    /**
     * Indicates the App Events-related operations should be logged.
     */
    APP_EVENTS,
    /**
     * Indicates that likely developer errors should be logged.  (This is set by default in LoggingBehavior.)
     */
    DEVELOPER_ERRORS
    ;

    @Deprecated
    public static final LoggingBehavior INSIGHTS = APP_EVENTS;
}
