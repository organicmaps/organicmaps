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
import android.test.suitebuilder.annotation.LargeTest;
import android.test.suitebuilder.annotation.MediumTest;
import android.test.suitebuilder.annotation.SmallTest;
import com.facebook.TestUtils;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Random;

public final class FileLruCacheTests extends AndroidTestCase {
    private static final Random random = new Random();

    @SmallTest @MediumTest @LargeTest
    public void testCacheOutputStream() throws Exception {
        int dataSize = 1024;
        byte[] data = generateBytes(dataSize);
        String key = "a";

        // Limit to 2x to allow for extra header data
        FileLruCache cache = new FileLruCache(getContext(), "testCacheOutputStream", limitCacheSize(2*dataSize));

        put(cache, key, data);
        checkValue(cache, key, data);
        TestUtils.clearFileLruCache(cache);
    }

    @SmallTest @MediumTest @LargeTest
    public void testCacheInputStream() throws Exception {
        int dataSize = 1024;
        byte[] data = generateBytes(dataSize);
        String key = "a";
        InputStream stream = new ByteArrayInputStream(data);

        // Limit to 2x to allow for extra header data
        FileLruCache cache = new FileLruCache(getContext(), "testCacheInputStream", limitCacheSize(2*dataSize));
        TestUtils.clearFileLruCache(cache);

        InputStream wrapped = cache.interceptAndPut(key, stream);
        consumeAndClose(wrapped);
        checkValue(cache, key, data);
    }

    @SmallTest @MediumTest @LargeTest
    public void testCacheClear() throws Exception {
        int dataSize = 1024;
        byte[] data = generateBytes(dataSize);
        String key = "a";

        // Limit to 2x to allow for extra header data
        FileLruCache cache = new FileLruCache(getContext(), "testCacheClear", limitCacheSize(2*dataSize));
        TestUtils.clearFileLruCache(cache);

        put(cache, key, data);
        checkValue(cache, key, data);

        TestUtils.clearFileLruCache(cache);
        assertEquals(false, hasValue(cache, key));
        assertEquals(0, cache.sizeInBytesForTest());
    }

    @SmallTest @MediumTest @LargeTest
    public void testCacheClearMidBuffer() throws Exception {
        int dataSize = 1024;
        byte[] data = generateBytes(dataSize);
        String key = "a";
        String key2 = "b";

        // Limit to 2x to allow for extra header data
        FileLruCache cache = new FileLruCache(getContext(), "testCacheClear", limitCacheSize(2*dataSize));
        TestUtils.clearFileLruCache(cache);

        put(cache, key, data);
        checkValue(cache, key, data);
        OutputStream stream = cache.openPutStream(key2);
        Thread.sleep(1000);

        TestUtils.clearFileLruCache(cache);

        stream.write(data);
        stream.close();

        assertEquals(false, hasValue(cache, key));
        assertEquals(false, hasValue(cache, key2));
        assertEquals(0, cache.sizeInBytesForTest());
    }

    @SmallTest @MediumTest @LargeTest
    public void testSizeInBytes() throws Exception {
        int count = 17;
        int dataSize = 53;
        int cacheSize = count * dataSize;
        byte[] data = generateBytes(dataSize);

        // Limit to 2x to allow for extra header data
        FileLruCache cache = new FileLruCache(getContext(), "testSizeInBytes", limitCacheSize(2*cacheSize));
        TestUtils.clearFileLruCache(cache);

        for (int i = 0; i < count; i++) {
            put(cache, i, data);

            // The size reported by sizeInBytes includes a version/size token as well
            // as a JSON blob that records the name.  Verify that the cache size is larger
            // than the data content but not more than twice as large.  This guarantees
            // that sizeInBytes is doing at least approximately the right thing.
            int totalDataSize = (i + 1) * dataSize;
            assertTrue(cache.sizeInBytesForTest() > totalDataSize);
            assertTrue(cache.sizeInBytesForTest() < 2 * totalDataSize);
        }
        for (int i = 0; i < count; i++) {
            String key = Integer.valueOf(i).toString();
            checkValue(cache, key, data);
        }
    }

