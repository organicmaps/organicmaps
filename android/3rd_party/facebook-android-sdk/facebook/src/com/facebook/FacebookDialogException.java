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
 * Represents an error condition relating to displaying a Facebook Web dialog.
 */
public class FacebookDialogException extends FacebookException {
    static final long serialVersionUID = 1;
    private int errorCode;
    private String failingUrl;

    /**
     * Constructs a new FacebookException.
     */
    public FacebookDialogException(String message, int errorCode, String failingUrl) {
        super(message);
        this.errorCode = errorCode;
        this.failingUrl = failingUrl;
    }

    /**
     * Gets the error code received by the WebView. See:
     * http://developer.android.com/reference/android/webkit/WebViewClient.html
     *
     * @return the error code
     */
    public int getErrorCode() {
        return errorCode;
    }

    /**
     * Gets the URL that the dialog was trying to load.
     * @return the URL
     */
    public String getFailingUrl() {
        return failingUrl;
    }
}
