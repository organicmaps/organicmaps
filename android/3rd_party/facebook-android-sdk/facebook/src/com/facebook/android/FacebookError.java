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
 * Encapsulation of a Facebook Error: a Facebook request that could not be
 * fulfilled.
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
public class FacebookError extends RuntimeException {

    private static final long serialVersionUID = 1L;

    private int mErrorCode = 0;
    private String mErrorType;

    @Deprecated
    public FacebookError(String message) {
        super(message);
    }

    @Deprecated
    public FacebookError(String message, String type, int code) {
        super(message);
        mErrorType = type;
        mErrorCode = code;
    }

    @Deprecated
    public int getErrorCode() {
        return mErrorCode;
    }

    @Deprecated
    public String getErrorType() {
        return mErrorType;
    }

}
