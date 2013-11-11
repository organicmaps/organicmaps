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
 * An Exception indicating that a Session failed to open or obtain new permissions.
 */
public class FacebookAuthorizationException extends FacebookException {
    static final long serialVersionUID = 1;

    /**
     * Constructs a FacebookAuthorizationException with no additional
     * information.
     */
    public FacebookAuthorizationException() {
        super();
    }

    /**
     * Constructs a FacebookAuthorizationException with a message.
     * 
     * @param message
     *            A String to be returned from getMessage.
     */
    public FacebookAuthorizationException(String message) {
        super(message);
    }

    /**
     * Constructs a FacebookAuthorizationException with a message and inner
     * error.
     * 
     * @param message
     *            A String to be returned from getMessage.
     * @param throwable
     *            A Throwable to be returned from getCause.
     */
    public FacebookAuthorizationException(String message, Throwable throwable) {
        super(message, throwable);
    }

    /**
     * Constructs a FacebookAuthorizationException with an inner error.
     * 
     * @param throwable
     *            A Throwable to be returned from getCause.
     */
    public FacebookAuthorizationException(Throwable throwable) {
        super(throwable);
    }
}
