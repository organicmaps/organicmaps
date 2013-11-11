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

import android.os.Bundle;

/**
 * Implements a trivial {@link TokenCachingStrategy} that does not actually cache any tokens.
 * It is intended for use when an access token may be used on a temporary basis but should not be
 * cached for future use (for instance, when handling a deep link).
 */
public class NonCachingTokenCachingStrategy extends TokenCachingStrategy {
    @Override
    public Bundle load() {
        return null;
    }

    @Override
    public void save(Bundle bundle) {
    }

    @Override
    public void clear() {
    }
}
