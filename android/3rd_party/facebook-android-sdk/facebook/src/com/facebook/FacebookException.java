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
 * Represents an error condition specific to the Facebook SDK for Android.
 */
public class FacebookException extends RuntimeException {
    static final long serialVersionUID = 1;

    /**
     * Constructs a new FacebookException.
     */
    public FacebookException() {
        super();
    }

    /**
     * Constructs a new FacebookException.
     * 
     * @param message
     *            the detail message of this exception
     */
    public FacebookException(String message) {
        super(message);
    }

    /**
     * Constructs a new FacebookException.
     * 
     * @param message
     *            the detail message of this exception
     * @param throwable
     *            the cause of this exception
     */
    public FacebookException(String message, Throwable throwable) {
        super(message, throwable);
    }

    /**
     * Constructs a new FacebookException.
     * 
     * @param throwable
     *            the cause of this exception
     */
    public FacebookException(Throwable throwable) {
        super(throwable);
    }
}