    @MediumTest @LargeTest
    public void testCacheSizeLimit() throws Exception {
        int count = 64;
        int dataSize = 32;
        int cacheSize = count * dataSize / 2;
        byte[] data = generateBytes(dataSize);

        // Here we do not set the limit to 2x to make sure we hit the limit well before we have
        // added all the data.
        FileLruCache cache = new FileLruCache(getContext(), "testCacheSizeLimit", limitCacheSize(cacheSize));
        TestUtils.clearFileLruCache(cache);

        for (int i = 0; i < count; i++) {
            put(cache, i, data);

            // See comment in testSizeInBytes for why this is not an exact calculation.
            //
            // This changes verification such that the final cache size lands somewhere
            // between half and full quota.
            int totalDataSize = (i + 1) * dataSize;
            assertTrue(cache.sizeInBytesForTest() > Math.min(totalDataSize, cacheSize/2));
            assertTrue(cache.sizeInBytesForTest() < Math.min(2 * totalDataSize, cacheSize));
        }

        // sleep for a bit to make sure the trim finishes
        Thread.sleep(5000);

        // Verify that some keys exist and others do not
        boolean hasValueExists = false;
        boolean hasNoValueExists = false;

        for (int i = 0; i < count; i++) {
            String key = Integer.valueOf(i).toString();
            if (hasValue(cache, key)) {
                hasValueExists = true;
                checkValue(cache, key, data);
            } else {
                hasNoValueExists = true;
            }
        }

        assertEquals(true, hasValueExists);
        assertEquals(true, hasNoValueExists);
    }

    @MediumTest @LargeTest
    public void testCacheCountLimit() throws Exception {
        int count = 64;
        int dataSize = 32;
        int cacheCount = count / 2;
        byte[] data = generateBytes(dataSize);

        // Here we only limit by count, and we allow half of the entries.
        FileLruCache cache = new FileLruCache(getContext(), "testCacheCountLimit", limitCacheCount(cacheCount));
        TestUtils.clearFileLruCache(cache);

        for (int i = 0; i < count; i++) {
            put(cache, i, data);
        }

        // sleep for a bit to make sure the trim finishes
        Thread.sleep(5000);

        // Verify that some keys exist and others do not
        boolean hasValueExists = false;
        boolean hasNoValueExists = false;

        for (int i = 0; i < count; i++) {
            if (hasValue(cache, i)) {
                hasValueExists = true;
                checkValue(cache, i, data);
            } else {
                hasNoValueExists = true;
            }
        }

        assertEquals(true, hasValueExists);
        assertEquals(true, hasNoValueExists);
    }

    @LargeTest
    public void testCacheLru() throws IOException, InterruptedException {
        int keepCount = 10;
        int otherCount = 5;
        int dataSize = 64;
        byte[] data = generateBytes(dataSize);

        // Limit by count, and allow all the keep keys plus one other.
        FileLruCache cache = new FileLruCache(getContext(), "testCacheLru", limitCacheCount(keepCount + 1));
        TestUtils.clearFileLruCache(cache);

        for (int i = 0; i < keepCount; i++) {
            put(cache, i, data);
        }

        // Make sure operations are separated by enough time that the file timestamps are all different.
        // On the test device, it looks like lastModified has 1-second resolution, so we have to wait at
        // least a second to guarantee that updated timestamps will come later.
        Thread.sleep(1000);
        for (int i = 0; i < otherCount; i++) {
            put(cache, keepCount + i, data);
            Thread.sleep(1000);

            // By verifying all the keep keys, they should be LRU and survive while the others do not.
            for (int keepIndex = 0; keepIndex < keepCount; keepIndex++) {
                checkValue(cache, keepIndex, data);
            }
            Thread.sleep(1000);
        }

        // All but the last other key should have been pushed out
        for (int i = 0; i < (otherCount - 1); i++) {
            String key = Integer.valueOf(keepCount + i).toString();
            assertEquals(false, hasValue(cache, key));
        }
    }

