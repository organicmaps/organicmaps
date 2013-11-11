/**
 * Copyright 2010-present Facebook
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

package com.facebook.android;

/**
 * Encapsulation of Dialog Error.
 * <p/>
 * THIS CLASS SHOULD BE CONSIDERED DEPRECATED.
 * <p/>
 * All public members of this class are intentionally deprecated.
 * New code should instead use
 * {@link com.facebook.FacebookException}
 * <p/>
 * Adding @Deprecated to this class causes warnings in other deprecated classes
 * that reference this one.  That is the only reason this entire class is not
 * deprecated.
 *
 * @devDocDeprecated
 */
public class DialogError extends Throwable {

    private static final long serialVersionUID = 1L;

    /**
     * The ErrorCode received by the WebView: see
     * http://developer.android.com/reference/android/webkit/WebViewClient.html
     */
    private int mErrorCode;

    /** The URL that the dialog was trying to load */
    private String mFailingUrl;

    @Deprecated
    public DialogError(String message, int errorCode, String failingUrl) {
        super(message);
        mErrorCode = errorCode;
        mFailingUrl = failingUrl;
    }

    @Deprecated
    public int getErrorCode() {
        return mErrorCode;
    }

    @Deprecated
    public String getFailingUrl() {
        return mFailingUrl;
    }

}
