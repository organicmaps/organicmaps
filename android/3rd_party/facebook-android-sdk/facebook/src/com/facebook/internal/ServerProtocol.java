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

import com.facebook.Settings;
import com.facebook.internal.Utility;

import java.util.Collection;

/**
 * com.facebook.internal is solely for the use of other packages within the Facebook SDK for Android. Use of
 * any of the classes in this package is unsupported, and they may be modified or removed without warning at
 * any time.
 */
public final class ServerProtocol {
    private static final String DIALOG_AUTHORITY_FORMAT = "m.%s";
    public static final String DIALOG_PATH = "dialog/";
    public static final String DIALOG_PARAM_SCOPE = "scope";
    public static final String DIALOG_PARAM_CLIENT_ID = "client_id";
    public static final String DIALOG_PARAM_DISPLAY = "display";
    public static final String DIALOG_PARAM_REDIRECT_URI = "redirect_uri";
    public static final String DIALOG_PARAM_TYPE = "type";
    public static final String DIALOG_PARAM_ACCESS_TOKEN = "access_token";
    public static final String DIALOG_PARAM_APP_ID = "app_id";
    public static final String DIALOG_PARAM_E2E = "e2e";

    // URL components
    private static final String GRAPH_URL_FORMAT = "https://graph.%s";
    private static final String REST_URL_FORMAT = "https://api.%s/method";
    public static final String BATCHED_REST_METHOD_URL_BASE = "method/";

    public static final Collection<String> errorsProxyAuthDisabled =
            Utility.unmodifiableCollection("service_disabled", "AndroidAuthKillSwitchException");
    public static final Collection<String> errorsUserCanceled =
            Utility.unmodifiableCollection("access_denied", "OAuthAccessDeniedException");

    public static final String getDialogAuthority() {
        return String.format(DIALOG_AUTHORITY_FORMAT, Settings.getFacebookDomain());
    }

    public static final String getGraphUrlBase() {
        return String.format(GRAPH_URL_FORMAT, Settings.getFacebookDomain());
    }

    public static final String getRestUrlBase() {
        return String.format(REST_URL_FORMAT, Settings.getFacebookDomain());
    }
}
