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

import android.test.AndroidTestCase;
import static android.test.MoreAsserts.*;

import android.test.suitebuilder.annotation.SmallTest;
import com.facebook.ProgressNoopOutputStream;

import java.lang.Exception;
import java.lang.Override;

public class ProgressNoopOutputStreamTests extends AndroidTestCase {
    private ProgressNoopOutputStream stream;

    @Override
    protected void setUp() throws Exception {
        stream = new ProgressNoopOutputStream(null);
    }

    @Override
    protected void tearDown() throws Exception {
        stream.close();
    }

    @SmallTest
    public void testSetup() {
        assertEquals(0, stream.getMaxProgress());
        assertEmpty(stream.getProgressMap());
    }

    @SmallTest
    public void testWriting() {
        assertEquals(0, stream.getMaxProgress());

        stream.write(0);
        assertEquals(1, stream.getMaxProgress());

        final byte[] buf = new byte[8];

        stream.write(buf);
        assertEquals(9, stream.getMaxProgress());

        stream.write(buf, 2, 2);
        assertEquals(11, stream.getMaxProgress());

        stream.addProgress(16);
        assertEquals(27, stream.getMaxProgress());
    }
}