    @LargeTest
    public void testConcurrentWritesToSameKey() throws IOException, InterruptedException {
        final int count = 5;
        final int dataSize = 81;
        final int threadCount = 31;
        final int iterationCount = 10;
        final byte[] data = generateBytes(dataSize);

        final FileLruCache cache = new FileLruCache(
                getContext(), "testConcurrentWritesToSameKey", limitCacheCount(count+1));
        TestUtils.clearFileLruCache(cache);

        Runnable run = new Runnable() {
            @Override
            public void run() {
                for (int iterations = 0; iterations < iterationCount; iterations++) {
                    for (int i = 0; i < count; i++) {
                        put(cache, i, data);
                    }
                }
            }
        };

        // Create a bunch of threads to write a set of keys repeatedly
        Thread[] threads = new Thread[threadCount];
        for (int i = 0; i < threads.length; i++) {
            threads[i] = new Thread(run);
        }

        for (Thread thread : threads) {
            thread.start();
        }

        for (Thread thread : threads) {
            thread.join(10 * 1000, 0);
        }

        // Verify that the file state ended up consistent in the end
        for (int i = 0; i < count; i++) {
            checkValue(cache, i, data);
        }
    }

    byte[] generateBytes(int n) {
        byte[] bytes = new byte[n];
        random.nextBytes(bytes);
        return bytes;
    }

    FileLruCache.Limits limitCacheSize(int n) {
        FileLruCache.Limits limits = new FileLruCache.Limits();
        limits.setByteCount(n);
        return limits;
    }

    FileLruCache.Limits limitCacheCount(int n) {
        FileLruCache.Limits limits = new FileLruCache.Limits();
        limits.setFileCount(n);
        return limits;
    }

    void put(FileLruCache cache, int i, byte[] data) {
        put(cache, Integer.valueOf(i).toString(), data);
    }

    void put(FileLruCache cache, String key, byte[] data) {
        try {
            OutputStream stream = cache.openPutStream(key);
            assertNotNull(stream);

            stream.write(data);
            stream.close();
        } catch (IOException e) {
            // Fail test and print Exception
            assertNull(e);
        }
    }

    void checkValue(FileLruCache cache, int i, byte[] expected) {
        checkValue(cache, Integer.valueOf(i).toString(), expected);
    }

    void checkValue(FileLruCache cache, String key, byte[] expected) {
        try {
            InputStream stream = cache.get(key);
            assertNotNull(stream);

            checkInputStream(expected, stream);
            stream.close();
        } catch (IOException e) {
            // Fail test and print Exception
            assertNull(e);
        }
    }

    boolean hasValue(FileLruCache cache, int i) {
        return hasValue(cache, Integer.valueOf(i).toString());
    }

    boolean hasValue(FileLruCache cache, String key) {
        InputStream stream = null;

        try {
            stream = cache.get(key);
        } catch (IOException e) {
            // Fail test and print Exception
            assertNull(e);
        }

        return stream != null;
    }

    void checkInputStream(byte[] expected, InputStream actual) {
        try {
            for (int i = 0; i < expected.length; i++) {
                int b = actual.read();
                assertEquals(((int)expected[i]) & 0xff, b);
            }

            int eof = actual.read();
            assertEquals(-1, eof);
        } catch (IOException e) {
            // Fail test and print Exception
            assertNull(e);
        }
    }

    void consumeAndClose(InputStream stream) {
        try {
            byte[] buffer = new byte[1024];
            while (stream.read(buffer) > -1) {
                // these bytes intentionally ignored
            }
            stream.close();
        } catch (IOException e) {
            // Fail test and print Exception
            assertNull(e);
        }
    }
}
