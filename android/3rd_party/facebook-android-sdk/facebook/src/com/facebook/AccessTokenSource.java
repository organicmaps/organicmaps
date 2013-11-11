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
 * Indicates where a Facebook access token was obtained from.
 */
public enum AccessTokenSource {
    /**
     * Indicates an access token has not been obtained, or is otherwise invalid.
     */
    NONE(false),
    /**
     * Indicates an access token was obtained by the user logging in through the
     * Facebook app for Android using the web login dialog.
     */
    FACEBOOK_APPLICATION_WEB(true),
    /**
     * Indicates an access token was obtained by the user logging in through the
     * Facebook app for Android using the native login dialog.
     */
    FACEBOOK_APPLICATION_NATIVE(true),
    /**
     * Indicates an access token was obtained by asking the Facebook app for the
     * current token based on permissions the user has already granted to the app.
     * No dialog was shown to the user in this case.
     */
    FACEBOOK_APPLICATION_SERVICE(true),
    /**
     * Indicates an access token was obtained by the user logging in through the
     * Web-based dialog.
     */
    WEB_VIEW(false),
    /**
     * Indicates an access token is for a test user rather than an actual
     * Facebook user.
     */
    TEST_USER(true),
    /**
     * Indicates an access token constructed with a Client Token.
     */
    CLIENT_TOKEN(true);

    private final boolean canExtendToken;

    AccessTokenSource(boolean canExtendToken) {
        this.canExtendToken = canExtendToken;
    }

    boolean canExtendToken() {
        return canExtendToken;
    }
}
