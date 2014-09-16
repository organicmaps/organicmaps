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

import com.facebook.internal.NativeProtocol;

/**
 * Certain operations such as publishing a status or publishing a photo require an audience. When the user
 * grants an application permission to perform a publish operation, a default audience is selected as the
 * publication ceiling for the application. This enumerated value allows the application to select which
 * audience to ask the user to grant publish permission for.
 */
public enum SessionDefaultAudience {
    /**
     * Represents an invalid default audience value, can be used when only reading.
     */
    NONE(null),

    /**
     * Indicates only the user is able to see posts made by the application.
     */
    ONLY_ME(NativeProtocol.AUDIENCE_ME),

    /**
     * Indicates that the user's friends are able to see posts made by the application.
     */
    FRIENDS(NativeProtocol.AUDIENCE_FRIENDS),

    /**
     * Indicates that all Facebook users are able to see posts made by the application.
     */
    EVERYONE(NativeProtocol.AUDIENCE_EVERYONE);

    private final String nativeProtocolAudience;

    private SessionDefaultAudience(String protocol) {
        nativeProtocolAudience = protocol;
    }

    public String getNativeProtocolAudience() {
        return nativeProtocolAudience;
    }
}
