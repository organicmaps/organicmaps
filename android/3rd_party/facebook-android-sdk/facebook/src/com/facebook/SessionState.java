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
 * <p>
 * Identifies the state of a Session.
 * </p>
 * <p>
 * Session objects implement a state machine that controls their lifecycle. This
 * enum represents the states of the state machine.
 * </p>
 */
public enum SessionState {
    /**
     * Indicates that the Session has not yet been opened and has no cached
     * token. Opening a Session in this state will involve user interaction.
     */
    CREATED(Category.CREATED_CATEGORY),

    /**
     * <p>
     * Indicates that the Session has not yet been opened and has a cached
     * token. Opening a Session in this state will not involve user interaction.
     * </p>
     * <p>
     * If you are using Session from an Android Service, you must provide a
     * TokenCachingStrategy implementation that contains a valid token to the Session
     * constructor. The resulting Session will be created in this state, and you
     * can then safely call open, passing null for the Activity.
     * </p>
     */
    CREATED_TOKEN_LOADED(Category.CREATED_CATEGORY),

    /**
     * Indicates that the Session is in the process of opening.
     */
    OPENING(Category.CREATED_CATEGORY),

    /**
     * Indicates that the Session is opened. In this state, the Session may be
     * used with a {@link Request}.
     */
    OPENED(Category.OPENED_CATEGORY),

    /**
     * <p>
     * Indicates that the Session is opened and that the token has changed. In
     * this state, the Session may be used with {@link Request}.
     * </p>
     * <p>
     * Every time the token is updated, {@link Session.StatusCallback
     * StatusCallback} is called with this value.
     * </p>
     */
    OPENED_TOKEN_UPDATED(Category.OPENED_CATEGORY),

    /**
     * Indicates that the Session is closed, and that it was not closed
     * normally. Typically this means that the open call failed, and the
     * Exception parameter to {@link Session.StatusCallback StatusCallback} will
     * be non-null.
     */
    CLOSED_LOGIN_FAILED(Category.CLOSED_CATEGORY),

    /**
     * Indicates that the Session was closed normally.
     */
    CLOSED(Category.CLOSED_CATEGORY);

    private final Category category;

    SessionState(Category category) {
        this.category = category;
    }

    /**
     * Returns a boolean indicating whether the state represents a successfully
     * opened state in which the Session can be used with a {@link Request}.
     * 
     * @return a boolean indicating whether the state represents a successfully
     *         opened state in which the Session can be used with a
     *         {@link Request}.
     */
    public boolean isOpened() {
        return this.category == Category.OPENED_CATEGORY;
    }

    /**
     * Returns a boolean indicating whether the state represents a closed
     * Session that can no longer be used with a {@link Request}.
     * 
     * @return a boolean indicating whether the state represents a closed
     * Session that can no longer be used with a {@link Request}.
     */
    public boolean isClosed() {
        return this.category == Category.CLOSED_CATEGORY;
    }

    private enum Category {
        CREATED_CATEGORY, OPENED_CATEGORY, CLOSED_CATEGORY
    }
}
