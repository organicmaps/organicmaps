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

import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.SmallTest;
import com.facebook.internal.Validate;

import java.util.Arrays;

public class ValidateTests extends AndroidTestCase {
    
    @SmallTest
    public void testNotNullOnNonNull() {
        Validate.notNull("A string", "name");
    }

    @SmallTest
    public void testNotNullOnNull() {
        try {
            Validate.notNull(null, "name");
            fail("expected exception");
        } catch (Exception e) {
        }
    }

    @SmallTest
    public void testNotEmptyOnNonEmpty() {
        Validate.notEmpty(Arrays.asList(new String[] { "hi" }), "name");
    }

    @SmallTest
    public void testNotEmptylOnEmpty() {
        try {
            Validate.notEmpty(Arrays.asList(new String[] {}), "name");
            fail("expected exception");
        } catch (Exception e) {
        }
    }

    @SmallTest
    public void testNotNullOrEmptyOnNonEmpty() {
        Validate.notNullOrEmpty("hi", "name");
    }

    @SmallTest
    public void testNotNullOrEmptyOnEmpty() {
        try {
            Validate.notNullOrEmpty("", "name");
            fail("expected exception");
        } catch (Exception e) {
        }
    }

    @SmallTest
    public void testNotNullOrEmptyOnNull() {
        try {
            Validate.notNullOrEmpty(null, "name");
            fail("expected exception");
        } catch (Exception e) {
        }
    }

    @SmallTest
    public void testOneOfOnValid() {
        Validate.oneOf("hi", "name", "hi", "there");
    }

    @SmallTest
    public void testOneOfOnInvalid() {
        try {
            Validate.oneOf("hit", "name", "hi", "there");
            fail("expected exception");
        } catch (Exception e) {
        }
    }

    @SmallTest
    public void testOneOfOnValidNull() {
        Validate.oneOf(null, "name", "hi", "there", null);
    }

    @SmallTest
    public void testOneOfOnInvalidNull() {
        try {
            Validate.oneOf(null, "name", "hi", "there");
            fail("expected exception");
        } catch (Exception e) {
        }
    }
}
