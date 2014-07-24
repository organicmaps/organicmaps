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
import android.test.suitebuilder.annotation.SmallTest;
import com.facebook.Request;
import com.facebook.RequestBatch;
import com.facebook.RequestProgress;

import static android.test.MoreAsserts.*;

import java.io.ByteArrayOutputStream;
import java.lang.Exception;
import java.lang.Override;
import java.util.HashMap;
import java.util.Map;

public class ProgressOutputStreamTests extends AndroidTestCase {
    private static final int MAX_PROGRESS = 10;

    private Request r1, r2;
    private Map<Request, RequestProgress> progressMap;
    private RequestBatch requests;
    private ProgressOutputStream stream;

    @Override
    protected void setUp() throws Exception {
        r1 = new Request(null, "4");
        r2 = new Request(null, "4");

        progressMap = new HashMap<Request, RequestProgress>();
        progressMap.put(r1, new RequestProgress(null, r1));
        progressMap.get(r1).addToMax(5);
        progressMap.put(r2, new RequestProgress(null, r2));
        progressMap.get(r2).addToMax(5);

        requests = new RequestBatch(r1, r2);

        ByteArrayOutputStream backing = new ByteArrayOutputStream();
        stream = new ProgressOutputStream(backing, requests, progressMap, MAX_PROGRESS);
    }

    @Override
    protected void tearDown() throws Exception {
        stream.close();
    }

    @SmallTest
    public void testSetup() {
        assertEquals(0, stream.getBatchProgress());
        assertEquals(MAX_PROGRESS, stream.getMaxProgress());

        for (RequestProgress p : progressMap.values()) {
            assertEquals(0, p.getProgress());
            assertEquals(5, p.getMaxProgress());
        }
    }

    @SmallTest
    public void testWriting() {
        try {
            assertEquals(0, stream.getBatchProgress());

            stream.setCurrentRequest(r1);
            stream.write(0);
            assertEquals(1, stream.getBatchProgress());

            final byte[] buf = new byte[4];
            stream.write(buf);
            assertEquals(5, stream.getBatchProgress());

            stream.setCurrentRequest(r2);
            stream.write(buf, 2, 2);
            stream.write(buf, 1, 3);
            assertEquals(MAX_PROGRESS, stream.getBatchProgress());

            assertEquals(stream.getMaxProgress(), stream.getBatchProgress());
            assertEquals(progressMap.get(r1).getMaxProgress(), progressMap.get(r1).getProgress());
            assertEquals(progressMap.get(r2).getMaxProgress(), progressMap.get(r2).getProgress());
        }
        catch (Exception ex) {
            fail(ex.getMessage());
        }
    }
}